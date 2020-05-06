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
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research_token_sale.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/services/dbs_expertise_contribution.hpp>
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/services/dbs_research_group_invite.hpp>
#include <deip/chain/services/dbs_research_token.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_vesting_balance.hpp>
#include <deip/chain/services/dbs_expertise_allocation_proposal.hpp>
#include <deip/chain/services/dbs_grant_application.hpp>
#include <deip/chain/services/dbs_funding_opportunity.hpp>
#include <deip/chain/services/dbs_dynamic_global_properties.hpp>

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

    const auto& dup_guard = duplicated_entity_guard(dgp_service);
    dup_guard(op);

    auto creator_balance  = account_balance_service.get_by_owner_and_asset(op.creator, op.fee.symbol);
    FC_ASSERT(creator_balance.amount >= op.fee.amount, 
      "Insufficient balance to create account.",
      ("creator.balance", creator_balance.amount)("required", op.fee.amount));

    FC_ASSERT(op.fee >= DEIP_MIN_ACCOUNT_CREATION_FEE,
      "Insufficient Fee: ${f} required, ${p} provided.",
      ("f", DEIP_MIN_ACCOUNT_CREATION_FEE)("p", op.fee));

    // check accounts existence
    account_service.check_account_existence(op.owner.account_auths);
    account_service.check_account_existence(op.active.account_auths);
    account_service.check_account_existence(op.posting.account_auths);

    account_service.create_account_by_faucets(
      op.new_account_name, 
      op.creator, 
      op.memo_key, 
      op.json_metadata, 
      op.owner, 
      op.active, 
      op.posting, 
      op.fee,
      op.traits,
      op.is_user_account());
}

void update_account_evaluator::do_apply(const update_account_operation& op)
{
    if (op.posting)
        op.posting->validate();

    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& account = account_service.get_account(op.account);
    const auto& account_auth = account_service.get_account_authority(op.account);

    if (op.owner)
    {
        account_service.check_account_existence(op.owner->account_auths);
        account_service.update_owner_authority(account, *op.owner);
    }

    if (op.active)
    {
        account_service.check_account_existence(op.active->account_auths);
    }

    if (op.posting)
    {
        account_service.check_account_existence(op.posting->account_auths);
    }


    std::string json_metadata = op.json_metadata.valid() ? *op.json_metadata : fc::to_string(account.json_metadata);
    public_key_type memo_key = op.memo_key.valid() ? *op.memo_key : account.memo_key;

    account_service.update_acount(
      account, 
      account_auth, 
      memo_key,
      json_metadata, 
      op.owner, 
      op.active, 
      op.posting,
      op.traits);
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

    const auto& from_balance = account_balance_service.get_by_owner_and_asset(op.from, op.amount.symbol);

    FC_ASSERT(asset(from_balance.amount, from_balance.symbol) >= op.amount, "Account does not have sufficient funds for transfer.");

    account_balance_service.adjust_balance(op.from, -op.amount);
    account_balance_service.adjust_balance(op.to, op.amount);
}

void transfer_to_common_tokens_evaluator::do_apply(const transfer_to_common_tokens_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();

    auto from_balance = account_balance_service.get_by_owner_and_asset(o.from, o.amount.symbol);

    const auto& from_account = account_service.get_account(o.from);
    const auto& to_account = o.to.size() ? account_service.get_account(o.to) : from_account;

    FC_ASSERT(from_balance.amount >= o.amount.amount, "Account does not have sufficient DEIP for transfer.");

    account_balance_service.adjust_balance(o.from, -o.amount);
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
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_expert_token& expert_token_service = _db.obtain_service<dbs_expert_token>();
    dbs_discipline& discipline_service = _db.obtain_service<dbs_discipline>();
    dbs_review& review_service = _db.obtain_service<dbs_review>();
    dbs_review_vote& review_votes_service = _db.obtain_service<dbs_review_vote>();
    dbs_expertise_contribution& expertise_contributions_service = _db.obtain_service<dbs_expertise_contribution>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_research_content& research_content_service = _db.obtain_service<dbs_research_content>();

    FC_ASSERT(op.weight > 0, "Review vote weight must be specified.");

    const auto& voter = account_service.get_account(op.voter);
    FC_ASSERT(voter.can_vote, "Review voter has declined his voting rights.");

    const auto& review = review_service.get(op.review_id);
    const auto& expert_token = expert_token_service.get_expert_token_by_account_and_discipline(op.voter, op.discipline_id);
    const auto& discipline = discipline_service.get_discipline(op.discipline_id);
    const auto& research_content = research_content_service.get_research_content(review.research_content_id);
    const auto& research = research_service.get_research(research_content.research_id);
    const auto& now = _db.head_block_time();

    FC_ASSERT(std::any_of(review.disciplines.begin(), review.disciplines.end(),
            [&](const discipline_id_type& discipline_id) {
                return discipline_id == op.discipline_id; }),
            "Cannot vote with {d} expert tokens as this discipline is not related to the review",
            ("d", discipline.name));

    FC_ASSERT(!review_votes_service.review_vote_exists_by_voter_and_discipline(op.review_id, voter.name, op.discipline_id), 
            "${a} has voted for this review with ${d} discipline already", 
            ("a", voter.name)("d", discipline.name));

    const int64_t elapsed_seconds = (now - expert_token.last_vote_time).to_seconds();
    const int64_t regenerated_power_percent = (DEIP_100_PERCENT * elapsed_seconds) / DEIP_VOTE_REGENERATION_SECONDS;
    const int64_t current_power_percent = std::min(int64_t(expert_token.voting_power + regenerated_power_percent), int64_t(DEIP_100_PERCENT));
    FC_ASSERT(current_power_percent > 0, 
            "${a} does not have power for ${e} expertise currently to vote for the review. The available power is ${p} %", 
            ("a", voter.name)("e", expert_token.discipline_id)("p", current_power_percent / DEIP_1_PERCENT));

    const int64_t vote_applied_power_percent = abs(op.weight);
    const int64_t used_power_percent = (current_power_percent * vote_applied_power_percent) / DEIP_100_PERCENT;
    FC_ASSERT(used_power_percent <= current_power_percent,
            "${a} does not have enough power for ${e} expertise to vote for the review with ${w} % of power. The available power is ${p} %",
            ("a", voter.name)("e", expert_token.discipline_id)("w", vote_applied_power_percent / DEIP_1_PERCENT)("p", current_power_percent / DEIP_1_PERCENT));

    const uint64_t used_expert_token_amount = ((uint128_t(expert_token.amount.value) * used_power_percent) / (DEIP_100_PERCENT)).to_uint64();
    FC_ASSERT(used_expert_token_amount > 0, "Account does not have enough power to vote for review.");

    _db._temporary_public_impl().modify(expert_token, [&](expert_token_object& t) {
        t.voting_power = current_power_percent - (DEIP_REVIEW_VOTE_SPREAD_DENOMINATOR != 0 ? (used_power_percent / DEIP_REVIEW_VOTE_SPREAD_DENOMINATOR) : 0);
        t.last_vote_time = now;
    });

    const discipline_id_share_type_map previous_research_content_eci = research_content.eci_per_discipline;
    const discipline_id_share_type_map previous_research_eci = research.eci_per_discipline;

    const review_vote_object& review_vote = review_votes_service.create_review_vote(
      voter.name, 
      review.id, 
      op.discipline_id, 
      used_expert_token_amount, 
      now, 
      review.created_at,
      research_content.id,
      research.id
    );

    const research_content_object& updated_research_content = research_content_service.update_eci_evaluation(research_content.id);
    const research_object& updated_research = research_service.update_eci_evaluation(research.id);

    const eci_diff research_content_eci_diff = eci_diff(
      previous_research_content_eci.at(op.discipline_id),
      updated_research_content.eci_per_discipline.at(op.discipline_id),
      now,
      static_cast<uint16_t>(expertise_contribution_type::review_support),
      review_vote.id._id
    );

    expertise_contributions_service.adjust_expertise_contribution(
        discipline.id, 
        research.id, 
        research_content.id,
        research_content_eci_diff
    );

    _db.push_virtual_operation(research_content_eci_history_operation(
        research_content.id._id, 
        op.discipline_id,
        research_content_eci_diff)
    );

    const eci_diff research_eci_diff = eci_diff(
      previous_research_eci.at(op.discipline_id),
      updated_research.eci_per_discipline.at(op.discipline_id),
      now,
      static_cast<uint16_t>(expertise_contribution_type::review_support),
      review_vote.id._id
    );

    _db.push_virtual_operation(research_eci_history_operation(
        research.id._id, 
        op.discipline_id,
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

    flat_set<account_name_type> required_posting;
    flat_set<account_name_type> required_active;
    flat_set<account_name_type> required_owner;
    vector<authority> other;

    for( auto& wrap : op.proposed_ops )
    {
        operation_get_required_authorities(wrap.op, required_active, required_owner, required_posting, other);
    }

    FC_ASSERT(other.size() == 0); // TODO: what about other??? 
    if (required_posting.size() != 0) // Transactions with operations required posting authority cannot be combined with transactions requiring active or owner authority
    {
        FC_ASSERT(required_active.size() == 0);
        FC_ASSERT(required_owner.size() == 0);
    }

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
    // Posting authority can not be combined with owner/active authorities
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
      remaining_active,
      required_posting
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
          op.posting_approvals_to_add.empty() && 
          op.key_approvals_to_add.empty(), 
          "This proposal is in its review period. No new approvals may be added."
        );
    }

    for (account_name_type account : op.owner_approvals_to_remove)
    {
        FC_ASSERT(proposal.available_owner_approvals.find(account) != proposal.available_owner_approvals.end(), 
        "", ("account", account)("available", proposal.available_owner_approvals));
    }
    for (account_name_type account : op.active_approvals_to_remove)
    {
        FC_ASSERT(proposal.available_active_approvals.find(account) != proposal.available_active_approvals.end(),
          "", ("account", account)("available", proposal.available_active_approvals));
    }
    for (account_name_type account : op.posting_approvals_to_remove)
    {
        FC_ASSERT(proposal.available_posting_approvals.find(account) != proposal.available_posting_approvals.end(),
          "", ("account", account)("available", proposal.available_posting_approvals));
    }

    proposals_service.update_proposal(proposal, 
      op.owner_approvals_to_add,
      op.active_approvals_to_add,
      op.posting_approvals_to_add,
      op.owner_approvals_to_remove,
      op.active_approvals_to_remove,
      op.posting_approvals_to_remove,
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
            db.modify(proposal, [&e](proposal_object& p) {
                fc::from_string(p.fail_reason, e.to_string(fc::log_level(fc::log_level::all)));
            });
            wlog("Proposed transaction ${id} failed to apply once approved with exception:\n----\n${reason}\n----\nWill try again when it expires.",
                ("id", proposal.external_id)("reason", e.to_detail_string()));
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
      : op.authority == static_cast<uint16_t>(authority_type::posting)
      ? proposal.required_posting_approvals
      : flat_set<account_name_type>();

    FC_ASSERT(required_approvals.find(op.account) != required_approvals.end(),
      "Provided authority is not authoritative for this proposal.",
      ("provided", op.account)("required", required_approvals));

    proposals_service.remove_proposal(proposal);
}

void make_review_evaluator::do_apply(const make_review_operation& op)
{
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_review& review_service = _db.obtain_service<dbs_review>();
    dbs_research_content& research_content_service = _db.obtain_service<dbs_research_content>();
    dbs_research_discipline_relation& research_discipline_service = _db.obtain_service<dbs_research_discipline_relation>();
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_expert_token& expertise_token_service = _db.obtain_service<dbs_expert_token>();
    dbs_expertise_contribution& expertise_contributions_service = _db.obtain_service<dbs_expertise_contribution>();

    FC_ASSERT(op.weight > 0, "Review weight must be specified.");

    account_service.check_account_existence(op.author);
    research_content_service.check_research_content_existence(op.research_content_id);
    const auto& research_content = research_content_service.get_research_content(op.research_content_id);
    const auto& research = research_service.get_research(research_content.research_id);
    const auto& reseach_group_tokens = research_group_service.get_research_group_tokens(research.research_group_id);
    const auto& existing_reviews = review_service.get_reviews_by_research_content(op.research_content_id);
    const auto& now = _db.head_block_time();

    FC_ASSERT(std::none_of(existing_reviews.begin(), existing_reviews.end(),
            [&](const review_object& rw) {
                return rw.author == op.author && rw.research_content_id == op.research_content_id; }),
            "${a} has reviewed research content ${rc} already", 
            ("a", op.author)("rc", op.research_content_id));

    FC_ASSERT(std::none_of(reseach_group_tokens.begin(), reseach_group_tokens.end(),
            [&](const research_group_token_object& rgt) {
                return rgt.owner == op.author; }),
            "${a} is member of research group ${g} and can not review its research content", 
            ("a", op.author)("g", research.research_group_id));

    const auto& expertise_tokens = expertise_token_service.get_expert_tokens_by_account_name(op.author);
    const auto& research_disciplines_relations = research_discipline_service.get_research_discipline_relations_by_research(research_content.research_id);

    std::map<discipline_id_type, share_type> review_used_expertise_by_disciplines;
    for (auto& wrap : expertise_tokens)
    {
        const auto& expert_token = wrap.get();

        if (std::any_of(research_disciplines_relations.begin(), research_disciplines_relations.end(),
            [&](const std::reference_wrapper<const research_discipline_relation_object> rel_wrap) {
                const research_discipline_relation_object& relation = rel_wrap.get();
                return relation.discipline_id == expert_token.discipline_id;
            }))
        {
            const int64_t elapsed_seconds = (now - expert_token.last_vote_time).to_seconds();
            const int64_t regenerated_power_percent = (DEIP_100_PERCENT * elapsed_seconds) / DEIP_VOTE_REGENERATION_SECONDS;
            const int64_t current_power_percent = std::min(int64_t(expert_token.voting_power + regenerated_power_percent), int64_t(DEIP_100_PERCENT));
            FC_ASSERT(current_power_percent > 0, 
                    "${a} does not have power for ${e} expertise currently to make the review. The available power is ${p} %", 
                    ("a", op.author)("e", expert_token.discipline_id)("p", current_power_percent / DEIP_1_PERCENT));

            const int64_t review_applied_power_percent = op.weight;
            const int64_t used_power_percent = (DEIP_REVIEW_REQUIRED_POWER_PERCENT * review_applied_power_percent) / DEIP_100_PERCENT;
            FC_ASSERT(used_power_percent <= current_power_percent,
                    "${a} does not have enough power for ${e} expertise to make the review with ${w} % of power. The available power is ${p} %",
                    ("a", op.author)("e", expert_token.discipline_id)("w", review_applied_power_percent / DEIP_1_PERCENT)("p", current_power_percent / DEIP_1_PERCENT));

            const uint64_t used_expert_token_amount = ((uint128_t(expert_token.amount.value) * current_power_percent) / (DEIP_100_PERCENT)).to_uint64();
            FC_ASSERT(used_expert_token_amount > 0, "Account does not have enough power to make the review.");

            _db._temporary_public_impl().modify(expert_token, [&](expert_token_object& exp) {
                exp.voting_power = current_power_percent - used_power_percent;
                exp.last_vote_time = now;
            });

            review_used_expertise_by_disciplines.insert(std::make_pair(expert_token.discipline_id, used_expert_token_amount));
        }
    }

    FC_ASSERT(review_used_expertise_by_disciplines.size() != 0, 
        "${a} does not have expertise to review ${r} research", 
        ("a", op.author)("r", research.id));

    const std::set<discipline_id_type>& review_disciplines = std::accumulate(review_used_expertise_by_disciplines.begin(), review_used_expertise_by_disciplines.end(), std::set<discipline_id_type>(),
        [=](std::set<discipline_id_type> acc, std::pair<discipline_id_type, share_type> entry) {
            acc.insert(entry.first);
            return acc;
        });

    bool is_positive;
    fc::optional<std::map<assessment_criteria, uint16_t>> scores;

    if (op.assessment_model.which() == assessment_models::tag<binary_scoring_assessment_model_v1_0_0_type>::value)
    {
        const auto model = op.assessment_model.get<binary_scoring_assessment_model_v1_0_0_type>();
        is_positive = model.is_positive;
    }
    else if (op.assessment_model.which() == assessment_models::tag<multicriteria_scoring_assessment_model_v1_0_0_type>::value)
    {
        const auto model = op.assessment_model.get<multicriteria_scoring_assessment_model_v1_0_0_type>();

        const uint16_t total_score = std::accumulate(std::begin(model.scores), std::end(model.scores), 0,
            [&](uint16_t total, const std::map<uint16_t, uint16_t>::value_type& m) { return total + m.second; });

        is_positive = total_score >= DEIP_MIN_POSITIVE_REVIEW_MARK;

        std::map<assessment_criteria, uint16_t> temp_scores;
        for (const auto& score : model.scores)
        {
            const assessment_criteria criteria = static_cast<assessment_criteria>(score.first);
            temp_scores.insert(std::make_pair(criteria, score.second));
        }

        scores = temp_scores;
    } 
    else
    {
        FC_ASSERT(false, "Review assessment model is not specified");
    }

    const discipline_id_share_type_map previous_research_content_eci = research_content.eci_per_discipline;
    const discipline_id_share_type_map previous_research_eci = research.eci_per_discipline;

    const auto& review = review_service.create(
      op.research_content_id,
      op.content,
      is_positive,
      op.author,
      review_disciplines,
      review_used_expertise_by_disciplines,
      op.assessment_model.which(),
      scores);

    const research_content_object& updated_research_content = research_content_service.update_eci_evaluation(research_content.id);
    const research_object& updated_research = research_service.update_eci_evaluation(research.id);

    for (auto& review_discipline_id : review_disciplines)
    {
        const eci_diff research_content_eci_diff = eci_diff(
          previous_research_content_eci.at(review_discipline_id),
          updated_research_content.eci_per_discipline.at(review_discipline_id),
          now,
          static_cast<uint16_t>(expertise_contribution_type::review),
          review.id._id
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
          review.id._id
        );

        _db.push_virtual_operation(research_eci_history_operation(
            research.id._id,
            review_discipline_id._id,
            research_eci_diff)
        );
    }

    _db._temporary_public_impl().modify(research, [&](research_object& r_o) {
        r_o.number_of_positive_reviews += review.is_positive ? 1 : 0;
        r_o.number_of_negative_reviews += review.is_positive ? 0 : 1;
    });
}

void contribute_to_token_sale_evaluator::do_apply(const contribute_to_token_sale_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();
    dbs_research_token_sale& research_token_sale_service = _db.obtain_service<dbs_research_token_sale>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();

    FC_ASSERT(research_service.research_exists(op.research_external_id), 
      "Research ${1} does not exist",
      ("1", op.research_external_id));

    FC_ASSERT(account_service.account_exists(op.contributor), 
      "Account ${1} does not exist",
      ("1", op.contributor));

    const auto& research = research_service.get_research(op.research_external_id);
    const auto& research_token_sales = research_token_sale_service.get_by_research_id_and_status(research.id, research_token_sale_status::token_sale_active);

    FC_ASSERT(research_token_sales.size() == 1, 
      "No active research token sale found for research ${1}",
      ("1", op.research_external_id));

    const auto& research_token_sale = (*research_token_sales.begin()).get();
    const auto& account_balance = account_balance_service.get_by_owner_and_asset(op.contributor, op.amount.symbol);

    FC_ASSERT(account_balance.amount >= op.amount.amount, 
      "Not enough funds to contribute. Available: ${1} Requested: ${2}", 
      ("1", account_balance.amount)("2", op.amount));

    asset amount_to_contribute = op.amount;
    bool is_hard_cap_reached = research_token_sale.total_amount + amount_to_contribute >= research_token_sale.hard_cap;

    if (is_hard_cap_reached) 
    {
        amount_to_contribute = research_token_sale.hard_cap - research_token_sale.total_amount;
    }

    // TODO: move to the service
    auto research_token_sale_contribution = _db._temporary_public_impl().find<research_token_sale_contribution_object, by_owner_and_research_token_sale_id>(std::make_tuple(op.contributor, research_token_sale.id));
    if (research_token_sale_contribution != nullptr) 
    {
        _db._temporary_public_impl().modify(*research_token_sale_contribution, [&](research_token_sale_contribution_object &rtsc_o) { rtsc_o.amount += amount_to_contribute; });
    }
    else 
    {
        fc::time_point_sec contribution_time = _db.head_block_time();
        research_token_sale_service.contribute(research_token_sale.id, op.contributor, contribution_time, amount_to_contribute);
    }

    account_balance_service.adjust_balance(op.contributor, -amount_to_contribute);
    research_token_sale_service.increase_tokens_amount(research_token_sale.id, amount_to_contribute);

    if (is_hard_cap_reached) 
    {
        research_token_sale_service.update_status(research_token_sale.id, token_sale_finished);
        research_token_sale_service.distribute_research_tokens(research_token_sale.id);
    }

    _db.push_virtual_operation(token_sale_contribution_to_history_operation(
      research_token_sale.research_id._id,
      research_token_sale.id._id,
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

    auto account_balance = account_balance_service.get_by_owner_and_asset(op.creator, op.balance.symbol);

    FC_ASSERT(account_balance.amount >= op.balance.amount, "Not enough funds to create vesting contract");

    account_balance_service.adjust_balance(op.creator, -op.balance);
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
            account_balance_service.adjust_balance(op.owner, op.amount);
        }
    }
}

void transfer_research_share_evaluator::do_apply(const transfer_research_share_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_research_token& research_token_service = _db.obtain_service<dbs_research_token>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();

    FC_ASSERT(account_service.account_exists(op.sender),
      "Account ${1} does not exist",
      ("1", op.sender));

    FC_ASSERT(account_service.account_exists(op.receiver),
      "Account ${1} does not exist",
      ("1", op.receiver));

    FC_ASSERT(research_service.research_exists(op.research_external_id),
      "Research with external id: ${1} does not exist",
      ("1", op.research_external_id));

    const auto& research = research_service.get_research(op.research_external_id);

    FC_ASSERT(research_token_service.exists_by_owner_and_research(op.sender, research.id),
      "Research token with owner: ${1} and research: ${2} does not exist",
      ("1", op.sender)("2", research.id));

    const auto& shart_to_transfer = research_token_service.get_by_owner_and_research(op.sender, research.id);

    FC_ASSERT(op.share.amount <= shart_to_transfer.amount,
      "Amount cannot be greater than total research token amount. Provided value: ${1}, actual amount: ${2}.",
      ("1", op.share.amount)("2", shart_to_transfer.amount));

    research_token_service.adjust_research_token(op.sender, research.id, -op.share.amount, false);
    research_token_service.adjust_research_token(op.receiver, research.id, op.share.amount, false);
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
    const dbs_account& accounts_service = _db.obtain_service<dbs_account>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();
    const dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_funding_opportunity& funding_opportunities_service = _db.obtain_service<dbs_funding_opportunity>();
    const dbs_asset& assets_service = _db.obtain_service<dbs_asset>();
    dbs_discipline_supply& discipline_supply_service = _db.obtain_service<dbs_discipline_supply>();

    FC_ASSERT(accounts_service.account_exists(op.grantor), "Account ${a} does not exists", ("a", op.grantor));
    FC_ASSERT(assets_service.exists_by_symbol(op.amount.symbol), "Asset ${s} does not exists", ("s", op.amount.symbol));

    std::set<discipline_id_type> target_disciplines;
    target_disciplines.insert(op.target_disciplines.begin(), op.target_disciplines.end());
    FC_ASSERT(target_disciplines.size() != 0, "Grant target disciplines are not specified");

    const account_balance_object& grantor_balance = account_balance_service.get_by_owner_and_asset(op.grantor, op.amount.symbol);
    FC_ASSERT(grantor_balance.amount >= op.amount.amount, 
      "Grantor ${g} does not have enough funds. Requested: ${ga} Actual: ${ba}", 
      ("g", op.grantor)("ga", op.amount)("ba", grantor_balance.amount));


    if (op.distribution_model.which() == grant_distribution_models::tag<announced_application_window_contract_v1_0_0_type>::value)
    {
        const auto announced_application_window_contract = op.distribution_model.get<announced_application_window_contract_v1_0_0_type>();
        FC_ASSERT(research_group_service.research_group_exists(announced_application_window_contract.review_committee_id), 
          "Review committee ${rg} does not exists", 
          ("rg", announced_application_window_contract.review_committee_id));
          
        FC_ASSERT(announced_application_window_contract.open_date >= _db.head_block_time(), "Start date must be greater than now");
        FC_ASSERT(announced_application_window_contract.open_date < announced_application_window_contract.close_date, "Open date must be earlier than close date");

        funding_opportunities_service.create_grant_with_eci_evaluation_distribution(
          op.grantor,
          op.amount,
          target_disciplines,
          announced_application_window_contract.funding_opportunity_number,
          announced_application_window_contract.additional_info,
          announced_application_window_contract.review_committee_id,
          announced_application_window_contract.min_number_of_positive_reviews,
          announced_application_window_contract.min_number_of_applications,
          announced_application_window_contract.max_number_of_research_to_grant,
          announced_application_window_contract.open_date,
          announced_application_window_contract.close_date);
    }
 
    else if (op.distribution_model.which() == grant_distribution_models::tag<funding_opportunity_announcement_contract_v1_0_0_type>::value)
    {
        const auto foa_contract = op.distribution_model.get<funding_opportunity_announcement_contract_v1_0_0_type>();
        FC_ASSERT(research_group_service.research_group_exists(foa_contract.organization_id), 
          "Organization ${1} does not exists", 
          ("1", foa_contract.organization_id));

        FC_ASSERT(research_group_service.research_group_exists(foa_contract.review_committee_id), 
          "Review committee ${1} does not exists", 
          ("1", foa_contract.review_committee_id));

        FC_ASSERT(research_group_service.research_group_exists(foa_contract.treasury_id), 
          "Treasury ${1} does not exists", 
          ("1", foa_contract.treasury_id));

        for (auto& officer : foa_contract.officers)
        {
            FC_ASSERT(research_group_service.is_research_group_member(officer, foa_contract.organization_id),
              "Account ${1} is not a member of ${2} organization and can not be programm officer",
              ("1", officer)("2", foa_contract.organization_id));
        }

        FC_ASSERT(foa_contract.open_date >= _db.head_block_time(), "Open date must be greater than now");
        FC_ASSERT(foa_contract.open_date < foa_contract.close_date, "Open date must be earlier than close date");

        funding_opportunities_service.create_grant_with_officer_evaluation_distribution(
          foa_contract.organization_id, 
          foa_contract.review_committee_id,
          foa_contract.treasury_id,
          op.grantor,
          foa_contract.funding_opportunity_number,
          foa_contract.additional_info,
          target_disciplines,
          op.amount,
          foa_contract.award_ceiling,
          foa_contract.award_floor,
          foa_contract.expected_number_of_awards,
          foa_contract.officers,
          foa_contract.open_date,
          foa_contract.close_date);
    }

    else if (op.distribution_model.which() == grant_distribution_models::tag<discipline_supply_announcement_contract_v1_0_0_type>::value)
    {
        const auto discipline_supply_announcement_contract = op.distribution_model.get<discipline_supply_announcement_contract_v1_0_0_type>();

        int64_t target_discipline = *op.target_disciplines.begin();
        discipline_supply_service.create_discipline_supply(
          op.grantor,
          op.amount,
          discipline_supply_announcement_contract.start_time,
          discipline_supply_announcement_contract.end_time,
          target_discipline,
          discipline_supply_announcement_contract.is_extendable,
          discipline_supply_announcement_contract.content_hash,
          discipline_supply_announcement_contract.additional_info);
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
    const dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
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

    FC_ASSERT(research_group_service.is_research_group_member(op.creator, research.research_group_id), 
              "${1} is not a member of ${2} research group",
              ("${1}", op.creator)("2", research.research_group_id));

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

void make_review_for_application_evaluator::do_apply(const make_review_for_application_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_expert_token& expertise_token_service = _db.obtain_service<dbs_expert_token>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_grant_application& grant_application_service = _db.obtain_service<dbs_grant_application>();
    dbs_research_discipline_relation& research_discipline_service = _db.obtain_service<dbs_research_discipline_relation>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    FC_ASSERT(account_service.account_exists(op.author),
              "Account(author) ${1} does not exist",
              ("1", op.author));

    FC_ASSERT(grant_application_service.grant_application_exists(op.grant_application_id),
              "Grant application with ID: ${1} does not exist",
              ("1", op.grant_application_id));

    const auto& application = grant_application_service.get_grant_application(op.grant_application_id);
    const auto& research = research_service.get_research(application.research_id);
    const auto reseach_group_tokens = research_group_service.get_research_group_tokens(research.research_group_id);

    for (auto& reseach_group_token : reseach_group_tokens)
        FC_ASSERT(reseach_group_token.get().owner != op.author, "You cannot review your own content");

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
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    account_service.check_account_existence(op.approver);
    grant_application_service.check_grant_application_existence(op.grant_application_id);

    auto& grant_application = grant_application_service.get_grant_application(op.grant_application_id);
    FC_ASSERT(grant_application.status == grant_application_status::pending,
              "Grant application ${1} has ${2} status",
              ("1", grant_application.id)("2", grant_application.status));

    const auto& grant = funding_opportunity_service.get_funding_opportunity(grant_application.funding_opportunity_number);

    const auto officers = research_group_service.get_research_group_members(grant.review_committee_id);
    bool op_is_allowed = grant.grantor == op.approver
        || std::any_of(officers.begin(), officers.end(),
                       [&](const account_name_type& officer) { return officer == op.approver; });

    FC_ASSERT(op_is_allowed, "This account cannot approve grant applications");
    grant_application_service.update_grant_application_status(grant_application, grant_application_status::approved);
}

void reject_grant_application_evaluator::do_apply(const reject_grant_application_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();
    dbs_grant_application& grant_application_service = _db.obtain_service<dbs_grant_application>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    account_service.check_account_existence(op.rejector);
    grant_application_service.check_grant_application_existence(op.grant_application_id);

    const auto& grant_application = grant_application_service.get_grant_application(op.grant_application_id);
    FC_ASSERT(grant_application.status == grant_application_status::pending,
              "Grant application ${a} has ${s} status", ("a", grant_application.id)("s", grant_application.status));

    const auto& grant = funding_opportunity_service.get_funding_opportunity(grant_application.funding_opportunity_number);

    const auto officers = research_group_service.get_research_group_members(grant.review_committee_id);
    bool op_is_allowed = grant.grantor == op.rejector
        || std::any_of(officers.begin(), officers.end(),
                       [&](const account_name_type& officer) { return officer == op.rejector; });

    FC_ASSERT(op_is_allowed, "This account cannot reject grant applications");
    grant_application_service.update_grant_application_status(grant_application, grant_application_status::rejected);
}

void create_asset_evaluator::do_apply(const create_asset_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_asset& asset_service = _db.obtain_service<dbs_asset>();

    account_service.check_account_existence(op.issuer);

    int p = std::pow(10, op.precision);
    std::string string_asset = "0." + fc::to_string(p).erase(0, 1) + " " + op.symbol;
    asset new_asset = asset::from_string(string_asset);

    asset_service.create(new_asset.symbol, op.symbol, op.precision, op.issuer, op.name, op.description);
}

void issue_asset_evaluator::do_apply(const issue_asset_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();
    dbs_asset& asset_service = _db.obtain_service<dbs_asset>();

    account_service.check_account_existence(op.issuer);
    account_balance_service.adjust_balance(op.issuer, op.amount);
    asset_service.check_existence(op.amount.symbol);

    auto& asset_obj = asset_service.get_by_symbol(op.amount.symbol);

    asset_service.adjust_current_supply(asset_obj, op.amount.amount);
}

void reserve_asset_evaluator::do_apply(const reserve_asset_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();
    dbs_asset& asset_service = _db.obtain_service<dbs_asset>();

    account_service.check_account_existence(op.owner);

    FC_ASSERT(asset_service.exists_by_symbol(op.amount.symbol), "Asset does not exist");
    auto& _asset_obj = asset_service.get_by_symbol(op.amount.symbol);

    account_balance_service.check_existence_by_owner_and_asset(op.owner, op.amount.symbol);
    account_balance_service.adjust_balance(op.owner, -op.amount);

    asset_service.adjust_current_supply(_asset_obj, -op.amount.amount);
}

void create_award_evaluator::do_apply(const create_award_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_award& award_service = _db.obtain_service<dbs_award>();
    dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();

    FC_ASSERT(account_service.account_exists(op.creator), 
      "Account ${1} does not exist", 
      ("1", op.creator));

    FC_ASSERT(account_service.account_exists(op.awardee),
      "Account ${1} does not exist",
      ("1", op.awardee));

    FC_ASSERT(research_service.research_exists(op.research_id),
      "Research ID:${1} does not exist",
      ("1", op.research_id));

    FC_ASSERT(research_group_service.research_group_exists(op.university_id),
      "University with ID:${1} does not exist",
      ("1", op.university_id));

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

    const auto& foa_organization = research_group_service.get_research_group(foa.organization_id);
    
    FC_ASSERT(research_group_service.is_research_group_member(op.creator, foa_organization.id),
      "${1} is not a member of FOA organization ${2}", 
      ("1", op.creator)("2", foa_organization.id));

    FC_ASSERT(research_group_id_type(op.university_id) != foa_organization.id,
      "Funding opportunity agency ${1} is not allowed as intermediary",
      ("1", op.university_id));

    FC_ASSERT(op.award.symbol == foa.amount.symbol,
      "Award asset ${1} does not match funding opportunity asset ${2}.",
      ("1", op.award.symbol_name())("2", foa.amount.symbol_name()));

    FC_ASSERT(account_balance_service.exists_by_owner_and_asset(op.creator, op.award.symbol),
      "Account balance for ${1} for asset ${2} does not exist", 
      ("1", op.creator)("2", op.award.symbol_name()));

    const asset university_fee = asset(((op.award.amount * share_type(op.university_overhead))) / DEIP_100_PERCENT, op.award.symbol);
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
      op.university_id,
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
      op.research_id,
      award_recipient_status::unconfirmed);

    for (auto& subaward : op.subawardees)
    {
        FC_ASSERT(account_service.account_exists(subaward.subawardee),
          "Account ${1} does not exist",
          ("1", subaward.subawardee));

        FC_ASSERT(research_service.research_exists(subaward.research_id),
          "Research ID:${1} does not exist",
          ("1", subaward.research_id));

        const auto& research = research_service.get_research(subaward.research_id);

        FC_ASSERT(research_group_service.is_research_group_member(subaward.subawardee, research.research_group_id),
          "Awardee ${1} is not a member of research group ID:{2}",
          ("1", subaward.subawardee)("2", research.research_group_id));

        award_service.create_award_recipient(
          award.award_number,
          subaward.subaward_number,
          award.funding_opportunity_number,
          subaward.subawardee,
          subaward.source,
          awards_map[subaward.subawardee],
          subaward.research_id,
          award_recipient_status::unconfirmed);
    }
}

void approve_award_evaluator::do_apply(const approve_award_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_award& award_service = _db.obtain_service<dbs_award>();
    dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
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

    const auto& university = research_group_service.get_research_group(award.university_id);
    const asset university_fee = asset(((award.amount.amount * share_type(award.university_overhead)) / DEIP_100_PERCENT), award.amount.symbol);
    account_balance_service.adjust_balance(university.creator, university_fee);

    auto awardees = award_service.get_award_recipients_by_award(op.award_number);

    for (auto& wrap : awardees)
    {
        const award_recipient_object& award_recipient = wrap.get();
        account_balance_service.adjust_balance(award_recipient.awardee, award_recipient.total_amount);
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
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

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

    FC_ASSERT(research_group_service.is_research_group_member(op.certifier, award.university_id),
      "Certifier ${1} is not a member of university with ID:{2}",
      ("1", op.certifier)("2", award.university_id));

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

    FC_ASSERT(std::any_of(foa.officers.begin(), foa.officers.end(),
      [&](const account_name_type& officer) { return officer == op.approver; }),
      "Account ${1} is not Funding Opportunity officer",
      ("1", op.approver));

    award_service.update_award_withdrawal_request(withdrawal, award_withdrawal_request_status::approved);
}

void reject_award_withdrawal_request_evaluator::do_apply(const reject_award_withdrawal_request_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_award& award_service = _db.obtain_service<dbs_award>();
    dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<dbs_funding_opportunity>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

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
        FC_ASSERT(research_group_service.is_research_group_member(op.rejector, award.university_id),
          "Rejector ${1} is not a member of university with ID:{2}",
          ("1", op.rejector)("2", award.university_id));
    }
    else if (withdrawal.status == static_cast<uint16_t>(award_withdrawal_request_status::certified))
    {
        FC_ASSERT(std::any_of(foa.officers.begin(), foa.officers.end(),
          [&](const account_name_type &officer) { return officer == op.rejector; }),
          "Account ${1} is not Funding Opportunity officer",
          ("1", op.rejector));
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
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
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
    const auto& treasury = research_group_service.get_research_group(foa.treasury_id);

    FC_ASSERT(research_group_service.is_research_group_member(op.payer, treasury.id),
      "Account ${1} is not a member of the Treasury",
      ("1", op.payer));

    FC_ASSERT(award_recipient.total_amount - award_recipient.total_expenses >= withdrawal.amount, 
      "Not enough funds to process the payment. Requested ${1}, Available: ${2} ", 
      ("1", withdrawal.amount)("2", award_recipient.total_amount - award_recipient.total_expenses));

    const auto& grant_asset = asset_service.get_by_symbol(withdrawal.amount.symbol);

    award_service.adjust_expenses(award_recipient.id, withdrawal.amount);
    asset_service.adjust_current_supply(grant_asset, -withdrawal.amount.amount); // burn grant tokens

    const price rate = price(asset(1, DEIP_USD_SYMBOL), asset(1, withdrawal.amount.symbol));
    const asset payout = withdrawal.amount * rate;

    account_balance_service.adjust_balance(withdrawal.requester, payout); // imitation of acquiring api call
    account_balance_service.adjust_balance(treasury.creator, -payout); // imitation of acquiring api call

    award_service.update_award_withdrawal_request(withdrawal, award_withdrawal_request_status::paid);
}

void create_nda_contract_evaluator::do_apply(const create_nda_contract_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_nda_contract& nda_contracts_service = _db.obtain_service<dbs_nda_contract>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    fc::time_point_sec now = _db.head_block_time();
    fc::time_point_sec start_date = op.start_date.valid() ? *op.start_date : now;

    FC_ASSERT(account_service.account_exists(op.contract_creator),
              "Account(contract_creator) ${1} does not exist",
              ("1", op.contract_creator));

    FC_ASSERT(account_service.account_exists(op.party_a),
              "Account(party_a) ${1} does not exist",
              ("1", op.party_a));

    FC_ASSERT(account_service.account_exists(op.party_a),
              "Account(party_b) ${1} does not exist",
              ("1", op.party_b));

    FC_ASSERT(research_group_service.is_research_group_member(op.party_a, op.party_a_research_group_id),
              "Account(party_a) ${1} is not a member of research group ID: ${2}",
              ("1", op.party_a)("2", op.party_a_research_group_id));

    FC_ASSERT(research_group_service.is_research_group_member(op.party_b, op.party_a_research_group_id),
              "Account(party_b) ${1} is not a member of research group ID: ${2}",
              ("1", op.party_b)("2", op.party_b_research_group_id));

    const auto& contracts = nda_contracts_service.get_by_creator_research_group_and_signee_research_group_and_contract_hash(op.party_a_research_group_id,
                                                                                                                            op.party_b_research_group_id,
                                                                                                                            op.contract_hash);

    FC_ASSERT(start_date >= now,
              "NDA start date (${start_date}) must be later or equal to the current moment (${now})",
              ("start_date", start_date)("now", now));

    FC_ASSERT(op.end_date > now,
              "NDA end date (${end_date}) must be later the current moment (${now})",
              ("end_date", op.end_date)("now", now));

    FC_ASSERT(op.end_date > start_date,
              "NDA start date (${start_date}) must be less than end date (${end_date})",
              ("start_date", start_date)("end_date", op.end_date));

    for (const auto& wrapper : contracts)
    {
        const auto& contract = wrapper.get();
        FC_ASSERT(contract.status != static_cast<uint16_t>(nda_contract_status::nda_contract_pending),
                  "NDA contract '${hash}' already exists with status '${status}' ",
                  ("hash", op.contract_hash)("status", contract.status));

        FC_ASSERT(contract.status != static_cast<uint16_t>(nda_contract_status::nda_contract_signed),
                  "NDA contract '${hash}' already exists with status '${status}' ",
                  ("hash", op.contract_hash)("status", contract.status));
    }

//    if (_db.has_hardfork(DEIP_HARDFORK_0_1)) {
//        dbs_subscription& subscription_service = _db.obtain_service<dbs_subscription>();
//        subscription_service.consume_nda_contract_quota_unit(op.party_a_research_group_id);
//    }

    nda_contracts_service.create(op.contract_creator,
                                 op.party_a,
                                 op.party_a_research_group_id,
                                 op.party_b,
                                 op.party_b_research_group_id,
                                 op.disclosing_party,
                                 op.title,
                                 op.contract_hash,
                                 now,
                                 start_date,
                                 op.end_date);
}

void sign_nda_contract_evaluator::do_apply(const sign_nda_contract_operation& op)
{
    dbs_account &account_service = _db.obtain_service<dbs_account>();
    dbs_nda_contract& nda_contracts_service = _db.obtain_service<dbs_nda_contract>();
    dbs_research_group &research_group_service = _db.obtain_service<dbs_research_group>();

    FC_ASSERT(account_service.account_exists(op.contract_signer),
              "Account(contract_signer) ${1} does not exist",
              ("1", op.contract_signer));

    FC_ASSERT(nda_contracts_service.nda_contract_exists(op.contract_id),
              "Nda contract with ID:${1} does not exist",
              ("1", op.contract_id));

    const auto &signer = account_service.get_account(op.contract_signer);
    const auto& contract = nda_contracts_service.get(op.contract_id);

    const bool is_party_a_sig = contract.party_a == op.contract_signer;

    FC_ASSERT((contract.party_a == op.contract_signer) || (contract.party_b == op.contract_signer),
              "Only ${party_a} or ${party_b} accounts can sign this NDA contract",
              ("party_a", contract.party_a)("party_b", contract.party_b));

    FC_ASSERT(contract.status == static_cast<uint16_t>(nda_contract_status::nda_contract_pending),
              "NDA contract with status ${status} cannot be signed",
              ("status", contract.status));

    FC_ASSERT(research_group_service.is_research_group_member(op.contract_signer, is_party_a_sig ? contract.party_a_research_group_id : contract.party_b_research_group_id),
              "Account(contract_signer) ${1} is not a member of research group ID: ${2}",
              ("1", op.contract_signer)("2", is_party_a_sig ? contract.party_a_research_group_id : contract.party_b_research_group_id));

    std::string stringified_party_a_signature = fc::to_string(contract.party_a_signature);
    std::string stringified_party_b_signature = fc::to_string(contract.party_b_signature);

    FC_ASSERT(stringified_party_a_signature.find(op.signature) == std::string::npos,
              "Signature ${signature} is already made from ${party} party",
              ("signature", op.signature)("party", contract.party_a));

    FC_ASSERT(stringified_party_b_signature.find(op.signature) == std::string::npos,
              "Signature ${signature} is already made from ${party} party",
              ("signature", op.signature)("party", contract.party_b));

    fc::sha256 digest = fc::sha256::hash(fc::to_string(contract.contract_hash));
    fc::ecc::compact_signature signature;
    fc::array<char, 65> buffer;
    fc::from_hex(op.signature, buffer.begin(), buffer.size());
    for (int i = 0; i < 65; i++)
    {
        unsigned char ch = static_cast<unsigned char>(buffer.at(i));
        signature.data[i] = ch;
    }
    fc::ecc::public_key signer_public_key = fc::ecc::public_key(signature, digest);

    const auto &signee_auth = account_service.get_account_authority(signer.name);
    std::set<public_key_type> keys;
    for (const auto &k : authority(signee_auth.owner).get_keys())
        keys.insert(k);
    for (const auto &k : authority(signee_auth.active).get_keys())
        keys.insert(k);

    const bool is_signature_valid = keys.find(signer_public_key) != keys.end();

    FC_ASSERT(is_signature_valid,
              "Signature ${signature} does not correspond any of ${signer} authorities",
              ("signature", signature)("signer", op.contract_signer));

    const auto& signed_contract = nda_contracts_service.sign(contract, op.contract_signer, op.signature);

    if (signed_contract.party_a_signature.size() && signed_contract.party_b_signature.size()) { // todo: add contract status resolver
        nda_contracts_service.set_new_contract_status(signed_contract, nda_contract_status::nda_contract_signed);
    }
}

void decline_nda_contract_evaluator::do_apply(const decline_nda_contract_operation& op)
{
    dbs_account &account_service = _db.obtain_service<dbs_account>();
    dbs_nda_contract& nda_contracts_service = _db.obtain_service<dbs_nda_contract>();
    dbs_research_group &research_group_service = _db.obtain_service<dbs_research_group>();

    FC_ASSERT(account_service.account_exists(op.decliner),
              "Account(decliner) ${1} does not exist",
              ("1", op.decliner));

    FC_ASSERT(nda_contracts_service.nda_contract_exists(op.contract_id),
              "Nda contract with ID:${1} does not exist",
              ("1", op.contract_id));

    const auto& contract = nda_contracts_service.get(op.contract_id);

    // Currently we are not supporting sharing files by both sides within a single NDA contract
    FC_ASSERT(contract.party_b == op.decliner,
              "Only ${party_b} account can decline the contract",
              ("party_b", contract.party_b));

    FC_ASSERT(research_group_service.is_research_group_member(op.decliner, contract.party_b_research_group_id),
              "Account(decliner) ${1} is not a member of research group ID: ${2}",
              ("1", op.decliner)("2", contract.party_b_research_group_id));

    FC_ASSERT(contract.status == static_cast<uint16_t>(nda_contract_status::nda_contract_pending),
              "NDA contract with status ${status} cannot be declined",
              ("status", contract.status));

    nda_contracts_service.set_new_contract_status(contract, nda_contract_status::nda_contract_declined);
}

void close_nda_contract_evaluator::do_apply(const close_nda_contract_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_nda_contract& nda_contracts_service = _db.obtain_service<dbs_nda_contract>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    FC_ASSERT(account_service.account_exists(op.closer),
              "Account(closer) ${1} does not exist",
              ("1", op.closer));

    FC_ASSERT(nda_contracts_service.nda_contract_exists(op.contract_id),
              "Nda contract with ID:${1} does not exist",
              ("1", op.contract_id));

    const auto& contract = nda_contracts_service.get(op.contract_id);

    // Currently we are not supporting sharing files by both sides within a single NDA contract
    FC_ASSERT(contract.party_a == op.closer,
              "Only ${party_a} account can close the contract",
              ("party_a", contract.party_a));

    FC_ASSERT(research_group_service.is_research_group_member(op.closer, contract.party_b_research_group_id),
              "Account(closer) ${1} is not a member of research group ID: ${2}",
              ("1", op.closer)("2", contract.party_b_research_group_id));

    FC_ASSERT(contract.status == static_cast<uint16_t>(nda_contract_status::nda_contract_pending),
              "NDA contract with status ${status} cannot be closed",
              ("status", contract.status));

    nda_contracts_service.set_new_contract_status(contract, nda_contract_status::nda_contract_closed);
}

void create_request_by_nda_contract_evaluator::do_apply(const create_request_by_nda_contract_operation& op)
{
    dbs_account &account_service = _db.obtain_service<dbs_account>();
    dbs_nda_contract& nda_contracts_service = _db.obtain_service<dbs_nda_contract>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_nda_contract_requests& nda_contract_requests_service = _db.obtain_service<dbs_nda_contract_requests>();

    fc::time_point_sec now = _db.head_block_time();

    FC_ASSERT(account_service.account_exists(op.requester),
              "Account(requester) ${1} does not exist",
              ("1", op.requester));

    FC_ASSERT(nda_contracts_service.nda_contract_exists(op.contract_id),
              "Nda contract with ID:${1} does not exist",
              ("1", op.contract_id));

    const auto& contract = nda_contracts_service.get(op.contract_id);

    // Currently we are not supporting sharing files by both sides within a single NDA contract
    FC_ASSERT(op.requester == contract.party_b,
              "Two-way NDA contracts are not supported currently. Only ${party_b} can create the request",
              ("party_b", contract.party_b));

    FC_ASSERT(research_group_service.is_research_group_member(op.requester, contract.party_b_research_group_id),
              "Account(requester) ${1} is not a member of research group ID: ${2}",
              ("1", op.requester)("2", contract.party_b_research_group_id));

    FC_ASSERT(contract.status == static_cast<uint16_t>(nda_contract_status::nda_contract_signed),
              "Files cannot be shared under the terms of a contract with ${status} status",
              ("status", contract.status));

    FC_ASSERT(contract.start_date <= now,
              "NDA contract is not active yet and will be operational at ${start_date}",
              ("start_date", contract.start_date));

    nda_contract_requests_service.create_file_access_request(op.contract_id, op.requester, op.encrypted_payload_hash, op.encrypted_payload_iv);
}

void fulfill_request_by_nda_contract_evaluator::do_apply(const fulfill_request_by_nda_contract_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_nda_contract& nda_contracts_service = _db.obtain_service<dbs_nda_contract>();
    dbs_nda_contract_requests& nda_contract_requests_service = _db.obtain_service<dbs_nda_contract_requests>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    FC_ASSERT(account_service.account_exists(op.grantor),
              "Account(grantor) ${1} does not exist",
              ("1", op.grantor));

    FC_ASSERT(nda_contract_requests_service.request_exists(op.request_id),
              "Request with ID:${1} does not exist",
              ("1", op.request_id));

    const auto& request = nda_contract_requests_service.get(op.request_id);

    FC_ASSERT(nda_contracts_service.nda_contract_exists(request.contract_id),
              "Nda contract with ID:${1} does not exist",
              ("1", request.contract_id));

    const auto& contract = nda_contracts_service.get(request.contract_id);

    // Currently we are not supporting sharing files by both sides within a single NDA contract
    FC_ASSERT(op.grantor == contract.party_a,
              "Two-way NDA contracts are not supported currently. Only ${party_a} can fulfill the request",
              ("party_a", contract.party_a));

    FC_ASSERT(research_group_service.is_research_group_member(op.grantor, contract.party_a_research_group_id),
              "Account(grantor) ${1} is not a member of research group ID: ${2}",
              ("1", op.grantor)("2", contract.party_a_research_group_id));

    FC_ASSERT(contract.status == static_cast<uint16_t>(nda_contract_status::nda_contract_signed),
              "Files cannot be shared under the terms of a contract with ${status} status",
              ("status", contract.status));

    FC_ASSERT(request.status == static_cast<uint16_t>(nda_contract_file_access_status::pending),
              "File access request with ${status} status cannot be fulfilled",
              ("status", request.status));

    nda_contract_requests_service.fulfill_file_access_request(request, op.encrypted_payload_encryption_key, op.proof_of_encrypted_payload_encryption_key);
}

void join_research_group_membership_evaluator::do_apply(const join_research_group_membership_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_groups_service = _db.obtain_service<dbs_research_group>();
    auto& research_service = _db.obtain_service<dbs_research>();

    FC_ASSERT(account_service.account_exists(op.member), "Account ${1} does not exist", ("1", op.member));

    FC_ASSERT(account_service.account_exists(op.research_group),
      "Research group ${1} does not exist",
      ("1", op.research_group));

    const auto& research_group = research_groups_service.get_research_group_by_account(op.research_group);

    FC_ASSERT(!research_group.is_personal, 
      "Can not join to personal ${1} research group", 
      ("1", research_group.account));

    FC_ASSERT(!research_groups_service.is_research_group_member(op.member, research_group.id),
      "Account ${1} is already a member of ${2} research group",
      ("1", op.member)("2", research_group.account));

    std::vector<std::reference_wrapper<const research_object>> researches;
    if (op.researches.valid())
    {
        const auto& list = *op.researches;
        for (auto& external_id : list)
        {
            const auto& research_opt = research_service.get_research_if_exists(external_id);
            FC_ASSERT(research_opt.valid(), "Research ${1} does not exist", ("1", external_id));
            const auto& research = (*research_opt).get();
            FC_ASSERT(research.research_group_id == research_group.id,
              "Research ${1} is not a research of ${2} research group",
              ("1", external_id)("2", research_group.account));
            researches.push_back(research);
        }
    }
    else
    {
        const auto& all_researches = research_service.get_researches_by_research_group(research_group.id);
        researches.insert(researches.end(), all_researches.begin(), all_researches.end());
    }

    for (auto& wrap : researches)
    {
        const auto& research = wrap.get();
        flat_set<account_name_type> updated_members;
        updated_members.insert(research.members.begin(), research.members.end());
        updated_members.insert(op.member);

        research_service.update_research(research, 
          fc::to_string(research.title), 
          fc::to_string(research.abstract), 
          fc::to_string(research.permlink),
          research.is_private,
          research.review_share,
          research.compensation_share,
          updated_members);
    }

    research_groups_service.add_member_to_research_group(
      op.member,
      research_group.id,
      op.reward_share.amount,
      account_name_type());
}

void left_research_group_membership_evaluator::do_apply(const left_research_group_membership_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_groups_service = _db.obtain_service<dbs_research_group>();
    auto& research_service = _db.obtain_service<dbs_research>();
    auto& research_token_service = _db.obtain_service<dbs_research_token>();

    FC_ASSERT(account_service.account_exists(op.member), 
      "Account ${1} does not exist", 
      ("1", op.member));

    FC_ASSERT(account_service.account_exists(op.research_group), 
      "Research group ${1} does not exist", 
      ("1", op.research_group));

    const auto& research_group = research_groups_service.get_research_group_by_account(op.research_group);

    FC_ASSERT(!research_group.is_personal, 
      "Can not left personal ${1} research group", 
      ("1", research_group.account));

    FC_ASSERT(research_groups_service.is_research_group_member(op.member, research_group.id),
      "Account ${1} is not a member of ${2} research group.",
      ("1", op.member)("2", research_group.account));

    const auto& rgt = research_groups_service.get_research_group_token_by_member(op.member, research_group.id);
    const auto& all_researches = research_service.get_researches_by_research_group(research_group.id);

    if (op.is_exclusion)
    {
        for (auto& wrap : all_researches)
        {
            const auto& research = wrap.get();
            if (research.compensation_share.valid() && research.members.find(op.member) != research.members.end())
            {
                const auto& compensation_share = *research.compensation_share;
                const auto& compensation = research.owned_tokens.amount * (rgt.amount * compensation_share.amount / DEIP_100_PERCENT) / DEIP_100_PERCENT;
                research_service.decrease_owned_tokens(research, percent(compensation));
                research_token_service.adjust_research_token(op.member, research.id, compensation, true);
            }
        }
    }

    for (auto& wrap : all_researches)
    {
        const auto& research = wrap.get();
        flat_set<account_name_type> updated_members;

        for (auto& member : research.members)
        {
            if (member != op.member)
                updated_members.insert(member);
        }

        research_service.update_research(research, 
          fc::to_string(research.title), 
          fc::to_string(research.abstract), 
          fc::to_string(research.permlink),
          research.is_private,
          research.review_share,
          research.compensation_share,
          updated_members);
    }

    research_groups_service.remove_member_from_research_group(
      op.member, 
      research_group.id);
}

void create_research_evaluator::do_apply(const create_research_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_groups_service = _db.obtain_service<dbs_research_group>();
    auto& research_service = _db.obtain_service<dbs_research>();
    auto& discipline_service =_db.obtain_service<dbs_discipline>();
    auto& dgp_service = _db.obtain_service<dbs_dynamic_global_properties>();

    const auto& dup_guard = duplicated_entity_guard(dgp_service);
    dup_guard(op);

    FC_ASSERT(account_service.account_exists(op.research_group),
      "Account(creator) ${1} does not exist",
      ("1", op.research_group));

    const auto& research_group = research_groups_service.get_research_group_by_account(op.research_group);

    std::set<discipline_id_type> disciplines;
    for (auto& discipline_id : op.disciplines)
    {
        FC_ASSERT(discipline_service.discipline_exists(discipline_id),
          "Discipline with ID: ${1} does not exist",
          ("1", discipline_id));

        disciplines.insert(discipline_id_type(discipline_id));
    }

    if (research_group.is_personal)
    {
        FC_ASSERT(!op.members.valid(), "Can not add members to personal research");
        FC_ASSERT(!op.compensation_share.valid(), "Exclusion compensation is not allowed for personal research");
    }

    const auto block_time = _db.head_block_time();
    const bool is_finished = false;

    flat_set<account_name_type> members;
    if (op.members.valid())
    {
        const auto& list = *op.members;
        for (auto& member : list)
        {
            FC_ASSERT(research_groups_service.is_research_group_member(member, research_group.id),
              "Account ${1} is not a member of ${2} research group. Add the account to group membership at first",
              ("1", member)("2", research_group.account));
            members.insert(member);
        }
    }
    else 
    {
        const auto& all_members = research_groups_service.get_research_group_tokens(research_group.id);
        for (auto& wrap : all_members)
        {
            const auto& member = wrap.get();
            members.insert(member.owner);
        }
    }

    research_service.create_research(
      research_group.id, 
      op.external_id, 
      op.title, 
      op.abstract, 
      op.permlink, 
      disciplines,
      op.review_share, 
      op.compensation_share, 
      op.is_private,
      is_finished,
      percent(DEIP_100_PERCENT), 
      members,
      block_time
    );
}

void create_research_content_evaluator::do_apply(const create_research_content_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_group_service = _db.obtain_service<dbs_research_group>();
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

    const auto& research_group = research_group_service.get_research_group_by_account(op.research_group);

    FC_ASSERT(research_service.research_exists(op.research_external_id),
      "Research with external id: ${1} does not exist",
      ("1", op.research_external_id));

    const auto& research = research_service.get_research(op.research_external_id);

    FC_ASSERT(research.research_group_id == research_group.id,
      "Research ${1} does not belong to research group ${2}",
      ("1", research.external_id)("2", research_group.account));

    FC_ASSERT(!research.is_finished,
      "The research ${1} has been finished already",
      ("1", research.title));

    for (auto& author : op.authors)
    {
        FC_ASSERT(research.members.find(author) != research.members.end(),
          "Account ${1} is not a member of ${2} research. Add the account to the research at first",
          ("1", author)("2", research.external_id));
    }

    const auto& research_content = research_content_service.create_research_content(
      research.id,
      op.external_id,
      op.title,
      op.content,
      op.permlink,
      static_cast<research_content_type>(op.type),
      op.authors,
      op.references,
      op.foreign_references,
      block_time);

    for (external_id_type id : op.references)
    {
        const auto& ref = research_content_service.get_research_content(id);
        _db.push_virtual_operation(research_content_reference_history_operation(
          research_content.id._id,
          research_content.research_id._id,
          fc::to_string(research_content.content),
          ref.id._id,
          ref.research_id._id,
          fc::to_string(ref.content))
        );
    }

    const std::map<discipline_id_type, share_type> previous_research_content_eci = research_content_service.get_eci_evaluation(research_content.id);
    const std::map<discipline_id_type, share_type> previous_research_eci = research_service.get_eci_evaluation(research.id);

    const research_content_object& updated_research_content = research_content_service.update_eci_evaluation(research_content.id);
    const research_object& updated_research = research_service.update_eci_evaluation(research.id);

    const auto& relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research.id);
    for (auto& wrap : relations)
    {
        const auto& rel = wrap.get();

        const eci_diff research_content_eci_diff = eci_diff(
          previous_research_content_eci.at(rel.discipline_id),
          updated_research_content.eci_per_discipline.at(rel.discipline_id),
          block_time,
          static_cast<uint16_t>(expertise_contribution_type::publication),
          updated_research_content.id._id
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
          updated_research_content.id._id
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
    auto& research_group_service = _db.obtain_service<dbs_research_group>();
    auto& research_token_sale_service = _db.obtain_service<dbs_research_token_sale>();
    const auto& block_time = _db.head_block_time();

    FC_ASSERT(account_service.account_exists(op.research_group),
      "Account(creator) ${1} does not exist",
      ("1", op.research_group));

    const auto& research_group = research_group_service.get_research_group_by_account(op.research_group);

    FC_ASSERT(research_service.research_exists(op.research_external_id),
      "Research with external id: ${1} does not exist",
      ("1", op.research_external_id));

    const auto& research = research_service.get_research(op.research_external_id);

    FC_ASSERT(research.research_group_id == research_group.id,
      "Research ${1} does not belong to research group ${2}",
      ("1", op.research_external_id)("2", research_group.name));

    auto research_token_sales = research_token_sale_service.get_by_research_id_and_status(research.id, research_token_sale_status::token_sale_active);

    FC_ASSERT(research_token_sales.size() == 0, 
      "Research ${1} token sale is in progress", 
      ("1", op.research_external_id));

    FC_ASSERT(op.start_time >= block_time);

    FC_ASSERT(research.owned_tokens - op.share >= percent(0),
      "Provided share: ${1}, Available share: ${2}.",
      ("1", op.share)("2", percent(research.owned_tokens)));

    research_service.decrease_owned_tokens(research, op.share);
    research_token_sale_service.start(
      research.id,
      op.start_time,
      op.end_time,
      op.share.amount,
      op.soft_cap,
      op.hard_cap
    );
}

void update_research_evaluator::do_apply(const update_research_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_service = _db.obtain_service<dbs_research>();
    auto& research_groups_service = _db.obtain_service<dbs_research_group>();

    FC_ASSERT(account_service.account_exists(op.research_group),
      "Account(creator) ${1} does not exist",
      ("1", op.research_group));

    const auto& research_group = research_groups_service.get_research_group_by_account(op.research_group);

    if (research_group.is_personal)
    {
        FC_ASSERT(!op.members.valid(), "Can not add members to personal research");
        FC_ASSERT(!op.compensation_share.valid(), "Exclusion compensation is not allowed for personal research");
    }

    FC_ASSERT(research_service.research_exists(op.external_id),
      "Research with external id: ${1} does not exist",
      ("1", op.external_id));

    const auto& research = research_service.get_research(op.external_id);

    FC_ASSERT(research.research_group_id == research_group.id,
      "Research ${1} does not belong to research group ${2}",
      ("1", research.external_id)("2", research_group.account));

    if (op.members.valid())
    {
        const auto& members = *op.members;
        for (auto& member : members)
        {
            FC_ASSERT(research_groups_service.is_research_group_member(member, research_group.id),
              "Account ${1} is not a member of ${2} research group. Add the account to group membership at first",
              ("1", member)("2", research_group.account));
        }
    }

    std::string title = op.title.valid() ? *op.title : fc::to_string(research.title);
    std::string abstract = op.abstract.valid() ? *op.abstract : fc::to_string(research.abstract);
    std::string permlink = op.permlink.valid() ? *op.permlink : fc::to_string(research.permlink);
    bool is_private = op.is_private.valid() ? *op.is_private : research.is_private;
    percent review_share = op.review_share.valid() ? *op.review_share : research.review_share;
    optional<percent> compensation_share = op.compensation_share.valid() ? op.compensation_share : research.compensation_share;
    flat_set<account_name_type> members = op.members.valid() ? *op.members : research.members;

    research_service.update_research(research, 
      title,
      abstract,
      permlink,
      is_private,
      review_share,
      compensation_share,
      members);
}


} // namespace chain
} // namespace deip