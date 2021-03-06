#include <deip/chain/deip_evaluator.hpp>
#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/schema/witness_objects.hpp>
#include <deip/chain/schema/block_summary_object.hpp>

#include <deip/chain/util/reward.hpp>

#include <deip/chain/database/database.hpp> //replace to dbservice after _temporary_public_impl remove
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_asset.hpp>
#include <deip/chain/services/dbs_award.hpp>
#include <deip/chain/services/dbs_witness.hpp>
#include <deip/chain/services/dbs_discipline_supply.hpp>
#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/services/dbs_nda_contract.hpp>
#include <deip/chain/services/dbs_nda_contract_requests.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/services/dbs_proposal.hpp>
#include <deip/chain/services/dbs_research_token_sale.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/services/dbs_expertise_contribution.hpp>
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_vesting_balance.hpp>
#include <deip/chain/services/dbs_expertise_allocation_proposal.hpp>
#include <deip/chain/services/dbs_grant_application.hpp>
#include <deip/chain/services/dbs_funding_opportunity.hpp>
#include <deip/chain/services/dbs_dynamic_global_properties.hpp>
#include <deip/chain/services/dbs_research_license.hpp>
#include <deip/chain/services/dbs_contract_agreement.hpp>

#ifndef IS_LOW_MEM
#include <diff_match_patch.h>
#include <boost/locale/encoding_utf.hpp>

    using boost::locale::conv::utf_to_utf;

std::wstring utf8_to_wstring(const std::string& str)
{
    return utf_to_utf<wchar_t>(str.c_str(), str.c_str() + str.size());
}

std::string wstring_to_utf8(const std::wstring& str)
{
    return utf_to_utf<char>(str.c_str(), str.c_str() + str.size());
}

#endif

#include <fc/uint128.hpp>
#include <fc/utf8.hpp>

#include <limits>

namespace deip {
namespace chain {
using fc::uint128_t;


struct strcmp_equal
{
    bool operator()(const fc::shared_string& a, const string& b)
    {
        return a.size() == b.size() || std::strcmp(a.c_str(), b.c_str()) == 0;
    }
};

struct duplicated_entity_guard
{
    duplicated_entity_guard(const dbs_dynamic_global_properties& svc)
        : dgp_service(svc){};

    typedef void result_type;

    void operator()(const create_proposal_operation& op) const
    {
        const auto& entity_id = op.get_entity_id();
        FC_ASSERT(!dgp_service.entity_exists(entity_id),
          "Duplicated entity ${1} detected. Refer other 'ref_block_num' and 'ref_block_prefix' in the "
          "transaction or modify the payload",
          ("1", entity_id));

        for (const op_wrapper& wrap : op.proposed_ops)
        {
            wrap.op.visit(*this);
        }
    }

    template <typename T> void operator()(const T& op) const
    {
        if (!op.entity_id().empty())
        {
            const auto& entity_id = op.get_entity_id();
            FC_ASSERT(!dgp_service.entity_exists(entity_id),
              "Duplicated entity ${1} detected. Refer other 'ref_block_num' and 'ref_block_prefix' in the transaction or modify the payload",
              ("1", entity_id));
        }
    }

private:
    const dbs_dynamic_global_properties& dgp_service;
};


void witness_update_evaluator::do_apply(const witness_update_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    account_service.check_account_existence(o.owner);

    FC_ASSERT(o.url.size() <= DEIP_MAX_WITNESS_URL_LENGTH, "URL is too long");
    FC_ASSERT(o.props.account_creation_fee.symbol == DEIP_SYMBOL);

    const auto& by_witness_name_idx = _db._temporary_public_impl().get_index<witness_index>().indices().get<by_name>();
    auto wit_itr = by_witness_name_idx.find(o.owner);
    if (wit_itr != by_witness_name_idx.end())
    {
        _db._temporary_public_impl().modify(*wit_itr, [&](witness_object& w) {
            fc::from_string(w.url, o.url);
            w.signing_key = o.block_signing_key;
            w.props = o.props;
        });
    }
    else
    {        
        _db._temporary_public_impl().create<witness_object>([&](witness_object& w) {
            w.owner = o.owner;
            fc::from_string(w.url, o.url);
            w.signing_key = o.block_signing_key;
            w.created = _db.head_block_time();
            w.props = o.props;
            w.hardfork_time_vote = _db.get_genesis_time();
        });
    }
}

void create_account_evaluator::do_apply(const create_account_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& account_balance_service = _db.obtain_service<dbs_account_balance>();
    auto& dgp_service = _db.obtain_service<dbs_dynamic_global_properties>();
    auto& discipline_service = _db.obtain_service<dbs_discipline>();
    auto& expert_token_service = _db.obtain_service<dbs_expert_token>();

    const time_point_sec block_time = _db.get_genesis_time();

    const auto& dup_guard = duplicated_entity_guard(dgp_service);
    dup_guard(op);

    FC_ASSERT(op.fee >= DEIP_MIN_ACCOUNT_CREATION_FEE,
      "Insufficient Fee: ${1} required, ${2} provided.",
      ("1", DEIP_MIN_ACCOUNT_CREATION_FEE)("2", op.fee));

    if (op.fee != asset(0, op.fee.symbol))
    {
        FC_ASSERT(account_balance_service.account_balance_exists_by_owner_and_asset(op.creator, op.fee.symbol), "${1} asset balance does not exist for ${2} account", ("1", op.fee.symbol_name())("2", op.creator));
        const auto& creator_balance = account_balance_service.get_account_balance_by_owner_and_asset(op.creator, op.fee.symbol); 
        
        FC_ASSERT(creator_balance.amount >= op.fee.amount, 
          "Insufficient balance to create account.",
          ("creator.balance", creator_balance.amount)("required", op.fee.amount));
    }

    // check accounts existence
    account_service.check_account_existence(op.owner.account_auths);
    account_service.check_account_existence(op.active.account_auths);

    for (const auto& override : op.active_overrides)
    {
        account_service.check_account_existence(override.second.account_auths);
    }

    account_service.create_account_by_faucets(
      op.new_account_name, 
      op.creator, 
      op.memo_key, 
      op.json_metadata, 
      op.owner,
      op.active, 
      op.active_overrides, 
      op.fee, 
      op.traits,
      op.is_user_account()
    );

    if (op.is_user_account())
    {
        const auto& disciplines = discipline_service.lookup_disciplines(discipline_id_type(0), DEIP_API_BULK_FETCH_LIMIT);
        for (const discipline_object& discipline : disciplines)
        {
            if (discipline.external_id == DEIP_COMMON_DISCIPLINE_ID)
            {
                continue;
            }

            const share_type& amount = share_type(DEIP_DEFAULT_EXPERTISE_AMOUNT);

            expert_token_service.create_expert_token(
              op.new_account_name, 
              discipline.id,
              amount, 
              true);

            flat_map<uint16_t, assessment_criteria_value> assessment_criterias;
            const eci_diff account_eci_diff = eci_diff(
              share_type(0), 
              amount,
              block_time, 
              static_cast<uint16_t>(expertise_contribution_type::unknown),
              0,
              assessment_criterias
            );

            _db.push_virtual_operation(account_eci_history_operation(
                op.new_account_name,
                discipline.id._id, 
                static_cast<uint16_t>(reward_recipient_type::unknown),
                account_eci_diff)
            );
        }
    }
}

void update_account_evaluator::do_apply(const update_account_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& account = account_service.get_account(op.account);

    if (op.owner.valid())
    {
        account_service.check_account_existence(op.owner->account_auths);
        account_service.update_owner_authority(account, *op.owner);
    }

    if (op.active.valid())
    {
        account_service.check_account_existence(op.active->account_auths);
        account_service.update_active_authority(account, *op.active);
    }

    if (op.active_overrides.valid())
    {
        const auto& active_overrides = *op.active_overrides;
        for (const auto& active_override : active_overrides)
        {
            if (active_override.second.valid())
            {
                const auto& auth_override = *active_override.second;
                account_service.check_account_existence(auth_override.account_auths);
            }
        }
        account_service.update_active_overrides_authorities(account, *op.active_overrides);
    }

    std::string json_metadata = op.json_metadata.valid() ? *op.json_metadata : fc::to_string(account.json_metadata);
    public_key_type memo_key = op.memo_key.valid() ? *op.memo_key : account.memo_key;

    account_service.update_acount(
      account, 
      memo_key,
      json_metadata, 
      op.traits,
      op.update_extensions
    );
}

/**
 *  Because net_rshares is 0 there is no need to update any pending payout calculations or parent posts.
 */

void transfer_evaluator::do_apply(const transfer_operation& op)
{
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    FC_ASSERT(account_service.account_exists(op.from), 
      "Account ${1} does not exist",
      ("1", op.from));

    FC_ASSERT(account_service.account_exists(op.to), 
      "Account ${1} does not exist",
      ("1", op.to));

    FC_ASSERT(account_balance_service.account_balance_exists_by_owner_and_asset(op.from, op.amount.symbol), "${1} asset balance does not exist for ${2} account", ("1", op.amount.symbol_name())("2", op.from));
    const auto& from_balance = account_balance_service.get_account_balance_by_owner_and_asset(op.from, op.amount.symbol);

    FC_ASSERT(asset(from_balance.amount, from_balance.symbol) >= op.amount, "Account does not have sufficient funds for transfer.");

    account_balance_service.adjust_account_balance(op.from, -op.amount);
    account_balance_service.adjust_account_balance(op.to, op.amount);
}

void transfer_to_common_tokens_evaluator::do_apply(const transfer_to_common_tokens_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();

    FC_ASSERT(account_balance_service.account_balance_exists_by_owner_and_asset(o.from, o.amount.symbol), "${1} asset balance does not exist for ${2} account", ("1", o.amount.symbol_name())("2", o.from));
    const auto& from_balance = account_balance_service.get_account_balance_by_owner_and_asset(o.from, o.amount.symbol);

    const auto& from_account = account_service.get_account(o.from);
    const auto& to_account = o.to.size() ? account_service.get_account(o.to) : from_account;

    FC_ASSERT(from_balance.amount >= o.amount.amount, "Account does not have sufficient DEIP for transfer.");

    account_balance_service.adjust_account_balance(o.from, -o.amount);
    account_service.increase_common_tokens(to_account, o.amount.amount);
}

void withdraw_common_tokens_evaluator::do_apply(const withdraw_common_tokens_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& account = account_service.get_account(o.account);

    FC_ASSERT(account.common_tokens_balance >= 0, "Account does not have sufficient Deip Power for withdraw.");
    FC_ASSERT(account.common_tokens_balance >= o.total_common_tokens_amount,
              "Account does not have sufficient Deip Power for withdraw.");

    if (!account.mined)
    {
        const witness_schedule_object& wso = _db.get_witness_schedule_object();

        share_type min_common_tokens = wso.median_props.account_creation_fee.amount;
        min_common_tokens *= 10;

        FC_ASSERT(
            account.common_tokens_balance > min_common_tokens || o.total_common_tokens_amount == 0,
            "Account registered by another account requires 10x account creation fee worth of Deip Power before it "
            "can be powered down.");
    }

    if (o.total_common_tokens_amount == 0)
    {
        FC_ASSERT(account.common_tokens_withdraw_rate != 0,
                  "This operation would not change the vesting withdraw rate.");

        account_service.update_withdraw(account, 0, time_point_sec::maximum(), 0);
    }
    else
    {

        // DEIP: We have to decide wether we use 13 weeks vesting withdraw period or low it down
        int common_tokens_withdraw_intervals = DEIP_COMMON_TOKENS_WITHDRAW_INTERVALS; /// 13 weeks = 1 quarter of a year

        auto new_common_tokens_withdraw_rate = o.total_common_tokens_amount / share_type(common_tokens_withdraw_intervals);

        if (new_common_tokens_withdraw_rate == 0)
            new_common_tokens_withdraw_rate = 1;

        FC_ASSERT(account.common_tokens_withdraw_rate != new_common_tokens_withdraw_rate,
                  "This operation would not change the vesting withdraw rate.");

        account_service.update_withdraw(account, new_common_tokens_withdraw_rate,
                                        _db.head_block_time() + fc::seconds(DEIP_COMMON_TOKENS_WITHDRAW_INTERVAL_SECONDS),
                                        o.total_common_tokens_amount);
    }
}

void set_withdraw_common_tokens_route_evaluator::do_apply(const set_withdraw_common_tokens_route_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    try
    {
        const auto& from_account = account_service.get_account(o.from_account);
        const auto& to_account = account_service.get_account(o.to_account);
        const auto& wd_idx
            = _db._temporary_public_impl().get_index<withdraw_common_tokens_route_index>().indices().get<by_withdraw_route>();
        auto itr = wd_idx.find(boost::make_tuple(from_account.id, to_account.id));

        if (itr == wd_idx.end())
        {
            FC_ASSERT(o.percent != 0, "Cannot create a 0% destination.");
            FC_ASSERT(from_account.withdraw_routes < DEIP_MAX_WITHDRAW_ROUTES,
                      "Account already has the maximum number of routes.");

            _db._temporary_public_impl().create<withdraw_common_tokens_route_object>(
                [&](withdraw_common_tokens_route_object& wvdo) {
                    wvdo.from_account = from_account.id;
                    wvdo.to_account = to_account.id;
                    wvdo.percent = o.percent;
                    wvdo.auto_common_token = o.auto_common_token;
                });

            account_service.increase_withdraw_routes(from_account);
        }
        else if (o.percent == 0)
        {
            _db._temporary_public_impl().remove(*itr);

            account_service.decrease_withdraw_routes(from_account);
        }
        else
        {
            _db._temporary_public_impl().modify(*itr, [&](withdraw_common_tokens_route_object& wvdo) {
                wvdo.from_account = from_account.id;
                wvdo.to_account = to_account.id;
                wvdo.percent = o.percent;
                wvdo.auto_common_token = o.auto_common_token;
            });
        }

        itr = wd_idx.upper_bound(boost::make_tuple(from_account.id, account_id_type()));
        uint16_t total_percent = 0;

        while (itr->from_account == from_account.id && itr != wd_idx.end())
        {
            total_percent += itr->percent;
            ++itr;
        }

        FC_ASSERT(total_percent <= DEIP_100_PERCENT,
                  "More than 100% of common_tokens withdrawals allocated to destinations.");
    }
    FC_CAPTURE_AND_RETHROW()
}

void account_witness_proxy_evaluator::do_apply(const account_witness_proxy_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& account = account_service.get_account(o.account);
    FC_ASSERT(account.proxy != o.proxy, "Proxy must change.");

    FC_ASSERT(account.can_vote, "Account has declined the ability to vote and cannot proxy votes.");

    optional<account_object> proxy;
    if (o.proxy.size())
    {
        proxy = account_service.get_account(o.proxy);
    }
    account_service.update_voting_proxy(account, proxy);
}

void account_witness_vote_evaluator::do_apply(const account_witness_vote_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_witness& witness_service = _db.obtain_service<dbs_witness>();

    const auto& voter = account_service.get_account(o.account);
    FC_ASSERT(voter.proxy.size() == 0, "A proxy is currently set, please clear the proxy before voting for a witness.");

    if (o.approve)
        FC_ASSERT(voter.can_vote, "Account has declined its voting rights.");

    const auto& witness = _db.get_witness(o.witness);

    const auto& by_account_witness_idx
        = _db._temporary_public_impl().get_index<witness_vote_index>().indices().get<by_account_witness>();
    auto itr = by_account_witness_idx.find(boost::make_tuple(voter.id, witness.id));

    if (itr == by_account_witness_idx.end())
    {
        FC_ASSERT(o.approve, "Vote doesn't exist, user must indicate a desire to approve witness.");

        FC_ASSERT(voter.witnesses_voted_for < DEIP_MAX_ACCOUNT_WITNESS_VOTES,
                  "Account has voted for too many witnesses."); // TODO: Remove after hardfork 2

        _db._temporary_public_impl().create<witness_vote_object>([&](witness_vote_object& v) {
            v.witness = witness.id;
            v.account = voter.id;
        });

        witness_service.adjust_witness_vote(witness, voter.witness_vote_weight());

        account_service.increase_witnesses_voted_for(voter);
    }
    else
    {
        FC_ASSERT(!o.approve, "Vote currently exists, user must indicate a desire to reject witness.");

        witness_service.adjust_witness_vote(witness, -voter.witness_vote_weight());

        account_service.decrease_witnesses_voted_for(voter);
        _db._temporary_public_impl().remove(*itr);
    }
}

void vote_for_review_evaluator::do_apply(const vote_for_review_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& expert_token_service = _db.obtain_service<dbs_expert_token>();
    auto& discipline_service = _db.obtain_service<dbs_discipline>();
    auto& review_service = _db.obtain_service<dbs_review>();
    auto& review_votes_service = _db.obtain_service<dbs_review_vote>();
    auto& expertise_contributions_service = _db.obtain_service<dbs_expertise_contribution>();
    auto& research_service = _db.obtain_service<dbs_research>();
    auto& research_content_service = _db.obtain_service<dbs_research_content>();
    auto& dgp_service = _db.obtain_service<dbs_dynamic_global_properties>();

    const auto& dup_guard = duplicated_entity_guard(dgp_service);
    dup_guard(op);

    const auto& now = _db.head_block_time();

    FC_ASSERT(op.weight.amount > 0, "Review vote weight must be specified.");

    const auto& voter = account_service.get_account(op.voter);
    FC_ASSERT(voter.can_vote, "Review voter has declined his voting rights.");

    const auto& review = review_service.get_review(op.review_external_id);
    const auto& discipline = discipline_service.get_discipline(op.discipline_external_id);

    const auto& expert_token = expert_token_service.get_expert_token_by_account_and_discipline(op.voter, discipline.id);
    const auto& research_content = research_content_service.get_research_content(review.research_content_id);
    const auto& research = research_service.get_research(research_content.research_id);

    FC_ASSERT(std::any_of(review.disciplines.begin(), review.disciplines.end(),
      [&](const discipline_id_type& discipline_id) { return discipline_id == discipline.id; }),
      "Cannot vote with {1} expert tokens as this discipline is not related to the review",
      ("1", discipline.name));

    FC_ASSERT(!review_votes_service.review_vote_exists_by_voter_and_discipline(review.id, voter.name, discipline.id),
              "${1} has voted for this review with ${2} discipline already", ("1", voter.name)("2", discipline.name));

    const int64_t elapsed_seconds = (now - expert_token.last_vote_time).to_seconds();
    const int64_t regenerated_power_percent = (DEIP_100_PERCENT * elapsed_seconds) / DEIP_VOTE_REGENERATION_SECONDS;
    const int64_t current_power_percent = std::min(int64_t(expert_token.voting_power + regenerated_power_percent), int64_t(DEIP_100_PERCENT));
    // FC_ASSERT(current_power_percent > 0, 
    //   "${1} does not have power for ${2} expertise currently to vote for the review. The available power is ${3} %", 
    //   ("1", voter.name)("2", expert_token.discipline_id)("3", current_power_percent / DEIP_1_PERCENT));

    const int64_t vote_applied_power_percent = abs(op.weight.amount.value);
    const int64_t used_power_percent = (current_power_percent * vote_applied_power_percent) / DEIP_100_PERCENT;
    // FC_ASSERT(used_power_percent <= current_power_percent,
    //   "${1} does not have enough power for ${2} expertise to vote for the review with ${3} % of power. The available power is ${4} %",
    //   ("1", voter.name)("2", expert_token.discipline_id)("3", vote_applied_power_percent / DEIP_1_PERCENT)("4", current_power_percent / DEIP_1_PERCENT));

    const uint64_t used_expert_token_amount = ((uint128_t(expert_token.amount.value) * used_power_percent) / (DEIP_100_PERCENT)).to_uint64();
    // FC_ASSERT(used_expert_token_amount > 0, "Account does not have enough power to vote for review.");

    _db._temporary_public_impl().modify(expert_token, [&](expert_token_object& t) {
        t.voting_power = current_power_percent - (DEIP_REVIEW_VOTE_SPREAD_DENOMINATOR != 0 ? (used_power_percent / DEIP_REVIEW_VOTE_SPREAD_DENOMINATOR) : 0);
        t.last_vote_time = now;
    });

    const auto previous_research_content_eci = research_content.eci_per_discipline;
    const auto previous_research_eci = research.eci_per_discipline;

    const review_vote_object& review_vote = review_votes_service.create_review_vote(
      op.external_id,
      voter.name, 
      review.external_id,
      review.id,
      discipline.external_id,
      discipline.id, 
      used_expert_token_amount, 
      now, 
      review.created_at,
      research_content.id,
      research.id
    );

    const research_content_object& updated_research_content = research_content_service.update_eci_evaluation(research_content.id);
    const research_object& updated_research = research_service.update_eci_evaluation(research.id);

    flat_map<uint16_t, assessment_criteria_value> assessment_criterias;
    for (const auto& criteria : review.assessment_criterias)
    {
        assessment_criterias.insert(std::make_pair(criteria.first, assessment_criteria_value(1)));
    }

    const eci_diff research_content_eci_diff = eci_diff(
      previous_research_content_eci.at(discipline.id),
      updated_research_content.eci_per_discipline.at(discipline.id),
      now,
      static_cast<uint16_t>(expertise_contribution_type::review_support),
      review_vote.id._id,
      assessment_criterias
    );

    expertise_contributions_service.adjust_expertise_contribution(
        discipline.id, 
        research.id, 
        research_content.id,
        research_content_eci_diff
    );

    _db.push_virtual_operation(research_content_eci_history_operation(
        research_content.id._id, 
        discipline.id._id,
        research_content_eci_diff)
    );

    const eci_diff research_eci_diff = eci_diff(
      previous_research_eci.at(discipline.id),
      updated_research.eci_per_discipline.at(discipline.id),
      now,
      static_cast<uint16_t>(expertise_contribution_type::review_support),
      review_vote.id._id,
      assessment_criterias
    );

    _db.push_virtual_operation(research_eci_history_operation(
        research.id._id, 
        discipline.id._id,
        research_eci_diff)
    );
}

void request_account_recovery_evaluator::do_apply(const request_account_recovery_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_witness& witness_service = _db.obtain_service<dbs_witness>();

    const auto& account_to_recover = account_service.get_account(o.account_to_recover);

    if (account_to_recover.recovery_account.length()) // Make sure recovery matches expected recovery account
        FC_ASSERT(account_to_recover.recovery_account == o.recovery_account,
                  "Cannot recover an account that does not have you as there recovery partner.");
    else // Empty string recovery account defaults to top witness
        FC_ASSERT(witness_service.get_top_witness().owner == o.recovery_account,
                  "Top witness must recover an account with no recovery partner.");

    account_service.create_account_recovery(o.account_to_recover, o.new_owner_authority);
}

void recover_account_evaluator::do_apply(const recover_account_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& account_to_recover = account_service.get_account(o.account_to_recover);

    FC_ASSERT(_db.head_block_time() - account_to_recover.last_account_recovery > DEIP_OWNER_UPDATE_LIMIT,
              "Owner authority can only be updated once an hour.");

    account_service.submit_account_recovery(account_to_recover, o.new_owner_authority, o.recent_owner_authority);
}

void change_recovery_account_evaluator::do_apply(const change_recovery_account_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    account_service.check_account_existence(o.new_recovery_account); // Simply validate account exists
    const auto& account_to_recover = account_service.get_account(o.account_to_recover);

    account_service.change_recovery_account(account_to_recover, o.new_recovery_account);
}

void create_proposal_evaluator::do_apply(const create_proposal_operation& op)
{
    auto& proposals_service = _db.obtain_service<dbs_proposal>();
    auto& dgp_service = _db.obtain_service<dbs_dynamic_global_properties>();
    const auto& block_time = _db.head_block_time();

    const auto& dup_guard = duplicated_entity_guard(dgp_service);
    dup_guard(op);

    FC_ASSERT(op.expiration_time > block_time, "Proposal has already expired on creation.");
    FC_ASSERT(op.expiration_time <= block_time + DEIP_MAX_PROPOSAL_LIFETIME_SEC,
      "Proposal expiration time is too far in the future.");
    FC_ASSERT(!op.review_period_seconds || fc::seconds(*op.review_period_seconds) < (op.expiration_time - block_time),
      "Proposal review period must be less than its overall lifetime." );

    flat_set<account_name_type> required_active;
    flat_set<account_name_type> required_owner;
    vector<authority> other;

    for( auto& wrap : op.proposed_ops )
    {
        operation_get_required_authorities(wrap.op, required_active, required_owner, other);
    }

    FC_ASSERT(other.size() == 0, "Not implemented for standalone auth"); // TODO: what about other??? 

    const uint16_t ref_block_num = _db.current_proposed_trx().valid() 
      ? _db.current_proposed_trx()->ref_block_num
      : _db.current_trx_ref_block_num();

    const uint32_t ref_block_prefix = _db.current_proposed_trx().valid() 
      ? _db.current_proposed_trx()->ref_block_prefix
      : _db.current_trx_ref_block_prefix();

    transaction proposed_trx;
    proposed_trx.expiration = op.expiration_time;
    proposed_trx.ref_block_num = ref_block_num;
    proposed_trx.ref_block_prefix = ref_block_prefix;

    for (const op_wrapper& wrap : op.proposed_ops)
    {
        proposed_trx.operations.push_back(wrap.op);
    }

    proposed_trx.validate();

    // All accounts which must provide both owner and active authority should be omitted from the active authority set;
    // owner authority approval implies active authority approval.
    flat_set<account_name_type> remaining_active;
    std::set_difference(
      required_active.begin(), required_active.end(),
      required_owner.begin(), required_owner.end(),
      std::inserter(remaining_active, remaining_active.begin())
    );

    proposals_service.create_proposal(
      op.external_id, 
      proposed_trx,
      op.expiration_time,
      op.creator,
      op.review_period_seconds,
      required_owner,
      remaining_active
    );
}

void update_proposal_evaluator::do_apply(const update_proposal_operation& op)
{
    auto& proposals_service = _db.obtain_service<dbs_proposal>();
    const auto& block_time = _db.head_block_time();
    chainbase::database& db = _db._temporary_public_impl();

    FC_ASSERT(proposals_service.proposal_exists(op.external_id),
      "Proposal ${1} does not exist", ("1", op.external_id));

    const auto& proposal = proposals_service.get_proposal(op.external_id);

    if (proposal.review_period_time.valid() && block_time >= *proposal.review_period_time)
    {
        FC_ASSERT(
          op.owner_approvals_to_add.empty() && 
          op.active_approvals_to_add.empty() && 
          op.key_approvals_to_add.empty(), 
          "This proposal is in its review period. No new approvals may be added."
        );
    }

    for (const account_name_type& account : op.owner_approvals_to_add)
    {
        FC_ASSERT(proposal.available_owner_approvals.find(account) == proposal.available_owner_approvals.end(), 
        "", ("account", account)("available", proposal.available_owner_approvals));
    }
    for (const account_name_type& account : op.active_approvals_to_add)
    {
        FC_ASSERT(proposal.available_active_approvals.find(account) == proposal.available_active_approvals.end(), 
        "", ("account", account)("available", proposal.available_active_approvals));
    }
    for (const public_key_type& key : op.key_approvals_to_add)
    {
        FC_ASSERT(proposal.available_key_approvals.find(key) == proposal.available_key_approvals.end(), 
        "", ("key", key)("available", proposal.available_key_approvals));
    }

    for (const account_name_type& account : op.owner_approvals_to_remove)
    {
        FC_ASSERT(proposal.available_owner_approvals.find(account) != proposal.available_owner_approvals.end(), 
        "", ("account", account)("available", proposal.available_owner_approvals));
    }
    for (const account_name_type& account : op.active_approvals_to_remove)
    {
        FC_ASSERT(proposal.available_active_approvals.find(account) != proposal.available_active_approvals.end(),
          "", ("account", account)("available", proposal.available_active_approvals));
    }
    for (const public_key_type& key : op.key_approvals_to_remove)
    {
        FC_ASSERT(proposal.available_key_approvals.find(key) != proposal.available_key_approvals.end(), 
        "", ("account", key)("available", proposal.available_key_approvals));
    }


    proposals_service.update_proposal(proposal, 
      op.owner_approvals_to_add,
      op.active_approvals_to_add,
      op.owner_approvals_to_remove,
      op.active_approvals_to_remove,
      op.key_approvals_to_add,
      op.key_approvals_to_remove
    );

    // If the proposal has a review period, don't bother attempting to authorize/execute it.
    // Proposals with a review period may never be executed except at their expiration.
    if (proposal.review_period_time.valid()) return;

    if (proposal.is_authorized_to_execute(db))
    {
        // All required approvals are satisfied. Execute!
        try 
        {
            _db.push_proposal(proposal);
        } 
        catch(fc::exception& e) {
            _db.reset_current_proposed_trx();
            _db._temporary_public_impl().modify(proposal, [&e](proposal_object& p) {
                fc::from_string(p.fail_reason, e.to_string(fc::log_level(fc::log_level::all)));
            });
            wlog("Proposed transaction ${id} failed to apply once approved with exception:\n----\n${reason}\n----\nWill try again when it expires.", ("id", proposal.external_id)("reason", e.to_detail_string()));
            _db.push_virtual_operation(proposal_status_changed_operation(proposal.external_id, static_cast<uint8_t>(proposal_status::failed)));
        }
    }
}

void delete_proposal_evaluator::do_apply(const delete_proposal_operation& op)
{
    auto& proposals_service = _db.obtain_service<dbs_proposal>();

    FC_ASSERT(proposals_service.proposal_exists(op.external_id),
      "Proposal ${1} does not exist", ("1", op.external_id));

    const auto& proposal = proposals_service.get_proposal(op.external_id);
    const auto& required_approvals = 
      op.authority == static_cast<uint16_t>(authority_type::owner) 
      ? proposal.required_owner_approvals 
      : op.authority == static_cast<uint16_t>(authority_type::active)
      ? proposal.required_active_approvals 
      : flat_set<account_name_type>();

    FC_ASSERT(required_approvals.find(op.account) != required_approvals.end(),
      "Provided authority is not authoritative for this proposal.",
      ("provided", op.account)("required", required_approvals));

    proposals_service.remove_proposal(proposal);
}

void create_review_evaluator::do_apply(const create_review_operation& op)
{
    auto& research_service = _db.obtain_service<dbs_research>();
    auto& disciplines_service = _db.obtain_service<dbs_discipline>();
    auto& review_service = _db.obtain_service<dbs_review>();
    auto& research_content_service = _db.obtain_service<dbs_research_content>();
    auto& research_discipline_service = _db.obtain_service<dbs_research_discipline_relation>();
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& expertise_token_service = _db.obtain_service<dbs_expert_token>();
    auto& expertise_contributions_service = _db.obtain_service<dbs_expertise_contribution>();
    auto& dgp_service = _db.obtain_service<dbs_dynamic_global_properties>();

    const auto& dup_guard = duplicated_entity_guard(dgp_service);
    dup_guard(op);

    FC_ASSERT(op.weight.amount > 0, "Review weight must be specified.");

    account_service.check_account_existence(op.author);
    const auto& research_content_opt = research_content_service.get_research_content_if_exists(op.research_content_external_id);
    FC_ASSERT(research_content_opt.valid(), "Research content ${1} does not exist", ("1", op.research_content_external_id));

    const auto& research_content = (*research_content_opt).get();
    const auto& research = research_service.get_research(research_content.research_id);
    const auto& existing_reviews = review_service.get_reviews_by_research_content(research_content.id);
    const auto& now = _db.head_block_time();

    FC_ASSERT(std::none_of(existing_reviews.begin(), existing_reviews.end(),
      [&](const review_object& rw) { return rw.author == op.author && rw.research_content_id == research_content.id; }),
      "${1} has reviewed research content ${2} already", 
      ("1", op.author)("2", research_content.id));

    const auto& expertise_tokens = expertise_token_service.get_expert_tokens_by_account_name(op.author);
    const auto& research_disciplines_relations = research_discipline_service.get_research_discipline_relations_by_research(research_content.research_id);
    
    for (const auto& external_id : op.disciplines)
    {
        const auto& discipline = disciplines_service.get_discipline(external_id);

        FC_ASSERT(std::any_of(research_disciplines_relations.begin(), research_disciplines_relations.end(),
          [&](const research_discipline_relation_object& relation) { return relation.discipline_id == discipline.id; }));

        FC_ASSERT(std::any_of(expertise_tokens.begin(), expertise_tokens.end(),
          [&](const expert_token_object& exp) { return exp.discipline_id == discipline.id; }));
    }


    std::map<discipline_id_type, share_type> review_used_expertise_by_disciplines;
    for (auto& wrap : expertise_tokens)
    {
        const auto& expert_token = wrap.get();

        if (std::any_of(op.disciplines.begin(), op.disciplines.end(),
            [&](const external_id_type& discipline_exernal_id) {
                return discipline_exernal_id == expert_token.discipline_external_id;
            }))
        {
            const int64_t elapsed_seconds = (now - expert_token.last_vote_time).to_seconds();
            const int64_t regenerated_power_percent = (DEIP_100_PERCENT * elapsed_seconds) / DEIP_VOTE_REGENERATION_SECONDS;
            const int64_t current_power_percent = std::min(int64_t(expert_token.voting_power + regenerated_power_percent), int64_t(DEIP_100_PERCENT));
            // FC_ASSERT(current_power_percent > 0, 
            //         "${1} does not have power for ${2} expertise currently to make the review. The available power is ${3} %", 
            //         ("1", op.author)("2", expert_token.discipline_id)("3", current_power_percent / DEIP_1_PERCENT));

            const int64_t review_applied_power_percent = op.weight.amount.value;
            const int64_t used_power_percent = (DEIP_REVIEW_REQUIRED_POWER_PERCENT * review_applied_power_percent) / DEIP_100_PERCENT;
            // FC_ASSERT(used_power_percent <= current_power_percent,
            //         "${1} does not have enough power for ${2} expertise to make the review with ${3} % of power. The available power is ${4} %",
            //         ("1", op.author)("2", expert_token.discipline_id)("3", review_applied_power_percent / DEIP_1_PERCENT)("4", current_power_percent / DEIP_1_PERCENT));

            const uint64_t used_expert_token_amount = ((uint128_t(expert_token.amount.value) * current_power_percent) / (DEIP_100_PERCENT)).to_uint64();
            // FC_ASSERT(used_expert_token_amount > 0, "Account does not have enough power to make the review.");

            _db._temporary_public_impl().modify(expert_token, [&](expert_token_object& exp) {
                exp.voting_power = current_power_percent - used_power_percent;
                exp.last_vote_time = now;
            });

            review_used_expertise_by_disciplines.insert(std::make_pair(expert_token.discipline_id, used_expert_token_amount));
        }
    }

    FC_ASSERT(review_used_expertise_by_disciplines.size() != 0, 
        "${1} does not have expertise to review ${2} research", 
        ("1", op.author)("2", research.id));

    const std::set<discipline_id_type>& review_disciplines = std::accumulate(review_used_expertise_by_disciplines.begin(), review_used_expertise_by_disciplines.end(), std::set<discipline_id_type>(),
        [=](std::set<discipline_id_type> acc, std::pair<discipline_id_type, share_type> entry) {
            acc.insert(entry.first);
            return acc;
        });

    bool is_positive;
    flat_map<uint16_t, assessment_criteria_value> assessment_criterias;

    if (op.assessment_model.which() == assessment_models::tag<binary_scoring_assessment_model_type>::value)
    {
        const auto model = op.assessment_model.get<binary_scoring_assessment_model_type>();
        is_positive = model.is_positive;
    }
    else if (op.assessment_model.which() == assessment_models::tag<multicriteria_scoring_assessment_model_type>::value)
    {
        const auto model = op.assessment_model.get<multicriteria_scoring_assessment_model_type>();

        const assessment_criteria_value total_score = std::accumulate(std::begin(model.scores), std::end(model.scores), assessment_criteria_value(0),
            [&](assessment_criteria_value total, const std::map<uint16_t, uint16_t>::value_type& m) { return total + assessment_criteria_value(m.second); });

        is_positive = total_score >= (assessment_criteria_value) DEIP_MIN_POSITIVE_REVIEW_SCORE;

        for (const auto& score : model.scores)
        {
            assessment_criterias.insert(std::make_pair(score.first, assessment_criteria_value(score.second)));
        }
    } 
    else
    {
        FC_ASSERT(false, "Review assessment model is not specified");
    }

    const auto previous_research_content_eci = research_content.eci_per_discipline;
    const auto previous_research_eci = research.eci_per_discipline;

    const auto& review = review_service.create_review(
      op.external_id,
      research.external_id,
      research_content.external_id,
      research_content.id,
      op.content,
      is_positive,
      op.author,
      review_disciplines,
      review_used_expertise_by_disciplines,
      op.assessment_model.which(),
      assessment_criterias);

    const research_content_object& updated_research_content = research_content_service.update_eci_evaluation(research_content.id);
    const research_object& updated_research = research_service.update_eci_evaluation(research.id);

    for (auto& review_discipline_id : review_disciplines)
    {
        const eci_diff research_content_eci_diff = eci_diff(
          previous_research_content_eci.at(review_discipline_id),
          updated_research_content.eci_per_discipline.at(review_discipline_id),
          now,
          static_cast<uint16_t>(expertise_contribution_type::review),
          review.id._id,
          assessment_criterias
        );

        expertise_contributions_service.adjust_expertise_contribution(
            review_discipline_id,
            research.id, 
            research_content.id, 
            research_content_eci_diff
        );

        _db.push_virtual_operation(research_content_eci_history_operation(
            research_content.id._id, 
            review_discipline_id._id,
            research_content_eci_diff)
        );

        const eci_diff research_eci_diff = eci_diff(
          previous_research_eci.at(review_discipline_id),
          updated_research.eci_per_discipline.at(review_discipline_id),
          now,
          static_cast<uint16_t>(expertise_contribution_type::review),
          review.id._id,
          assessment_criterias
        );

        _db.push_virtual_operation(research_eci_history_operation(
            research.id._id,
            review_discipline_id._id,
            research_eci_diff)
        );
    }
}

void contribute_to_token_sale_evaluator::do_apply(const contribute_to_token_sale_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& account_balance_service = _db.obtain_service<dbs_account_balance>();
    auto& research_token_sale_service = _db.obtain_service<dbs_research_token_sale>();
    auto& research_service = _db.obtain_service<dbs_research>();
    const auto& block_time = _db.head_block_time();

    FC_ASSERT(account_service.account_exists(op.contributor), 
      "Account ${1} does not exist",
      ("1", op.contributor));

    const auto& research_token_sale_opt = research_token_sale_service.get_research_token_sale_if_exists(op.token_sale_external_id);

    FC_ASSERT(research_token_sale_opt.valid(), 
      "Research token sale ${1} does not exist",
      ("1", op.token_sale_external_id));

    const research_token_sale_object& research_token_sale = *research_token_sale_opt;
    const auto& research = research_service.get_research(research_token_sale.research_external_id);

    FC_ASSERT(research_token_sale.status == static_cast<uint16_t>(research_token_sale_status::active),
      "Research token sale ${1} is in ${2} status",
      ("1", op.token_sale_external_id)("2", research_token_sale.status));

    FC_ASSERT(account_balance_service.account_balance_exists_by_owner_and_asset(op.contributor, op.amount.symbol), "${1} asset balance does not exist for ${2} account", ("1", op.amount.symbol_name())("2", op.contributor));
    const auto& account_balance = account_balance_service.get_account_balance_by_owner_and_asset(op.contributor, op.amount.symbol);

    FC_ASSERT(account_balance.amount >= op.amount.amount, 
      "Not enough funds to contribute. Available: ${1} Requested: ${2}", 
      ("1", account_balance.amount)("2", op.amount));

    asset amount_to_contribute = op.amount;
    const bool is_hard_cap_reached = research_token_sale.total_amount + amount_to_contribute >= research_token_sale.hard_cap;

    if (is_hard_cap_reached) 
    {
        amount_to_contribute = research_token_sale.hard_cap - research_token_sale.total_amount;
    }

    const auto& research_token_sale_contribution = research_token_sale_service.contribute(research_token_sale.id, op.contributor, block_time, amount_to_contribute);

    account_balance_service.adjust_account_balance(op.contributor, -amount_to_contribute);
    research_token_sale_service.collect_funds(research_token_sale.id, amount_to_contribute);

    if (is_hard_cap_reached)
    {
        research_token_sale_service.update_status(research_token_sale.id, research_token_sale_status::finished);
        research_token_sale_service.finish_research_token_sale(research_token_sale.id);
    }

    _db.push_virtual_operation(token_sale_contribution_to_history_operation(
      research_token_sale.research_id._id,
      research.external_id,
      research_token_sale.id._id,
      research_token_sale.external_id,
      op.contributor,
      amount_to_contribute
    ));
}

void create_vesting_balance_evaluator::do_apply(const create_vesting_balance_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();
    dbs_vesting_balance& vesting_balance_service = _db.obtain_service<dbs_vesting_balance>();

    account_service.check_account_existence(op.creator);
    account_service.check_account_existence(op.owner);

    FC_ASSERT(account_balance_service.account_balance_exists_by_owner_and_asset(op.creator, op.balance.symbol), "${1} asset balance does not exist for ${2} account", ("1", op.balance.symbol_name())("2", op.creator));
    const auto& account_balance = account_balance_service.get_account_balance_by_owner_and_asset(op.creator, op.balance.symbol);

    FC_ASSERT(account_balance.amount >= op.balance.amount, "Not enough funds to create vesting contract");

    account_balance_service.adjust_account_balance(op.creator, -op.balance);
    vesting_balance_service.create(op.owner, op.balance, op.vesting_duration_seconds, op.period_duration_seconds, op.vesting_cliff_seconds);

}

void withdraw_vesting_balance_evaluator::do_apply(const withdraw_vesting_balance_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();
    dbs_vesting_balance& vesting_balance_service = _db.obtain_service<dbs_vesting_balance>();

    account_service.check_account_existence(op.owner);
    vesting_balance_service.check_existence(op.vesting_balance_id);

    const auto& vco = vesting_balance_service.get(op.vesting_balance_id);
    FC_ASSERT(op.owner == vco.owner, "", ("op.owner", op.owner)("vco.owner", vco.owner));

    const auto now = _db.head_block_time();
    share_type allowed_withdraw = 0;

    if (now >= vco.start_timestamp) {
        const auto elapsed_seconds = (now - vco.start_timestamp).to_seconds();

        if (elapsed_seconds >= vco.vesting_cliff_seconds) {
            share_type total_vested = 0;

            if (elapsed_seconds < vco.vesting_duration_seconds) {
                const uint32_t withdraw_period = elapsed_seconds / vco.period_duration_seconds;
                const uint32_t total_periods = vco.vesting_duration_seconds / vco.period_duration_seconds;
                total_vested = (vco.balance.amount * withdraw_period) / total_periods;
            } else {
                total_vested = vco.balance.amount;
            }
            FC_ASSERT(total_vested >= 0);

            allowed_withdraw = total_vested - vco.withdrawn.amount;
            FC_ASSERT(allowed_withdraw >= 0);
            FC_ASSERT(allowed_withdraw >= op.amount.amount,
                    "You cannot withdraw requested amount. Allowed withdraw: ${a}, requested amount: ${r}.",
                    ("a", asset(allowed_withdraw, DEIP_SYMBOL))("r", op.amount));

            vesting_balance_service.withdraw(vco.id, op.amount);
            account_balance_service.adjust_account_balance(op.owner, op.amount);
        }
    }
}

void transfer_research_share_evaluator::do_apply(const transfer_research_share_operation& op)
{

}

void create_expertise_allocation_proposal_evaluator::do_apply(const create_expertise_allocation_proposal_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_expert_token& expert_token_service = _db.obtain_service<dbs_expert_token>();
    dbs_expertise_allocation_proposal& expertise_allocation_proposal_service = _db.obtain_service<dbs_expertise_allocation_proposal>();

    account_service.check_account_existence(op.claimer);

    FC_ASSERT(!expert_token_service.expert_token_exists_by_account_and_discipline(op.claimer, op.discipline_id),
              "Expert token for account \"${1}\" and discipline \"${2}\" already exists", ("1", op.claimer)("2", op.discipline_id));

    FC_ASSERT(!expertise_allocation_proposal_service.exists_by_claimer_and_discipline(op.claimer, op.discipline_id),
              "You have created expertise allocation proposal already");

    expertise_allocation_proposal_service.create(op.claimer, op.discipline_id, op.description);
}

void vote_for_expertise_allocation_proposal_evaluator::do_apply(const vote_for_expertise_allocation_proposal_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_expert_token& expert_token_service = _db.obtain_service<dbs_expert_token>();
    dbs_expertise_allocation_proposal& expertise_allocation_proposal_service = _db.obtain_service<dbs_expertise_allocation_proposal>();

    auto& proposal = expertise_allocation_proposal_service.get(op.proposal_id);

    account_service.check_account_existence(op.voter);

    FC_ASSERT(expert_token_service.expert_token_exists_by_account_and_discipline(op.voter, proposal.discipline_id),
      "Expertise token ${1} for ${2} does not exist", 
      ("1", op.voter)("2", proposal.discipline_id));

    auto& expert_token = expert_token_service.get_expert_token_by_account_and_discipline(op.voter, proposal.discipline_id);

    if (op.voting_power == DEIP_100_PERCENT)
        expertise_allocation_proposal_service.upvote(proposal, op.voter, expert_token.amount);
    else if (op.voting_power == -DEIP_100_PERCENT)
        expertise_allocation_proposal_service.downvote(proposal, op.voter, expert_token.amount);

}

void create_grant_evaluator::do_apply(const create_grant_operation& op)
{
    const auto& account_service = _db.obtain_service<dbs_account>();
    auto& account_balance_service = _db.obtain_service<dbs_account_balance>();
    auto& funding_opportunities_service = _db.obtain_service<dbs_funding_opportunity>();
    const auto& assets_service = _db.obtain_service<dbs_asset>();
    auto& discipline_supply_service = _db.obtain_service<dbs_discipline_supply>();
    auto& discipline_service = _db.obtain_service<dbs_discipline>();

    FC_ASSERT(account_service.account_exists(op.grantor), "Account ${a} does not exists", ("a", op.grantor));
    FC_ASSERT(assets_service.asset_exists_by_symbol(op.amount.symbol), "Asset ${s} does not exists", ("s", op.amount.symbol));

    std::set<discipline_id_type> target_disciplines;
    for (const auto& external_id : op.target_disciplines)
    {
        const auto& discipline = discipline_service.get_discipline(external_id);
        target_disciplines.insert(discipline.id);
    }

    FC_ASSERT(target_disciplines.size() != 0, "Grant target disciplines are not specified");

    FC_ASSERT(account_balance_service.account_balance_exists_by_owner_and_asset(op.grantor, op.amount.symbol),
              "${1} asset balance does not exist for ${2} account", ("1", op.amount.symbol_name())("2", op.grantor));
    const account_balance_object& grantor_balance = account_balance_service.get_account_balance_by_owner_and_asset(op.grantor, op.amount.symbol);
    
    FC_ASSERT(grantor_balance.amount >= op.amount.amount, 
      "Grantor ${g} does not have enough funds. Requested: ${ga} Actual: ${ba}", 
      ("g", op.grantor)("ga", op.amount)("ba", grantor_balance.amount));

    if (op.distribution_model.which() == grant_distribution_models::tag<announced_application_window_contract_type>::value)
    {
        const auto contract = op.distribution_model.get<announced_application_window_contract_type>();
        

        FC_ASSERT(contract.open_date >= _db.head_block_time(), "Start date must be greater than now");
        FC_ASSERT(contract.open_date < contract.close_date, "Open date must be earlier than close date");

        const auto& review_committee = account_service.get_account(contract.review_committee_id);

        funding_opportunities_service.create_grant_with_eci_evaluation_distribution(
          op.grantor,
          op.amount,
          target_disciplines,
          op.external_id,
          contract.additional_info,
          review_committee.id,
          review_committee.name,
          contract.min_number_of_positive_reviews,
          contract.min_number_of_applications,
          contract.max_number_of_research_to_grant,
          contract.open_date,
          contract.close_date);
    }
 
    else if (op.distribution_model.which() == grant_distribution_models::tag<funding_opportunity_announcement_contract_type>::value)
    {
        const auto contract = op.distribution_model.get<funding_opportunity_announcement_contract_type>();
        const auto& organization = account_service.get_account(contract.organization_id);
        const auto& review_committee = account_service.get_account(contract.review_committee_id);
        const auto& treasury = account_service.get_account(contract.treasury_id);

        FC_ASSERT(contract.open_date >= _db.head_block_time(), "Open date must be greater than now");
        FC_ASSERT(contract.open_date < contract.close_date, "Open date must be earlier than close date");

        funding_opportunities_service.create_grant_with_officer_evaluation_distribution(
          organization.id,
          organization.name,
          review_committee.id,
          review_committee.name,
          treasury.id,
          treasury.name,
          op.grantor,
          op.external_id,
          contract.additional_info,
          target_disciplines,
          op.amount,
          contract.award_ceiling,
          contract.award_floor,
          contract.expected_number_of_awards,
          contract.officers,
          contract.open_date,
          contract.close_date);
    }

    else if (op.distribution_model.which() == grant_distribution_models::tag<discipline_supply_announcement_contract_type>::value)
    {
        const auto contract = op.distribution_model.get<discipline_supply_announcement_contract_type>();

        int64_t target_discipline = (*target_disciplines.begin())._id;
        discipline_supply_service.create_discipline_supply(
          op.grantor,
          op.amount,
          contract.start_time,
          contract.end_time,
          target_discipline,
          contract.is_extendable,
          contract.content_hash,
          contract.additional_info);
    }

    else 
    {
        FC_ASSERT(true, "WIP");
    }
}

void create_grant_application_evaluator::do_apply(const create_grant_application_operation& op)
{
    const dbs_account& account_service = _db.obtain_service<dbs_account>();
    const dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();
    const dbs_research& research_service = _db.obtain_service<dbs_research>();
    const dbs_research_discipline_relation& research_discipline_relation_service = _db.obtain_service<dbs_research_discipline_relation>();
    dbs_grant_application& grant_application_service = _db.obtain_service<dbs_grant_application>();

    FC_ASSERT(account_service.account_exists(op.creator),
              "Account(creator) ${1} does not exist",
              ("1", op.creator));

    FC_ASSERT(funding_opportunity_service.funding_opportunity_exists(op.funding_opportunity_number),
              "Grant ${1} does not exist",
              ("1", op.funding_opportunity_number));

    FC_ASSERT(research_service.research_exists(op.research_id),
              "Research with id: ${1} does not exist",
              ("1", op.research_id));
    
    const auto& research = research_service.get_research(op.research_id);

    const auto& grant = funding_opportunity_service.get_funding_opportunity(op.funding_opportunity_number);

    FC_ASSERT(grant.distribution_type == static_cast<uint16_t>(funding_opportunity_distribution_type::eci_evaluation),
              "You can create application only for funding opportunity with eci evaluation distribution type. Funding opportunity distribution type: ${1}",
              ("1", grant.distribution_type));

    auto parent_discipline_itr = std::min_element(grant.target_disciplines.begin(), grant.target_disciplines.end());
    FC_ASSERT(parent_discipline_itr != grant.target_disciplines.end(), "Grant main disciplne is not defined");

    FC_ASSERT(research_discipline_relation_service.exists_by_research_and_discipline(op.research_id, *parent_discipline_itr),
              "Research discipline relation with research ID: ${1} and discipline ID: ${2} does not exist",
              ("1", op.research_id)("2", *parent_discipline_itr));

    auto now = _db.head_block_time();
    FC_ASSERT((now >= grant.open_date) && (now <= grant.close_date), "Grant is inactive now");

    grant_application_service.create_grant_application(op.funding_opportunity_number, op.research_id, op.application_hash, op.creator);
}

void create_review_for_application_evaluator::do_apply(const create_review_for_application_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_expert_token& expertise_token_service = _db.obtain_service<dbs_expert_token>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_grant_application& grant_application_service = _db.obtain_service<dbs_grant_application>();
    dbs_research_discipline_relation& research_discipline_service = _db.obtain_service<dbs_research_discipline_relation>();

    FC_ASSERT(account_service.account_exists(op.author),
              "Account(author) ${1} does not exist",
              ("1", op.author));

    FC_ASSERT(grant_application_service.grant_application_exists(op.grant_application_id),
              "Grant application with ID: ${1} does not exist",
              ("1", op.grant_application_id));

    const auto& application = grant_application_service.get_grant_application(op.grant_application_id);
    const auto& research = research_service.get_research(application.research_id);

    auto expertise_tokens = expertise_token_service.get_expert_tokens_by_account_name(op.author);
    const auto& research_disciplines_relations = research_discipline_service.get_research_discipline_relations_by_research(application.research_id);

    std::set<discipline_id_type> disciplines_ids;
    for (auto& wrap : expertise_tokens)
    {
        const auto& expert_token = wrap.get();

        if (std::any_of(research_disciplines_relations.begin(), research_disciplines_relations.end(),
                        [&](const std::reference_wrapper<const research_discipline_relation_object> rel_wrap) {
                            const research_discipline_relation_object& relation = rel_wrap.get();
                            return relation.discipline_id == expert_token.discipline_id;
                        }))
        {
            // TODO: decide what to do with expertise tokens
            disciplines_ids.insert(expert_token.discipline_id);
        }
    }

    FC_ASSERT(disciplines_ids.size() != 0, 
              "${1} does not have expertise to review ${2} grant application",
              ("1", op.author)("2", op.grant_application_id));

    grant_application_service.create_grant_application_review(op.grant_application_id, op.content, op.is_positive, op.author, disciplines_ids);
}

void approve_grant_application_evaluator::do_apply(const approve_grant_application_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();
    dbs_grant_application& grant_application_service = _db.obtain_service<dbs_grant_application>();

    account_service.check_account_existence(op.approver);
    grant_application_service.check_grant_application_existence(op.grant_application_id);

    auto& grant_application = grant_application_service.get_grant_application(op.grant_application_id);
    FC_ASSERT(grant_application.status == grant_application_status::pending,
              "Grant application ${1} has ${2} status",
              ("1", grant_application.id)("2", grant_application.status));

    const auto& grant = funding_opportunity_service.get_funding_opportunity(grant_application.funding_opportunity_number);
    grant_application_service.update_grant_application_status(grant_application, grant_application_status::approved);
}

void reject_grant_application_evaluator::do_apply(const reject_grant_application_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();
    dbs_grant_application& grant_application_service = _db.obtain_service<dbs_grant_application>();

    account_service.check_account_existence(op.rejector);
    grant_application_service.check_grant_application_existence(op.grant_application_id);

    const auto& grant_application = grant_application_service.get_grant_application(op.grant_application_id);
    FC_ASSERT(grant_application.status == grant_application_status::pending,
              "Grant application ${a} has ${s} status", ("a", grant_application.id)("s", grant_application.status));

    const auto& grant = funding_opportunity_service.get_funding_opportunity(grant_application.funding_opportunity_number);
    grant_application_service.update_grant_application_status(grant_application, grant_application_status::rejected);
}

void create_asset_evaluator::do_apply(const create_asset_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_asset& asset_service = _db.obtain_service<dbs_asset>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();

    int p = std::pow(10, op.precision);
    std::string string_asset = "0." + fc::to_string(p).erase(0, 1) + " " + op.symbol;
    const asset new_asset = asset::from_string(string_asset);

    FC_ASSERT(account_service.account_exists(op.issuer), "Account ${1} does not exist", ("1", op.issuer));
    FC_ASSERT(!asset_service.asset_exists_by_symbol(new_asset.symbol), "Asset ${1} exist already", ("1", op.symbol));

    optional<std::reference_wrapper<const research_object>> tokenized_research;
    optional<percent> license_revenue_holders_share;

    for (const auto& asset_trait : op.traits)
    {
        if (asset_trait.which() == asset_trait_type::tag<research_security_token_trait>::value)
        {
            const auto& security_token_trait = asset_trait.get<research_security_token_trait>();

            const auto& research = research_service.get_research(security_token_trait.research_external_id);
            const auto& research_group = account_service.get_account(security_token_trait.research_group);

            FC_ASSERT(research_group.name == research.research_group, "Research ${1} is not owned by ${2} research group", ("1", research.external_id)("2", research_group.name));
            tokenized_research = research;
        }

        if (asset_trait.which() == asset_trait_type::tag<research_license_revenue_trait>::value)
        {
            const auto& license_revenue_trait = asset_trait.get<research_license_revenue_trait>();
            license_revenue_holders_share = license_revenue_trait.holders_share;
        }
    }
    
    asset_service.create_asset(
      op.issuer, 
      new_asset.symbol, 
      op.symbol, 
      op.precision, 
      share_type(0), 
      op.max_supply, 
      op.description, 
      tokenized_research, 
      license_revenue_holders_share
    );
}

void issue_asset_evaluator::do_apply(const issue_asset_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_asset& asset_service = _db.obtain_service<dbs_asset>();

    FC_ASSERT(account_service.account_exists(op.issuer), "Account ${1} does not exist", ("1", op.issuer));
    FC_ASSERT(account_service.account_exists(op.recipient), "Account ${1} does not exist", ("1", op.recipient));
    FC_ASSERT(asset_service.asset_exists_by_symbol(op.amount.symbol), "Asset ${1} does not exist", ("1", op.amount.symbol_name()));

    const auto& asset_o = asset_service.get_asset_by_symbol(op.amount.symbol);
    FC_ASSERT(op.issuer == asset_o.issuer, "Account ${1} is not ${2} asset issuer", ("1", op.issuer)("2", op.amount.symbol_name()));

    asset_service.issue_asset(asset_o, op.recipient, op.amount);
}

void reserve_asset_evaluator::do_apply(const reserve_asset_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();
    dbs_asset& asset_service = _db.obtain_service<dbs_asset>();

    FC_ASSERT(account_service.account_exists(op.owner), "Account ${1} does not exist", ("1", op.owner));
    FC_ASSERT(asset_service.asset_exists_by_symbol(op.amount.symbol), "Asset ${1} does not exist", ("1", op.amount.symbol_name()));
    FC_ASSERT(account_balance_service.account_balance_exists_by_owner_and_asset(op.owner, op.amount.symbol), "Asset ${1} balance does not exist for ${2} account", ("1", op.amount.symbol_name())("2", op.owner));

    const auto& asset_o = asset_service.get_asset_by_symbol(op.amount.symbol);

    asset_service.reserve_asset(asset_o, op.owner, op.amount);
}

void create_award_evaluator::do_apply(const create_award_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_award& award_service = _db.obtain_service<dbs_award>();
    dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();

    FC_ASSERT(account_service.account_exists(op.creator), 
      "Account ${1} does not exist", 
      ("1", op.creator));

    FC_ASSERT(account_service.account_exists(op.awardee),
      "Account ${1} does not exist",
      ("1", op.awardee));

    FC_ASSERT(research_service.research_exists(op.research_external_id),
      "Research ID:${1} does not exist",
      ("1", op.research_external_id));

    FC_ASSERT(funding_opportunity_service.funding_opportunity_exists(op.funding_opportunity_number),
      "Funding opportunity with ID ${1} does not exist", 
      ("1", op.funding_opportunity_number));

    const auto& foa = funding_opportunity_service.get_funding_opportunity(op.funding_opportunity_number);

    FC_ASSERT(op.award.symbol == foa.amount.symbol,
      "Award asset ${1} should be the same as funding opportunity asset ${2}", 
      ("1", op.award.symbol_name())("2", foa.amount.symbol_name()));

    FC_ASSERT(foa.current_supply >= op.award,
      "Funding opportunity does not have enough funds to award. Availabale amount:${1}, Required amount:${2}.",
      ("1", foa.current_supply.amount)("2", op.award.amount));

    const auto& foa_organization = account_service.get_account(foa.organization_id._id);

    FC_ASSERT(op.university_external_id != foa_organization.name,
      "Funding opportunity agency ${1} is not allowed as intermediary",
      ("1", op.university_external_id));

    FC_ASSERT(op.award.symbol == foa.amount.symbol,
      "Award asset ${1} does not match funding opportunity asset ${2}.",
      ("1", op.award.symbol_name())("2", foa.amount.symbol_name()));

    FC_ASSERT(account_balance_service.account_balance_exists_by_owner_and_asset(op.creator, op.award.symbol),
              "Account balance for ${1} for asset ${2} does not exist", ("1", op.creator)("2", op.award.symbol_name()));

    const auto& university = account_service.get_account(op.university_external_id);
    const auto& research = research_service.get_research(op.research_external_id);

    const asset university_fee = asset((op.award.amount * op.university_overhead.amount) / DEIP_100_PERCENT, op.award.symbol);

    const asset top_subawards_amount = std::accumulate(
        op.subawardees.begin(), op.subawardees.end(), asset(0, op.award.symbol),
        [&](asset acc, const subawardee_type& s) { return s.source == op.awardee ? acc + s.subaward : acc; });

    std::map<account_name_type, asset> awards_map = { { op.awardee, op.award - university_fee - top_subawards_amount } };
    for (auto& subaward : op.subawardees)
    {
        const asset subsequent_subawards_amount = std::accumulate(
            op.subawardees.begin(), op.subawardees.end(), asset(0, op.award.symbol), 
            [&](asset acc, const subawardee_type& s) {
                return s.source == subaward.subawardee ? acc + s.subaward : acc;
            });

        awards_map[subaward.subawardee] = subaward.subaward - subsequent_subawards_amount;
    }

    const auto& award = award_service.create_award(
      op.funding_opportunity_number, 
      op.award_number, 
      op.awardee,
      op.award, 
      university.id,
      university.name,
      op.university_overhead,
      op.creator,
      award_status::pending);

    award_service.create_award_recipient(
      award.award_number,
      award.award_number,
      award.funding_opportunity_number,
      award.awardee,
      account_name_type(),
      awards_map[award.awardee],
      research.id,
      research.external_id,
      award_recipient_status::unconfirmed);

    for (auto& subaward : op.subawardees)
    {
        FC_ASSERT(account_service.account_exists(subaward.subawardee),
          "Account ${1} does not exist",
          ("1", subaward.subawardee));

        FC_ASSERT(research_service.research_exists(subaward.research_external_id),
          "Research ${1} does not exist",
          ("1", subaward.research_external_id));

        const auto& research = research_service.get_research(subaward.research_external_id);

        award_service.create_award_recipient(
          award.award_number,
          subaward.subaward_number,
          award.funding_opportunity_number,
          subaward.subawardee,
          subaward.source,
          awards_map[subaward.subawardee],
          research.id,
          research.external_id,
          award_recipient_status::unconfirmed);
    }
}

void approve_award_evaluator::do_apply(const approve_award_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_award& award_service = _db.obtain_service<dbs_award>();
    dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();

    FC_ASSERT(account_service.account_exists(op.approver),
      "Account ${1} does not exist",
      ("1", op.approver));

    FC_ASSERT(award_service.award_exists(op.award_number),
      "Award ${1} does not exist",
      ("1", op.award_number));

    const auto& award = award_service.get_award(op.award_number);
    const auto& foa = funding_opportunity_service.get_funding_opportunity(award.funding_opportunity_number);

    FC_ASSERT(std::any_of(foa.officers.begin(), foa.officers.end(),
      [&](const account_name_type& officer) { return officer == op.approver; }),
      "Account ${1} is not program officer",
      ("1", op.approver));

    FC_ASSERT(award.status == static_cast<uint16_t>(award_status::pending),
      "Only pending award can be approved. The current status is: ${1}",
      ("1", award.status));

    const auto& university = account_service.get_account(account_id_type(award.university_id._id));
    const asset university_fee = asset(((award.amount.amount * share_type(award.university_overhead.amount)) / DEIP_100_PERCENT), award.amount.symbol);
    account_balance_service.adjust_account_balance(university.name, university_fee);

    auto awardees = award_service.get_award_recipients_by_award(op.award_number);

    for (auto& wrap : awardees)
    {
        const award_recipient_object& award_recipient = wrap.get();
        account_balance_service.adjust_account_balance(award_recipient.awardee, award_recipient.total_amount);
        award_service.update_award_recipient_status(award_recipient, award_recipient_status::confirmed);
    }

    funding_opportunity_service.adjust_funding_opportunity_supply(foa.id, -award.amount);
    award_service.update_award_status(award, award_status::approved);
}

void reject_award_evaluator::do_apply(const reject_award_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_award& award_service = _db.obtain_service<dbs_award>();
    dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();

    FC_ASSERT(account_service.account_exists(op.rejector),
      "Account ${1} does not exist",
      ("1", op.rejector));

    FC_ASSERT(award_service.award_exists(op.award_number),
      "Award ${1} does not exist",
      ("1", op.award_number));

    const auto& award = award_service.get_award(op.award_number);
    const auto& foa = funding_opportunity_service.get_funding_opportunity(award.funding_opportunity_number);

    FC_ASSERT(std::any_of(foa.officers.begin(), foa.officers.end(),
      [&](const account_name_type& officer) { return officer == op.rejector; }),
      "Account ${1} is not program officer",
      ("1", op.rejector));

    FC_ASSERT(award.status == static_cast<uint16_t>(award_status::pending),
      "Only pending award can be rejected. The current status is: ${1}",
      ("1", award.status));

    auto awardees = award_service.get_award_recipients_by_award(op.award_number);

    for (auto& wrap : awardees)
    {
        const auto& award_recipient = wrap.get();
        award_service.update_award_recipient_status(award_recipient, award_recipient_status::canceled);
    }

    award_service.update_award_status(award, award_status::rejected);
}

void create_award_withdrawal_request_evaluator::do_apply(const create_award_withdrawal_request_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_award& award_service = _db.obtain_service<dbs_award>();

    const external_id_type payment_number = op.payment_number;
    const external_id_type award_number = op.award_number;
    const external_id_type subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;

    FC_ASSERT(account_service.account_exists(op.requester),
      "Account ${1} does not exist",
      ("1", op.requester));

    FC_ASSERT(award_service.award_exists(award_number),
      "Award ${1} does not exist",
      ("1", award_number));

    const auto& award = award_service.get_award(award_number);

    FC_ASSERT(op.amount.symbol == award.amount.symbol,
      "Award withdrawal request asset ${1} should be the same as award asset ${2}",
      ("1", op.amount.symbol_name())("2", award.amount.symbol_name()));

    FC_ASSERT(award.status == static_cast<uint16_t>(award_status::approved),
      "Award ${1} is not approved",
      ("1", award_number));

    FC_ASSERT(award_service.award_recipient_exists(award_number, subaward_number),
      "Award recipient ${1}:${2} does not exist",
      ("1", award_number)("2", subaward_number));

    const auto& award_recipient = award_service.get_award_recipient(award_number, subaward_number);

    FC_ASSERT(award_recipient.awardee == op.requester,
      "Requester ${1} is not the award recipient ${2}. Make sure you specified the correct Subaward number",
      ("1", op.requester)("2", award_recipient.awardee));

    FC_ASSERT(op.amount.symbol == award_recipient.total_amount.symbol,
      "Withdrawal asset ${1} does not match the award asset ${2}.",
      ("1", op.amount.symbol_name())("2", award_recipient.total_amount.symbol_name()));

    FC_ASSERT(award_recipient.total_amount - award_recipient.total_expenses >= op.amount,
      "Not enough funds. Requested amount: ${1} Available amount: ${2}",
      ("1", op.amount.to_string())("2", (award_recipient.total_amount - award_recipient.total_expenses).to_string()));

    const auto now = _db.head_block_time();

    award_service.create_award_withdrawal_request(
      payment_number,
      award_number,
      subaward_number,
      op.requester,
      op.amount,
      op.description,
      now,
      op.attachment
    );
}

void certify_award_withdrawal_request_evaluator::do_apply(const certify_award_withdrawal_request_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_award& award_service = _db.obtain_service<dbs_award>();

    const external_id_type payment_number = op.payment_number;
    const external_id_type award_number = op.award_number;
    const external_id_type subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;

    FC_ASSERT(account_service.account_exists(op.certifier),
      "Account ${1} does not exist",
      ("1", op.certifier));

    FC_ASSERT(award_service.award_withdrawal_request_exists(award_number, payment_number),
      "Award withdrawal request ${1}:${2} does not exist",
      ("1", award_number)("2", payment_number));

    const auto& withdrawal = award_service.get_award_withdrawal_request(award_number, payment_number);
    const auto& award_recipient = award_service.get_award_recipient(award_number, subaward_number);

    FC_ASSERT(award_recipient.awardee == withdrawal.requester,
      "Requester ${1} is not the award recipient ${2}. Make sure you specified the correct Subaward number",
      ("1", withdrawal.requester)("2", award_recipient.awardee));

    FC_ASSERT(withdrawal.status == static_cast<uint16_t>(award_withdrawal_request_status::pending),
      "Only pending award withdrawal request can be certified. The current status is: ${1}",
      ("1", withdrawal.status));

    const auto& award = award_service.get_award(award_number);

    award_service.update_award_withdrawal_request(withdrawal, award_withdrawal_request_status::certified);
}

void approve_award_withdrawal_request_evaluator::do_apply(const approve_award_withdrawal_request_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_award& award_service = _db.obtain_service<dbs_award>();
    dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();

    const external_id_type payment_number = op.payment_number;
    const external_id_type award_number = op.award_number;
    const external_id_type subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;

    FC_ASSERT(account_service.account_exists(op.approver),
      "Account ${1} does not exist",
      ("1", op.approver));

    FC_ASSERT(award_service.award_withdrawal_request_exists(award_number, payment_number),
      "Award withdrawal request ${1}:${2} does not exist",
      ("1", award_number)("2", payment_number));

    const auto& withdrawal = award_service.get_award_withdrawal_request(award_number, payment_number);
    const auto& award_recipient = award_service.get_award_recipient(award_number, subaward_number);

    FC_ASSERT(award_recipient.awardee == withdrawal.requester,
      "Requester ${1} is not the award recipient ${2}. Make sure you specified the correct Subaward number",
      ("1", withdrawal.requester)("2", award_recipient.awardee));

    FC_ASSERT(withdrawal.status == static_cast<uint16_t>(award_withdrawal_request_status::certified),
      "Not certified withdrawal request can not be approved. The current status is: ${1}",
      ("1", withdrawal.status));

    const auto& foa = funding_opportunity_service.get_funding_opportunity(award_recipient.funding_opportunity_number);

    // FC_ASSERT(std::any_of(foa.officers.begin(), foa.officers.end(),
    //   [&](const account_name_type& officer) { return officer == op.approver; }),
    //   "Account ${1} is not Funding Opportunity officer",
    //   ("1", op.approver));

    award_service.update_award_withdrawal_request(withdrawal, award_withdrawal_request_status::approved);
}

void reject_award_withdrawal_request_evaluator::do_apply(const reject_award_withdrawal_request_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_award& award_service = _db.obtain_service<dbs_award>();
    dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();

    const external_id_type payment_number = op.payment_number;
    const external_id_type award_number = op.award_number;
    const external_id_type subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;

    FC_ASSERT(account_service.account_exists(op.rejector),
      "Account ${1} does not exist",
      ("1", op.rejector));

    FC_ASSERT(award_service.award_withdrawal_request_exists(award_number, payment_number),
      "Award withdrawal request ${1}:${2} does not exist",
      ("1", award_number)("2", payment_number));

    const auto& withdrawal = award_service.get_award_withdrawal_request(award_number, payment_number);
    const auto& award_recipient = award_service.get_award_recipient(award_number, subaward_number);

    FC_ASSERT(award_recipient.awardee == withdrawal.requester,
      "Requester ${1} is not the award recipient ${2}. Make sure you specified the correct Subaward number",
      ("1", withdrawal.requester)("2", award_recipient.awardee));

    FC_ASSERT(withdrawal.status == static_cast<uint16_t>(award_withdrawal_request_status::pending) ||
              withdrawal.status == static_cast<uint16_t>(award_withdrawal_request_status::certified),
      "Only pending or certified award withdrawal request can be rejected. The current status is: ${1}",
      ("1", withdrawal.status));

    const auto& award = award_service.get_award(award_number);
    const auto& foa = funding_opportunity_service.get_funding_opportunity(award_recipient.funding_opportunity_number);

    if (withdrawal.status == static_cast<uint16_t>(award_withdrawal_request_status::pending))
    {

    }
    else if (withdrawal.status == static_cast<uint16_t>(award_withdrawal_request_status::certified))
    {

    }
    else 
    {
        FC_ASSERT(false, "Unexpected status");
    }

    award_service.update_award_withdrawal_request(withdrawal, award_withdrawal_request_status::rejected);
}

void pay_award_withdrawal_request_evaluator::do_apply(const pay_award_withdrawal_request_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();
    dbs_asset& asset_service = _db.obtain_service<dbs_asset>();
    dbs_award& award_service = _db.obtain_service<dbs_award>();
    dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();

    const external_id_type payment_number = op.payment_number;
    const external_id_type award_number = op.award_number;
    const external_id_type subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;

    FC_ASSERT(account_service.account_exists(op.payer),
      "Account ${1} does not exist",
      ("1", op.payer));

    FC_ASSERT(award_service.award_withdrawal_request_exists(award_number, payment_number),
      "Award withdrawal request ${1}:${2} does not exist",
      ("1", award_number)("2", payment_number));

    const auto& withdrawal = award_service.get_award_withdrawal_request(award_number, payment_number);
    const auto& award_recipient = award_service.get_award_recipient(award_number, subaward_number);

    FC_ASSERT(award_recipient.awardee == withdrawal.requester,
      "Requester ${1} is not the award recipient ${2}. Make sure you specified the correct Subaward number",
      ("1", withdrawal.requester)("2", award_recipient.awardee));

    FC_ASSERT(withdrawal.status == static_cast<uint16_t>(award_withdrawal_request_status::approved),
      "Only approved award withdrawal request can be paid. The current status is: ${1}",
      ("1", withdrawal.status));

    const auto& foa = funding_opportunity_service.get_funding_opportunity(award_recipient.funding_opportunity_number);
    const auto& treasury = account_service.get_account(account_id_type(foa.treasury_id));

    FC_ASSERT(award_recipient.total_amount - award_recipient.total_expenses >= withdrawal.amount, 
      "Not enough funds to process the payment. Requested ${1}, Available: ${2} ", 
      ("1", withdrawal.amount)("2", award_recipient.total_amount - award_recipient.total_expenses));

    const auto& grant_asset = asset_service.get_asset_by_symbol(withdrawal.amount.symbol);

    award_service.adjust_expenses(award_recipient.id, withdrawal.amount);
    asset_service.adjust_asset_current_supply(grant_asset, -withdrawal.amount); // burn grant tokens

    const price rate = price(asset(1, DEIP_USD_SYMBOL), asset(1, withdrawal.amount.symbol));
    const asset payout = withdrawal.amount * rate;

    account_balance_service.adjust_account_balance(withdrawal.requester, payout); // imitation of acquiring api call
    account_balance_service.adjust_account_balance(treasury.name, -payout); // imitation of acquiring api call

    award_service.update_award_withdrawal_request(withdrawal, award_withdrawal_request_status::paid);
}

void create_research_nda_evaluator::do_apply(const create_research_nda_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_nda_contract& nda_contracts_service = _db.obtain_service<dbs_nda_contract>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    
    fc::time_point_sec block_time = _db.head_block_time();
    fc::time_point_sec start_time = op.start_time.valid() ? *op.start_time : block_time;
    FC_ASSERT(op.researches.size() == 1, "Research must be specifed"); // TEMP
    const external_id_type& research_external_id = *(op.researches.begin());

    FC_ASSERT(research_service.research_exists(research_external_id),
      "Research ${1} does not exist",
      ("1", research_external_id));

    const auto& research = research_service.get_research(research_external_id);

    FC_ASSERT(op.parties.find(research.research_group) != op.parties.end(), 
      "Research group ${1} of the research ${2} must be specified as a party", 
      ("1", research.research_group)("2", research.external_id));

    FC_ASSERT(start_time >= block_time,
      "NDA start date ${1} must be later or equal to the current moment ${2}",
      ("1", start_time)("2", block_time));

    FC_ASSERT(op.end_time > block_time,
      "NDA end date ${1} must be later the current moment ${2}",
      ("1", op.end_time)("2", block_time));

    FC_ASSERT(op.end_time > start_time,
      "NDA start date ${1} must be less than end date ${2}",
      ("1", start_time)("2", op.end_time));

    nda_contracts_service.create_research_nda(op.external_id,
                                              op.creator,
                                              op.parties,
                                              op.description,
                                              research_external_id,
                                              start_time,
                                              op.end_time);
}

void sign_nda_contract_evaluator::do_apply(const sign_nda_contract_operation& op)
{
    // DEPRECATED
}

void decline_nda_contract_evaluator::do_apply(const decline_nda_contract_operation& op)
{
    // DEPRECATED
}

void close_nda_contract_evaluator::do_apply(const close_nda_contract_operation& op)
{
    // DEPRECATED
}

void create_nda_content_access_request_evaluator::do_apply(const create_nda_content_access_request_operation& op)
{
    dbs_nda_contract& nda_contracts_service = _db.obtain_service<dbs_nda_contract>();
    dbs_nda_contract_requests& nda_contract_requests_service = _db.obtain_service<dbs_nda_contract_requests>();

    const auto& block_time = _db.head_block_time();

    FC_ASSERT(nda_contracts_service.research_nda_exists(op.nda_external_id),
      "NDA contract ${1} does not exist",
      ("1", op.nda_external_id));

    const auto& nda = nda_contracts_service.get_research_nda(op.nda_external_id);

    FC_ASSERT(nda.start_time <= block_time,
      "NDA contract is not active yet and will be operational at ${1}",
      ("1", nda.start_time));

    nda_contract_requests_service.create_content_access_request(op.external_id, op.nda_external_id, op.requester, op.encrypted_payload_hash, op.encrypted_payload_iv);
}

void fulfill_nda_content_access_request_evaluator::do_apply(const fulfill_nda_content_access_request_operation& op)
{
    dbs_nda_contract& nda_contracts_service = _db.obtain_service<dbs_nda_contract>();
    dbs_nda_contract_requests& nda_contract_requests_service = _db.obtain_service<dbs_nda_contract_requests>();

    FC_ASSERT(nda_contract_requests_service.content_access_request_exists(op.external_id),
      "Request ${1} does not exist",
      ("1", op.external_id));

    const auto& request = nda_contract_requests_service.get_content_access_request(op.external_id);

    FC_ASSERT(nda_contracts_service.research_nda_exists(request.nda_external_id),
      "NDA contract ${1} does not exist",
      ("1", request.nda_external_id));

    const auto& nda = nda_contracts_service.get_research_nda(request.nda_external_id);

    nda_contract_requests_service.fulfill_content_access_request(request, op.encrypted_payload_encryption_key, op.proof_of_encrypted_payload_encryption_key);
}

void join_research_contract_evaluator::do_apply(const join_research_contract_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_service = _db.obtain_service<dbs_research>();

    FC_ASSERT(account_service.account_exists(op.member), "Account ${1} does not exist", ("1", op.member));
    FC_ASSERT(account_service.account_exists(op.research_group), "Research group ${1} does not exist", ("1", op.research_group));

    const auto& research_account = account_service.get_account(op.research_group);
    // TODO: add check to verify existing membership
    account_service.add_to_active_authority(research_account, op.member);
    // TODO: set reward share and other conditions
}

void leave_research_contract_evaluator::do_apply(const leave_research_contract_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_service = _db.obtain_service<dbs_research>();

    FC_ASSERT(account_service.account_exists(op.member), "Account ${1} does not exist", ("1", op.member));
    FC_ASSERT(account_service.account_exists(op.research_group), "Research group ${1} does not exist", ("1", op.research_group));

    const auto& research_account = account_service.get_account(op.research_group);
    account_service.remove_from_active_authority(research_account, op.member);
    // TODO: cancel reward share and other conditions
}

void create_research_evaluator::do_apply(const create_research_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_service = _db.obtain_service<dbs_research>();
    auto& discipline_service =_db.obtain_service<dbs_discipline>();
    auto& dgp_service = _db.obtain_service<dbs_dynamic_global_properties>();

    const auto& block_time = _db.head_block_time();
    const auto& dup_guard = duplicated_entity_guard(dgp_service);
    dup_guard(op);

    const auto& account = account_service.get_account(op.account);

    std::set<discipline_id_type> disciplines;
    for (const auto& external_id : op.disciplines)
    {
        const auto& discipline_opt = discipline_service.get_discipline_if_exists(external_id);
        FC_ASSERT(discipline_opt.valid(), "Discipline ${1} does not exist", ("1", external_id));
        const discipline_object& discipline = *discipline_opt;
        disciplines.insert(discipline_id_type(discipline.id));
    }

    const bool& is_finished = false;
    const bool& is_default = false;
    const bool& is_private = op.is_private;

    research_service.create_research(
      account, 
      op.external_id, 
      op.description, 
      disciplines,
      is_private,
      is_finished,
      is_default,
      block_time
    );
}

void create_research_content_evaluator::do_apply(const create_research_content_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_service = _db.obtain_service<dbs_research>();
    auto& research_content_service = _db.obtain_service<dbs_research_content>();
    auto& research_discipline_relation_service = _db.obtain_service<dbs_research_discipline_relation>();
    auto& expertise_contributions_service = _db.obtain_service<dbs_expertise_contribution>();
    auto& dgp_service = _db.obtain_service<dbs_dynamic_global_properties>();
    const auto& block_time = _db.head_block_time();

    const auto& dup_guard = duplicated_entity_guard(dgp_service);
    dup_guard(op);

    FC_ASSERT(account_service.account_exists(op.research_group),
      "Account(creator) ${1} does not exist",
      ("1", op.research_group));

    const auto& research_group = account_service.get_account(op.research_group);

    FC_ASSERT(research_service.research_exists(op.research_external_id),
      "Research with external id: ${1} does not exist",
      ("1", op.research_external_id));

    const auto& research = research_service.get_research(op.research_external_id);

    FC_ASSERT(research.research_group == research_group.name,
      "Research ${1} does not belong to research group ${2}",
      ("1", research.external_id)("2", research_group.name));

    FC_ASSERT(!research.is_finished,
      "The research ${1} has been finished already",
      ("1", research.description));

    const auto& research_content = research_content_service.create_research_content(
      research,
      op.external_id,
      op.description,
      op.content,
      static_cast<research_content_type>(op.type),
      op.authors,
      op.references,
      block_time);

    for (external_id_type id : op.references)
    {
        const auto& ref = research_content_service.get_research_content(id);
        _db.push_virtual_operation(research_content_reference_history_operation(
          research_content.id._id,
          research_content.external_id,
          research_content.research_id._id,
          research_content.research_external_id,
          fc::to_string(research_content.content),
          ref.id._id,
          ref.external_id,
          ref.research_id._id,
          ref.research_external_id,
          fc::to_string(ref.content))
        );
    }

    const auto previous_research_content_eci = research_content_service.get_eci_evaluation(research_content.id);
    const auto previous_research_eci = research_service.get_eci_evaluation(research.id);

    const research_content_object& updated_research_content = research_content_service.update_eci_evaluation(research_content.id);
    const research_object& updated_research = research_service.update_eci_evaluation(research.id);

    const auto& relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research.id);
    for (auto& wrap : relations)
    {
        const auto& rel = wrap.get();

        flat_map<uint16_t, assessment_criteria_value> assessment_criterias;

        const eci_diff research_content_eci_diff = eci_diff(
          previous_research_content_eci.at(rel.discipline_id),
          updated_research_content.eci_per_discipline.at(rel.discipline_id),
          block_time,
          static_cast<uint16_t>(expertise_contribution_type::publication),
          updated_research_content.id._id,
          assessment_criterias
        );

        expertise_contributions_service.adjust_expertise_contribution(
          rel.discipline_id,
          research.id,
          research_content.id,
          research_content_eci_diff
        );

        _db.push_virtual_operation(research_content_eci_history_operation(
            updated_research_content.id._id,
            rel.discipline_id._id,
            research_content_eci_diff)
        );

        const eci_diff research_eci_diff = eci_diff(
          previous_research_eci.at(rel.discipline_id),
          updated_research.eci_per_discipline.at(rel.discipline_id),
          block_time,
          static_cast<uint16_t>(expertise_contribution_type::publication),
          updated_research_content.id._id,
          assessment_criterias
        );

        _db.push_virtual_operation(research_eci_history_operation(
            research.id._id,
            rel.discipline_id._id,
            research_eci_diff)
        );
    }

}

void create_research_token_sale_evaluator::do_apply(const create_research_token_sale_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_service = _db.obtain_service<dbs_research>();
    auto& research_token_sale_service = _db.obtain_service<dbs_research_token_sale>();
    auto& asset_service = _db.obtain_service<dbs_asset>();
    auto& account_balance_service = _db.obtain_service<dbs_account_balance>();
    auto& dgp_service = _db.obtain_service<dbs_dynamic_global_properties>();

    const auto& dup_guard = duplicated_entity_guard(dgp_service);
    dup_guard(op);

    const auto& block_time = _db.head_block_time();
    FC_ASSERT(op.start_time >= block_time);

    FC_ASSERT(account_service.account_exists(op.research_group),
      "Account ${1} does not exist",
      ("1", op.research_group));

    const auto& research_group = account_service.get_account(op.research_group);

    FC_ASSERT(research_service.research_exists(op.research_external_id),
      "Research with external id: ${1} does not exist",
      ("1", op.research_external_id));

    const auto& research = research_service.get_research(op.research_external_id);

    for (const auto& security_token_on_sale : op.security_tokens_on_sale)
    {
        const auto& asset_o = asset_service.get_asset_by_symbol(security_token_on_sale.symbol);
        FC_ASSERT(static_cast<asset_type>(asset_o.type) == asset_type::research_security_token, "Asset ${1} is not a research security token",
            ("1", security_token_on_sale.symbol_name()));

        FC_ASSERT(std::count_if(research.security_tokens.begin(), research.security_tokens.end(),
            [&](const asset& a) { return a.symbol == security_token_on_sale.symbol; }) != 0, "Research ${1} is not tokenized with ${2} security token", ("1", research.external_id)("2", security_token_on_sale.symbol_name()));

        const auto& security_token_balance_opt = account_balance_service.get_account_balance_by_owner_and_asset_if_exists(research.research_group, security_token_on_sale.symbol);
        FC_ASSERT(security_token_balance_opt.valid(), "Research group ${1} does not have balance for security token ${2} ", ("1", research.research_group)("2", security_token_on_sale.symbol_name()));

        const account_balance_object& security_token_balance = *security_token_balance_opt;
        FC_ASSERT(security_token_balance.to_asset() >= security_token_on_sale, "Research group ${1} security token balance (${2}) is not enough", ("1", research.research_group)("2", security_token_balance.to_asset()));
    }

    FC_ASSERT(research.research_group == research_group.name, "Research ${1} does not belong to research group ${2}",
      ("1", op.research_external_id)("2", research_group.name));

    const auto& active_research_token_sales = research_token_sale_service.get_by_research_id_and_status(research.id, research_token_sale_status::active);
    const auto& inactive_research_token_sales = research_token_sale_service.get_by_research_id_and_status(research.id, research_token_sale_status::inactive);

    FC_ASSERT(inactive_research_token_sales.size() == 0 && active_research_token_sales.size() == 0, "Research ${1} has a scheduled token sale already", 
      ("1", op.research_external_id));

    for (const auto& security_token_on_sale : op.security_tokens_on_sale)
    {
        account_balance_service.freeze_account_balance(research.research_group, security_token_on_sale);
    }

    research_token_sale_service.create_research_token_sale(
      op.external_id,
      research,
      op.security_tokens_on_sale,
      op.start_time,
      op.end_time,
      op.soft_cap,
      op.hard_cap
    );
}

void update_research_evaluator::do_apply(const update_research_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_service = _db.obtain_service<dbs_research>();

    FC_ASSERT(account_service.account_exists(op.account),
      "Account(creator) ${1} does not exist",
      ("1", op.account));

    const auto& account = account_service.get_account(op.account);

    FC_ASSERT(research_service.research_exists(op.external_id),
      "Research with external id: ${1} does not exist",
      ("1", op.external_id));

    const auto& research = research_service.get_research(op.external_id);

    FC_ASSERT(research.research_group == account.name,
      "Research ${1} does not belong to research group ${2}",
      ("1", research.external_id)("2", account.name));

    std::string description = op.description.valid() ? *op.description : fc::to_string(research.description);
    bool is_private = op.is_private.valid() ? *op.is_private : research.is_private;

    research_service.update_research(
      research, 
      description,
      is_private,
      op.update_extensions
    );
}

void create_assessment_evaluator::do_apply(const create_assessment_operation& op)
{
    
}

void create_research_license_evaluator::do_apply(const create_research_license_operation& op)
{
    auto& dgp_service = _db.obtain_service<dbs_dynamic_global_properties>();
    auto& research_service = _db.obtain_service<dbs_research>();
    auto& asset_service = _db.obtain_service<dbs_asset>();
    auto& research_license_service = _db.obtain_service<dbs_research_license>();
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& account_balance_service = _db.obtain_service<dbs_account_balance>();

    const auto& dup_guard = duplicated_entity_guard(dgp_service);
    dup_guard(op);

    FC_ASSERT(account_service.account_exists(op.licenser), "Account ${1} does not exist", ("1", op.licenser));
    FC_ASSERT(account_service.account_exists(op.licensee), "Account ${1} does not exist", ("1", op.licensee));

    const auto& research_group = account_service.get_account(op.licenser);
    const auto& research = research_service.get_research(op.research_external_id);
    const auto& now = _db.head_block_time();

    FC_ASSERT(research_group.name == research.research_group, "Research ${1} is not owned by ${2} research group", ("1", research.external_id)("2", research_group.name));

    if (op.license_conditions.which() == license_agreement_types::tag<licensing_fee_type>::value)
    {
        const auto& fee_model = op.license_conditions.get<licensing_fee_type>();
        const auto& research_license = research_license_service.create_research_license(research, op.external_id, op.licensee, fee_model.terms, fee_model.expiration_time, fee_model.fee);

        if (fee_model.expiration_time.valid())
        {
            const auto& expiration_time = *fee_model.expiration_time;
            FC_ASSERT(now < expiration_time, "Research license is expired on creation");
        }

        if (research_license.fee.valid())
        {
            const auto& fee = *research_license.fee;
            asset total_revenue = asset(0, fee.symbol);

            FC_ASSERT(account_balance_service.account_balance_exists_by_owner_and_asset(op.licensee, fee.symbol), "${1} asset balance does not exist for ${2} account", ("1", fee.symbol_name())("2", op.licensee));
            const auto& licensee_balance = account_balance_service.get_account_balance_by_owner_and_asset(op.licensee, fee.symbol);
            FC_ASSERT(licensee_balance.to_asset() >= fee, "Account ${1} balance is not enough.", ("1", op.licensee));
            
            optional<external_id_type> tokenized_research;
            tokenized_research = op.research_external_id;

            const auto& beneficiary_tokens = asset_service.get_assets_by_tokenize_research(tokenized_research);

            std::map<string, asset> beneficiary_shares;
            for (const asset_object& beneficiary_token : beneficiary_tokens)
            {
                const auto& share_percent = beneficiary_token.license_revenue_holders_share.valid() 
                  ? *beneficiary_token.license_revenue_holders_share
                  : percent(0);

                const asset share = util::calculate_share(fee, share_percent);
                const string sym = fc::to_string(beneficiary_token.string_symbol);
                beneficiary_shares.insert(std::make_pair(sym, share));
            }

            for (const auto& beneficiary_share : beneficiary_shares)
            {
                const auto& security_token = asset_service.get_asset_by_string_symbol(beneficiary_share.first);
                const auto& security_token_balances = account_balance_service.get_accounts_balances_by_symbol(security_token.symbol);
                const auto& beneficiary_revenue = beneficiary_share.second;

                for (const account_balance_object& security_token_balance : security_token_balances)
                {
                    const asset revenue = util::calculate_share(beneficiary_revenue, security_token_balance.amount, security_token.current_supply);
                    const auto& account_revenue_balance = account_balance_service.adjust_account_balance(security_token_balance.owner, revenue);
                    
                    _db.push_virtual_operation(account_revenue_income_history_operation(
                        security_token_balance.owner, 
                        security_token_balance.to_asset(),
                        revenue,
                        now)
                    );

                    total_revenue += revenue;
                }
            }

            FC_ASSERT(total_revenue <= fee, "Total revenue amount ${1} is more than fee amount ${2}", ("1", total_revenue)("2", fee));

            if (total_revenue < fee)
            {
                const asset rest = fee - total_revenue;
                account_balance_service.adjust_account_balance(research.research_group, rest);
            }

            account_balance_service.adjust_account_balance(op.licensee, -fee);
        }
    }
    else 
    {
        FC_ASSERT(false, "Unknown research license type");
    }
}

void create_contract_agreement_evaluator::do_apply(const create_contract_agreement_operation& op)
{
    auto& dgp_service = _db.obtain_service<dbs_dynamic_global_properties>();
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& contract_agreement_service = _db.obtain_service<dbs_contract_agreement>();

    const auto& dup_guard = duplicated_entity_guard(dgp_service);
    dup_guard(op);

    FC_ASSERT(account_service.account_exists(op.creator), "Account ${1} does not exist", ("1", op.creator));
    for (const auto& party: op.parties)
    {
        FC_ASSERT(account_service.account_exists(party), "Account ${1} does not exist", ("1", party));
    }

    const auto block_time = _db.head_block_time();
    const auto start_time = op.start_time.valid() ? *op.start_time : block_time;

    FC_ASSERT(start_time >= block_time,
      "Start time ${1} must be later or equal to the current moment ${2}",
      ("1", start_time)("2", block_time));

    if (op.end_time.valid())
    {
        FC_ASSERT(*op.end_time > start_time,
          "Start time ${1} must be less than end time ${2}",
          ("1", start_time)("2", *op.end_time));
    }

    contract_agreement_service.create(op.external_id,
                                      op.creator,
                                      op.parties,
                                      op.hash,
                                      start_time,
                                      op.end_time);
}

void accept_contract_agreement_evaluator::do_apply(const accept_contract_agreement_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& contract_agreement_service = _db.obtain_service<dbs_contract_agreement>();

    FC_ASSERT(account_service.account_exists(op.party), "Account ${1} does not exist", ("1", op.party));

    const auto& contract_opt_ref = contract_agreement_service.get_if_exists(op.external_id);
    FC_ASSERT(contract_opt_ref.valid(), "Contract agreement ${1} does not exist", ("1", op.external_id));

    const auto& contract = contract_opt_ref->get();
    const auto itEnd = contract.parties.end();
    bool found = false;
    for (auto it = contract.parties.begin(); it != itEnd; ++it)
    {
        FC_ASSERT(it->second != static_cast<uint8_t>(acceptance_status::Rejected),
                  "Contract agreement ${1} is rejected by ${2}",
                  ("1", op.external_id)("2", it->first));

        if (it->first == op.party)
        {
            found = true;
            FC_ASSERT(it->second == static_cast<uint8_t>(acceptance_status::NotAccepted),
                      "Contract agreement ${1} is already accepted by ${2}",
                      ("1", op.external_id)("2", op.party));
        }
    }

    FC_ASSERT(found,
              "Party ${2} is not listed in contract agreement ${1}",
              ("1", op.external_id)("2", op.party));

    const auto block_time = _db.head_block_time();
    FC_ASSERT(contract.start_time <= block_time,
              "Contract agreement ${1} is not active. (start_time = ${2}, block_time = ${3}",
              ("1", op.external_id)("2", contract.start_time)("3", block_time));

    if (contract.end_time.valid())
    {
        FC_ASSERT(block_time < *contract.end_time,
                  "Contract agreement ${1} is expired",
                  ("1", op.external_id));
    }

    contract_agreement_service.accept_by(contract, op.party);
}

void reject_contract_agreement_evaluator::do_apply(const reject_contract_agreement_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& contract_agreement_service = _db.obtain_service<dbs_contract_agreement>();

    FC_ASSERT(account_service.account_exists(op.party), "Account ${1} does not exist", ("1", op.party));

    const auto& contract_opt_ref = contract_agreement_service.get_if_exists(op.external_id);
    FC_ASSERT(contract_opt_ref.valid(), "Contract agreement ${1} does not exist", ("1", op.external_id));

    const auto& contract = contract_opt_ref->get();
    const auto itEnd = contract.parties.end();
    bool found = false;
    for (auto it = contract.parties.begin(); it != itEnd; ++it)
    {
        FC_ASSERT(it->second != static_cast<uint8_t>(acceptance_status::Rejected),
                  "Contract agreement ${1} is rejected by ${2}",
                  ("1", op.external_id)("2", it->first));

        if (it->first == op.party)
        {
            found = true;
            FC_ASSERT(it->second == static_cast<uint8_t>(acceptance_status::NotAccepted),
                      "Contract agreement ${1} is already accepted by ${2}",
                      ("1", op.external_id)("2", op.party));
        }
    }

    FC_ASSERT(found,
              "Party ${2} is not listed in contract agreement ${1}",
              ("1", op.external_id)("2", op.party));

    const auto block_time = _db.head_block_time();
    FC_ASSERT(contract.start_time <= block_time,
              "Contract agreement ${1} is not active. (start_time = ${2}, block_time = ${3}",
              ("1", op.external_id)("2", contract.start_time)("3", block_time));

    if (contract.end_time.valid())
    {
        FC_ASSERT(block_time < *contract.end_time,
                  "Contract agreement ${1} is expired",
                  ("1", op.external_id));
    }

    contract_agreement_service.reject_by(contract, op.party);
}

} // namespace chain
} // namespace deip
