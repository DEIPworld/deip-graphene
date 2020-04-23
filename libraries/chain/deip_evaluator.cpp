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
#include <deip/chain/services/dbs_grant.hpp>
#include <deip/chain/services/dbs_grant_application.hpp>
#include <deip/chain/services/dbs_grant_application_review.hpp>
#include <deip/chain/services/dbs_funding_opportunity.hpp>

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
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();

    auto creator_balance = account_balance_service.get_by_owner_and_asset(op.creator, op.fee.symbol);
    FC_ASSERT(creator_balance.amount >= op.fee.amount, 
      "Insufficient balance to create account.",
      ("creator.balance", creator_balance.amount)("required", op.fee.amount));

    FC_ASSERT(op.fee >= asset(DEIP_MIN_ACCOUNT_CREATION_FEE, DEIP_SYMBOL),
      "Insufficient Fee: ${f} required, ${p} provided.",
      ("f", asset(DEIP_MIN_ACCOUNT_CREATION_FEE, DEIP_SYMBOL))("p", op.fee));

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

void account_update_evaluator::do_apply(const account_update_operation& o)
{
    if (o.posting)
        o.posting->validate();

    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& account = account_service.get_account(o.account);
    const auto& account_auth = account_service.get_account_authority(o.account);

    if (o.owner)
    {
#ifndef IS_TEST_NET
        FC_ASSERT(_db.head_block_time() - account_auth.last_owner_update > DEIP_OWNER_UPDATE_LIMIT,
                  "Owner authority can only be updated once an hour.");
#endif
        account_service.check_account_existence(o.owner->account_auths);

        account_service.update_owner_authority(account, *o.owner);
    }

    if (o.active)
    {
        account_service.check_account_existence(o.active->account_auths);
    }

    if (o.posting)
    {
        account_service.check_account_existence(o.posting->account_auths);
    }

    account_service.update_acount(account, account_auth, o.memo_key, o.json_metadata, o.owner, o.active, o.posting);
}

/**
 *  Because net_rshares is 0 there is no need to update any pending payout calculations or parent posts.
 */

void transfer_evaluator::do_apply(const transfer_operation& o)
{
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();

    auto from_balance = account_balance_service.get_by_owner_and_asset(o.from, o.amount.symbol);

    FC_ASSERT(asset(from_balance.amount, from_balance.symbol) >= o.amount, "Account does not have sufficient funds for transfer.");

    account_balance_service.adjust_balance(o.from, -o.amount);
    account_balance_service.adjust_balance(o.to, o.amount);
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
    const auto& block_time = _db.head_block_time();
    FC_ASSERT(op.expiration_time > block_time, "Proposal has already expired on creation." );
    // FC_ASSERT(op.expiration_time <= block_time + global_parameters.maximum_proposal_lifetime,
    //             "Proposal expiration time is too far in the future.");
    FC_ASSERT(!op.review_period_seconds || fc::seconds(*op.review_period_seconds) < (op.expiration_time - block_time),
      "Proposal review period must be less than its overall lifetime." );
    
    {
        // If we're dealing with the committee authority, make sure this transaction has a sufficient review period.
        flat_set<account_name_type> auths;
        vector<authority> other;
        for( auto& wrap : op.proposed_ops )
        {
          operation_get_required_authorities(wrap.op, auths, auths, auths, other);
        }

        FC_ASSERT( other.size() == 0 ); // TODO: what about other??? 
    }

    transaction _proposed_trx;

    for (const op_wrapper& wrap : op.proposed_ops)
    {
        _proposed_trx.operations.push_back(wrap.op);
    }

   _proposed_trx.validate();

   const proposal_object& proposal = _db._temporary_public_impl().create<proposal_object>([&](proposal_object& proposal) {
      proposal.external_id = op.external_id;
      _proposed_trx.expiration = op.expiration_time;
      proposal.proposed_transaction = _proposed_trx;
      proposal.expiration_time = op.expiration_time;
      proposal.proposer = op.creator;
      if (op.review_period_seconds)
        proposal.review_period_time = op.expiration_time - *op.review_period_seconds;

      //Populate the required approval sets
      flat_set<account_name_type> required_active;
      flat_set<account_name_type> required_posting;
      vector<authority> other;
      
      // TODO: consider caching values from evaluate?
      for( auto& o : _proposed_trx.operations )
      {
          operation_get_required_authorities(o, required_active, proposal.required_owner_approvals, required_posting, other);
      }

      //All accounts which must provide both owner and active authority should be omitted from the active authority set;
      //owner authority approval implies active authority approval.
      std::set_difference(required_active.begin(), required_active.end(),
                          proposal.required_owner_approvals.begin(), proposal.required_owner_approvals.end(),
                          std::inserter(proposal.required_active_approvals, proposal.required_active_approvals.begin()));
   });
}

void update_proposal_evaluator::do_apply(const update_proposal_operation& op)
{
    const auto& proposals_service = _db.obtain_service<dbs_proposal>();
    const auto& block_time = _db.head_block_time();
    chainbase::database& db = _db._temporary_public_impl();

    FC_ASSERT(proposals_service.proposal_exists(op.external_id),
      "Proposal ${1} does not exist", ("1", op.external_id));

    const auto& proposal = proposals_service.get_proposal(op.external_id);

    if (proposal.review_period_time && block_time >= *proposal.review_period_time)
    {
        FC_ASSERT(op.active_approvals_to_add.empty() && op.owner_approvals_to_add.empty(),
          "This proposal is in its review period. No new approvals may be added." );
    }

    for (account_name_type account : op.active_approvals_to_remove)
    {
        FC_ASSERT(proposal.available_active_approvals.find(account) != proposal.available_active_approvals.end(),
          "", ("account", account)("available", proposal.available_active_approvals));
    }
    for (account_name_type account : op.owner_approvals_to_remove)
    {
        FC_ASSERT(proposal.available_owner_approvals.find(account) != proposal.available_owner_approvals.end(),
          "", ("account", account)("available", proposal.available_owner_approvals));
    }

    db.modify(proposal, [&](proposal_object& p) {
        p.available_active_approvals.insert(op.active_approvals_to_add.begin(), op.active_approvals_to_add.end());
        p.available_owner_approvals.insert(op.owner_approvals_to_add.begin(), op.owner_approvals_to_add.end());
        for(account_name_type account : op.active_approvals_to_remove)
          p.available_active_approvals.erase(account);
        for(account_name_type account : op.owner_approvals_to_remove)
          p.available_owner_approvals.erase(account);
        for(const auto& key : op.key_approvals_to_add)
          p.available_key_approvals.insert(key);
        for(const auto& key : op.key_approvals_to_remove)
          p.available_key_approvals.erase(key);
    });

    // If the proposal has a review period, don't bother attempting to authorize/execute it.
    // Proposals with a review period may never be executed except at their expiration.
    if (proposal.review_period_time) return;

    if (proposal.is_authorized_to_execute(db))
    {
        // All required approvals are satisfied. Execute!
        try {
            _db.push_proposal(proposal);
        } catch(fc::exception& e) {
            db.modify(proposal, [&e](proposal_object& p) {
                fc::from_string(p.fail_reason, e.to_string(fc::log_level(fc::log_level::all)));
            });

          // wlog("Proposed transaction ${id} failed to apply once approved with exception:\n----\n${reason}\n----\nWill try again when it expires.",
          //       ("id", op.proposal)("reason", e.to_detail_string()));
        }
    }

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

    research_token_sale_service.check_research_token_sale_existence(op.research_token_sale_id);
    account_service.check_account_existence(op.owner);

    auto account_balance = account_balance_service.get_by_owner_and_asset(op.owner, op.amount.symbol);

    FC_ASSERT(account_balance.amount >= op.amount.amount, "Not enough funds to contribute");

    auto& research_token_sale = research_token_sale_service.get_by_id(op.research_token_sale_id);
    FC_ASSERT(research_token_sale.status == research_token_sale_status::token_sale_active, "You cannot contribute to inactive, finished or expired token sale");

    asset amount_to_contribute = op.amount;
    bool is_hard_cap_reached = research_token_sale.total_amount + amount_to_contribute >= research_token_sale.hard_cap;

    if (is_hard_cap_reached) {
        amount_to_contribute = research_token_sale.hard_cap - research_token_sale.total_amount;
    }

    auto research_token_sale_contribution = _db._temporary_public_impl().
            find<research_token_sale_contribution_object, by_owner_and_research_token_sale_id>(boost::make_tuple(op.owner, op.research_token_sale_id));

    if (research_token_sale_contribution != nullptr) {
        _db._temporary_public_impl().modify(*research_token_sale_contribution,
                                            [&](research_token_sale_contribution_object &rtsc_o) { rtsc_o.amount += amount_to_contribute; });
    } else {
        fc::time_point_sec contribution_time = _db.head_block_time();
        research_token_sale_service.contribute(op.research_token_sale_id, op.owner,
                                               contribution_time, amount_to_contribute);
    }

    account_balance_service.adjust_balance(op.owner, -amount_to_contribute);
    research_token_sale_service.increase_tokens_amount(op.research_token_sale_id, amount_to_contribute);

    if (is_hard_cap_reached) {
        research_token_sale_service.update_status(op.research_token_sale_id, token_sale_finished);
        research_token_sale_service.distribute_research_tokens(op.research_token_sale_id);
    }

    _db.push_virtual_operation(token_sale_contribution_to_history_operation(research_token_sale.research_id._id,
                                                                            research_token_sale.id._id,
                                                                            op.owner,
                                                                            amount_to_contribute));

}

void approve_research_group_invite_evaluator::do_apply(const approve_research_group_invite_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_research_group_invite &research_group_invites_service = _db.obtain_service<dbs_research_group_invite>();

    FC_ASSERT(research_group_invites_service.research_group_invite_exists(op.research_group_invite_id), 
      "Invite ${i} for ${a} is expired or does not exist", 
      ("i", op.research_group_invite_id)("a", op.owner));

    const research_group_invite_object& invite = research_group_invites_service.get_research_group_invite(op.research_group_invite_id);
    account_service.check_account_existence(invite.account_name);
    research_group_service.check_research_group_existence(invite.research_group_id);

    const research_group_object& research_group = research_group_service.get_research_group(invite.research_group_id);

    research_group_service.add_member_to_research_group(
      invite.account_name,
      research_group.id,
      invite.research_group_token_amount,
      invite.token_source);

    if (invite.is_head)
    {
        research_group_service.add_research_group_head( 
          invite.account_name,
          research_group);
    }

    research_group_invites_service.remove(invite);
}

void reject_research_group_invite_evaluator::do_apply(const reject_research_group_invite_operation& op)
{
    dbs_research_group_invite& research_group_invites_service = _db.obtain_service<dbs_research_group_invite>();

    FC_ASSERT(research_group_invites_service.research_group_invite_exists(op.research_group_invite_id), 
      "Invite ${i} for ${a} is expired or does not exist", 
      ("i", op.research_group_invite_id)("a", op.owner));

    const research_group_invite_object& invite = research_group_invites_service.get_research_group_invite(op.research_group_invite_id);
    research_group_invites_service.remove(invite);
}

void transfer_research_tokens_to_research_group_evaluator::do_apply(const transfer_research_tokens_to_research_group_operation& op)
{
    dbs_research_token &research_token_service = _db.obtain_service<dbs_research_token>();
    dbs_research &research_service = _db.obtain_service<dbs_research>();

    research_token_service.check_existence_by_owner_and_research(op.owner, op.research_id);
    research_service.check_research_existence(op.research_id);

    auto& research_token = research_token_service.get_by_owner_and_research(op.owner, op.research_id);
    auto& research = research_service.get_research(op.research_id);

    FC_ASSERT(op.amount > 0 && share_type(op.amount) <= research_token.amount, "Amount cannot be negative or greater than research token amount");

    _db._temporary_public_impl().modify(research, [&](research_object& r_o) {
        r_o.owned_tokens += op.amount;
    });

    if (op.amount == research_token.amount) {
        _db._temporary_public_impl().remove(research_token);
    } else {
        _db._temporary_public_impl().modify(research_token, [&](research_token_object &rt_o) {
            rt_o.amount -= op.amount;
        });
    }

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

void transfer_research_tokens_evaluator::do_apply(const transfer_research_tokens_operation& op)
{
    dbs_research_token &research_token_service = _db.obtain_service<dbs_research_token>();
    dbs_research &research_service = _db.obtain_service<dbs_research>();

    research_service.check_research_existence(op.research_id);
    research_token_service.check_existence_by_owner_and_research(op.sender, op.research_id);

    auto &research_token_to_transfer = research_token_service.get_by_owner_and_research(op.sender, op.research_id);

    FC_ASSERT(op.amount > 0 && share_type(op.amount) <= research_token_to_transfer.amount,
              "Amount cannot be negative or greater than total research token amount");

    if (research_token_service.exists_by_owner_and_research(op.receiver, op.research_id)) {
        auto &receiver_token = research_token_service.get_by_owner_and_research(op.receiver, op.research_id);
        _db._temporary_public_impl().modify(receiver_token, [&](research_token_object &r_o) {
            r_o.amount += op.amount;
        });
    } else {
        research_token_service.create_research_token(op.receiver, op.amount, op.research_id, false);
    }

    if (op.amount == research_token_to_transfer.amount) {
        _db._temporary_public_impl().remove(research_token_to_transfer);
    } else {
        _db._temporary_public_impl().modify(research_token_to_transfer, [&](research_token_object &rt_o) {
            rt_o.amount -= op.amount;
        });
    }
}

void delegate_expertise_evaluator::do_apply(const delegate_expertise_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_expert_token& expert_token_service = _db.obtain_service<dbs_expert_token>();

    account_service.check_account_existence(op.sender);
    account_service.check_account_existence(op.receiver);

    FC_ASSERT(expert_token_service.expert_token_exists_by_account_and_discipline(op.sender, op.discipline_id), 
      "Expertise token ${1} for ${2} does not exist",
      ("1", op.sender)("2", op.discipline_id));

    FC_ASSERT(expert_token_service.expert_token_exists_by_account_and_discipline(op.receiver, op.discipline_id),
      "Expertise token ${1} for ${2} does not exist", 
      ("1", op.receiver)("2", op.discipline_id));

    auto& sender_token = expert_token_service.get_expert_token_by_account_and_discipline(op.sender, op.discipline_id);

    FC_ASSERT(sender_token.proxy != op.receiver, "Proxy must change.");

    auto& proxy_token = expert_token_service.get_expert_token_by_account_and_discipline(op.receiver, op.discipline_id);

    expert_token_service.update_expertise_proxy(sender_token, proxy_token);

}

void revoke_expertise_delegation_evaluator::do_apply(const revoke_expertise_delegation_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_expert_token& expert_token_service = _db.obtain_service<dbs_expert_token>();

    account_service.check_account_existence(op.sender);
    FC_ASSERT(expert_token_service.expert_token_exists_by_account_and_discipline(op.sender, op.discipline_id),
      "Expertise token ${1} for ${2} does not exist", 
      ("1", op.sender)("2", op.discipline_id));

    auto& sender_token = expert_token_service.get_expert_token_by_account_and_discipline(op.sender, op.discipline_id);

    FC_ASSERT(sender_token.proxy.size() != 0, "You have no proxy");

    optional<expert_token_object> proxy;

    expert_token_service.update_expertise_proxy(sender_token, proxy);
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

    FC_ASSERT(expert_token.proxy.size() == 0, "A proxy is currently set, please clear the proxy before voting.");


    if (op.voting_power == DEIP_100_PERCENT)
        expertise_allocation_proposal_service.upvote(proposal, op.voter, expert_token.amount + expert_token.proxied_expertise_total());
    else if (op.voting_power == -DEIP_100_PERCENT)
        expertise_allocation_proposal_service.downvote(proposal, op.voter, expert_token.amount + expert_token.proxied_expertise_total());

}

void create_grant_evaluator::do_apply(const create_grant_operation& op)
{
    const dbs_account& accounts_service = _db.obtain_service<dbs_account>();
    dbs_account_balance& account_balance_service = _db.obtain_service<dbs_account_balance>();
    const dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_funding_opportunity& funding_opportunities_service = _db.obtain_service<dbs_funding_opportunity>();
    dbs_grant& grants_service = _db.obtain_service<dbs_grant>();
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
          
        FC_ASSERT(announced_application_window_contract.start_date >= _db.head_block_time(), "Start date must be greater than now");
        FC_ASSERT(announced_application_window_contract.start_date < announced_application_window_contract.end_date, "Start date must be earlier than end date");

        grants_service.create_grant_with_announced_application_window(
          op.grantor,
          op.amount,
          target_disciplines,
          announced_application_window_contract.review_committee_id,
          announced_application_window_contract.min_number_of_positive_reviews,
          announced_application_window_contract.min_number_of_applications,
          announced_application_window_contract.max_number_of_research_to_grant,
          announced_application_window_contract.start_date,
          announced_application_window_contract.end_date);
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

        funding_opportunities_service.create_funding_opportunity_announcement(
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
    const dbs_grant& grant_service = _db.obtain_service<dbs_grant>();
    const dbs_account& account_service = _db.obtain_service<dbs_account>();
    const dbs_research& research_service = _db.obtain_service<dbs_research>();
    const dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    const dbs_research_discipline_relation& research_discipline_relation_service = _db.obtain_service<dbs_research_discipline_relation>();
    dbs_grant_application& grant_application_service = _db.obtain_service<dbs_grant_application>();

    account_service.check_account_existence(op.creator);

    FC_ASSERT(grant_service.grant_with_announced_application_window_exists(op.grant_id), 
      "Grant ${1} does not exist", ("1", op.grant_id));

    research_service.check_research_existence(op.research_id);
    
    const auto& research = research_service.get_research(op.research_id);    
    FC_ASSERT(research_group_service.is_research_group_member(op.creator, research.research_group_id), 
      "${a} is not a member of ${rg} research group",
      ("${a}", op.creator)("rg", research.research_group_id));

    const auto& grant = grant_service.get_grant_with_announced_application_window(op.grant_id);
    auto parent_discipline_itr = std::min_element(grant.target_disciplines.begin(), grant.target_disciplines.end());
    FC_ASSERT(parent_discipline_itr != grant.target_disciplines.end(), "Grant main disciplne is not defined");
    research_discipline_relation_service.check_existence_by_research_and_discipline(op.research_id, *parent_discipline_itr);

    auto now = _db.head_block_time();
    FC_ASSERT((now >= grant.start_date) && (now <= grant.end_date), "Grant is inactive now");

    grant_application_service.create_grant_application(op.grant_id, op.research_id, op.application_hash, op.creator);
}

void make_review_for_application_evaluator::do_apply(const make_review_for_application_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_expert_token& expertise_token_service = _db.obtain_service<dbs_expert_token>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_grant_application& grant_application_service = _db.obtain_service<dbs_grant_application>();
    dbs_research_discipline_relation& research_discipline_service = _db.obtain_service<dbs_research_discipline_relation>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_grant_application_review& grant_application_review_service = _db.obtain_service<dbs_grant_application_review>();

    account_service.check_account_existence(op.author);
    grant_application_service.check_grant_application_existence(op.grant_application_id);

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
        "${a} does not have expertise to review ${r} grant application", 
        ("a", op.author)("r", op.grant_application_id));

    grant_application_review_service.create(op.grant_application_id, op.content, op.is_positive, op.author, disciplines_ids);
}

void approve_grant_application_evaluator::do_apply(const approve_grant_application_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_grant& grant_service = _db.obtain_service<dbs_grant>();
    dbs_grant_application& grant_application_service = _db.obtain_service<dbs_grant_application>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    account_service.check_account_existence(op.approver);
    grant_application_service.check_grant_application_existence(op.grant_application_id);

    auto& grant_application = grant_application_service.get_grant_application(op.grant_application_id);
    FC_ASSERT(grant_application.status == grant_application_status::application_pending,
              "Grant application ${a} has ${s} status", ("a", grant_application.id)("s", grant_application.status));

    const auto& grant = grant_service.get_grant_with_announced_application_window(grant_application.grant_id);

    const auto officers = research_group_service.get_research_group_members(grant.review_committee_id);
    bool op_is_allowed = grant.grantor == op.approver
        || std::any_of(officers.begin(), officers.end(),
                       [&](const account_name_type& officer) { return officer == op.approver; });

    FC_ASSERT(op_is_allowed, "This account cannot approve grant applications");
    grant_application_service.update_grant_application_status(grant_application, grant_application_status::application_approved);
}

void reject_grant_application_evaluator::do_apply(const reject_grant_application_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_grant& grant_service = _db.obtain_service<dbs_grant>();
    dbs_grant_application& grant_application_service = _db.obtain_service<dbs_grant_application>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    account_service.check_account_existence(op.rejector);
    grant_application_service.check_grant_application_existence(op.grant_application_id);

    const auto& grant_application = grant_application_service.get_grant_application(op.grant_application_id);
    FC_ASSERT(grant_application.status == grant_application_status::application_pending,
              "Grant application ${a} has ${s} status", ("a", grant_application.id)("s", grant_application.status));

    const auto& grant = grant_service.get_grant_with_announced_application_window(grant_application.grant_id);

    const auto officers = research_group_service.get_research_group_members(grant.review_committee_id);
    bool op_is_allowed = grant.grantor == op.rejector
        || std::any_of(officers.begin(), officers.end(),
                       [&](const account_name_type& officer) { return officer == op.rejector; });

    FC_ASSERT(op_is_allowed, "This account cannot reject grant applications");
    grant_application_service.update_grant_application_status(grant_application, grant_application_status::application_rejected);
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

    FC_ASSERT(funding_opportunity_service.funding_opportunity_announcement_exists(op.funding_opportunity_number),
      "Funding opportunity with ID ${1} does not exist", 
      ("1", op.funding_opportunity_number));

    const auto& foa = funding_opportunity_service.get_funding_opportunity_announcement(op.funding_opportunity_number);

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
      fc::to_string(award.award_number),
      fc::to_string(award.award_number),
      fc::to_string(award.funding_opportunity_number),
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
          fc::to_string(award.award_number),
          subaward.subaward_number,
          fc::to_string(award.funding_opportunity_number),
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
    const auto& foa = funding_opportunity_service.get_funding_opportunity_announcement(fc::to_string(award.funding_opportunity_number));

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

    funding_opportunity_service.adjust_fundind_opportunity_supply(foa.id, -award.amount);
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
    const auto& foa = funding_opportunity_service.get_funding_opportunity_announcement(fc::to_string(award.funding_opportunity_number));

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

    const string payment_number = op.payment_number;
    const string award_number = op.award_number;
    const string subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;

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

    const string payment_number = op.payment_number;
    const string award_number = op.award_number;
    const string subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;

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

    const string payment_number = op.payment_number;
    const string award_number = op.award_number;
    const string subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;

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

    const auto& foa = funding_opportunity_service.get_funding_opportunity_announcement(fc::to_string(award_recipient.funding_opportunity_number));

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

    const string payment_number = op.payment_number;
    const string award_number = op.award_number;
    const string subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;

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
    const auto& foa = funding_opportunity_service.get_funding_opportunity_announcement(fc::to_string(award_recipient.funding_opportunity_number));

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

    const string payment_number = op.payment_number;
    const string award_number = op.award_number;
    const string subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;

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

    const auto& foa = funding_opportunity_service.get_funding_opportunity_announcement(fc::to_string(award_recipient.funding_opportunity_number));
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

//    if (_db.has_hardfork(DEIP_HARDFORK_0_1)) {
//        dbs_subscription& subscription_service = _db.obtain_service<dbs_subscription>();
//        subscription_service.consume_nda_protected_file_quota_unit(contract.party_a_research_group_id);
//    }

    nda_contract_requests_service.fulfill_file_access_request(request, op.encrypted_payload_encryption_key, op.proof_of_encrypted_payload_encryption_key);
}

void invite_member_evaluator::do_apply(const invite_member_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_group_invite_service = _db.obtain_service<dbs_research_group_invite>();
    auto& research_group_service = _db.obtain_service<dbs_research_group>();

    FC_ASSERT(account_service.account_exists(op.creator),
              "Account(creator) ${1} does not exist",
              ("1", op.creator));

    FC_ASSERT(research_group_service.research_group_exists(op.research_group_external_id),
              "Research group with external id: ${1} does not exists",
              ("1", op.research_group_external_id));

    const auto& research_group = research_group_service.get_research_group_by_account(op.research_group_external_id);

    FC_ASSERT(!research_group_service.is_research_group_member(op.creator, research_group.id),
              "Account(invitee) ${1} is already a member of ${2} research group",
              ("${1}", op.invitee)("2", research_group.id));

    FC_ASSERT(account_service.account_exists(op.invitee),
              "Account(invitee) ${1} does not exist",
              ("1", op.invitee));

    if (op.is_head)
    {
        FC_ASSERT(research_group.is_centralized,
          "Heads are allowed only in centralized research groups");
    }

    research_group_invite_service.create(op.invitee,
                                         research_group.id,
                                         op.research_group_token_amount_in_percent,
                                         op.cover_letter,
                                         account_name_type(),
                                         op.is_head);
}

void exclude_member_evaluator::do_apply(const exclude_member_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_group_service = _db.obtain_service<dbs_research_group>();
    auto& research_service = _db.obtain_service<dbs_research>();
    auto& research_token_service = _db.obtain_service<dbs_research_token>();

    FC_ASSERT(account_service.account_exists(op.creator),
              "Account(creator) ${1} does not exist",
              ("1", op.creator));

    FC_ASSERT(research_group_service.research_group_exists(op.research_group_external_id),
              "Research group with external id: ${1} does not exists",
              ("1", op.research_group_external_id));

    const auto& research_group = research_group_service.get_research_group_by_account(op.research_group_external_id);

    FC_ASSERT(research_group_service.is_research_group_member(op.creator, research_group.id),
              "Account(creator) ${1} is not a member of ${2} research group",
              ("${1}", op.creator)("2", research_group.id));

    FC_ASSERT(account_service.account_exists(op.account_to_exclude),
              "Account(to exclude) ${1} does not exist",
              ("1", op.account_to_exclude));

    FC_ASSERT(research_group_service.is_research_group_member(op.account_to_exclude, research_group.id),
              "Account(to exclude) ${1} is not a member of ${2} research group",
              ("${1}", op.account_to_exclude)("2", research_group.id));

    // TODO: rewrite after multisig implementation
    /*if (op.account_to_dropout == research_group.creator)
    {
        FC_ASSERT(op.creator == research_group.creator || proposal.voted_accounts.count(research_group.creator) != 0,
                  "Research group Creator ${c} can not be excluded by member ${m} without his own approval",
                  ("c", research_group.creator)("m", proposal.creator));
    }*/

    if (research_group.heads.count(op.account_to_exclude) != 0) // centralized
    {
        FC_ASSERT(op.creator == op.account_to_exclude || op.creator == research_group.creator,
                  "Research group Head ${1} can not be excluded by member ${2} without his own or research group creator ${3} approval",
                  ("1", op.account_to_exclude)("2", op.creator)("3", research_group.creator));
    }

    if (research_group_service.is_organization_division(research_group.id))
    {
        const auto& division_contract = research_group_service.get_division_contract_by_research_group(research_group.id);
        if (division_contract.organization_agents.count(op.account_to_exclude) != 0)
        {
            // TODO: rewrite after multisig implementation
            /*FC_ASSERT(
                    op.creator == op.account_to_dropout
                    || proposal.voted_accounts.count(data.name) != 0
                    || proposal.creator == research_group.creator
                    || proposal.voted_accounts.count(research_group.creator) != 0,
                    "Organization agent ${a} can not be excluded without organization approval",
                    ("a", data.name));*/

            research_group_service.remove_organization_agent_from_division_contract(division_contract, op.account_to_exclude);
        }
    }

    auto& token = research_group_service.get_research_group_token_by_account_and_research_group(op.account_to_exclude, research_group.id);
    auto researches = research_service.get_researches_by_research_group(research_group.id);
    for (auto& r : researches)
    {
        auto& research = r.get();

        auto tokens_amount_in_percent_after_dropout_compensation
                = token.amount * research.dropout_compensation_in_percent / DEIP_100_PERCENT;
        auto tokens_amount_after_dropout_compensation
                = research.owned_tokens * tokens_amount_in_percent_after_dropout_compensation / DEIP_100_PERCENT;

        research_service.decrease_owned_tokens(research, tokens_amount_after_dropout_compensation);

        if (research_token_service.exists_by_owner_and_research(op.account_to_exclude, research.id))
        {
            auto& research_token = research_token_service.get_by_owner_and_research(op.account_to_exclude, research.id);
            research_token_service.increase_research_token_amount(research_token, tokens_amount_after_dropout_compensation);
        }
        else
        {
            research_token_service.create_research_token(op.account_to_exclude, tokens_amount_after_dropout_compensation, research.id, true);
        }
    }

    const auto& members = research_group_service.remove_member_from_research_group(op.account_to_exclude, research_group.id);
    FC_ASSERT(members.size() != 0, "The last research group member can not be excluded"); // let's forbid it for now
}

void create_research_evaluator::do_apply(const create_research_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_group_service = _db.obtain_service<dbs_research_group>();
    auto& research_service = _db.obtain_service<dbs_research>();
    auto& discipline_service =_db.obtain_service<dbs_discipline>();

    FC_ASSERT(account_service.account_exists(op.creator),
              "Account(creator) ${1} does not exist",
              ("1", op.creator));

    FC_ASSERT(research_group_service.research_group_exists(op.research_group_external_id),
              "Research group with external id: ${1} does not exists",
              ("1", op.research_group_external_id));

    const auto& research_group = research_group_service.get_research_group_by_account(op.research_group_external_id);

    FC_ASSERT(research_group_service.is_research_group_member(op.creator, research_group.id),
              "Account(creator) ${1} is not a member of ${2} research group",
              ("${1}", op.creator)("2", research_group.id));

    std::set<discipline_id_type> disciplines;
    for (auto& discipline_id : op.disciplines)
    {
        FC_ASSERT(discipline_service.discipline_exists(discipline_id),
                  "Discipline with ID: ${1} does not exist",
                  ("1", discipline_id))
                  ;
        disciplines.insert(discipline_id_type(discipline_id));
    }

    const auto now = _db.head_block_time();
    const bool is_finished = false;

    research_service.create_research(
            research_group.id,
            op.external_id,
            op.title,
            op.abstract,
            op.permlink,
            disciplines,
            op.review_share_in_percent,
            op.dropout_compensation_in_percent,
            op.is_private,
            is_finished,
            DEIP_100_PERCENT,
            now,
            now,
            now
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

    FC_ASSERT(account_service.account_exists(op.creator),
              "Account(creator) ${1} does not exist",
              ("1", op.creator));

    FC_ASSERT(research_group_service.research_group_exists(op.research_group_external_id),
              "Research group with external id: ${1} does not exists",
              ("1", op.research_group_external_id));

    const auto& research_group = research_group_service.get_research_group_by_account(op.research_group_external_id);

    FC_ASSERT(research_group_service.is_research_group_member(op.creator, research_group.id),
              "Account(creator) ${1} is not a member of ${2} research group",
              ("${1}", op.creator)("2", research_group.id));

    FC_ASSERT(research_service.research_exists(op.research_external_id),
              "Research with external id: ${1} does not exist",
              ("1", op.research_external_id));

    const auto& now = _db.head_block_time();

    const auto& research = research_service.get_research(op.research_external_id);

    FC_ASSERT(!research.is_finished,
              "The research ${1} has been finished already",
              ("1", research.title));

    std::set<research_content_id_type> references_internal_ids;
    std::set<research_content_object> references;

    for (external_id_type reference : op.references)
    {
        const auto& ref = research_content_service.get_research_content(reference);
        references_internal_ids.insert(ref.id);
    }

    const auto& research_content = research_content_service.create_research_content(
      research.id,
      op.external_id,
      op.title,
      op.content,
      op.permlink,
      static_cast<research_content_type>(op.type),
      op.authors,
      references_internal_ids,
      op.foreign_references,
      now);

    for (auto& ref : references)
    {
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
          now,
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
          now,
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

    FC_ASSERT(account_service.account_exists(op.creator),
              "Account(creator) ${1} does not exist",
              ("1", op.creator));

    FC_ASSERT(research_group_service.research_group_exists(op.research_group_external_id),
              "Research group with external id: ${1} does not exists",
              ("1", op.research_group_external_id));

    const auto& research_group = research_group_service.get_research_group_by_account(op.research_group_external_id);

    FC_ASSERT(research_group_service.is_research_group_member(op.creator, research_group.id),
              "Account(creator) ${1} is not a member of ${2} research group",
              ("${1}", op.creator)("2", research_group.id));

    FC_ASSERT(research_service.research_exists(op.research_external_id),
              "Research with external id: ${1} does not exist",
              ("1", op.research_external_id));

    auto &research = research_service.get_research(op.research_external_id);

    FC_ASSERT(research.research_group_id == research_group.id,
              "Research ${1} does not attached to research group ${2}. Research's group id: ${3}.",
              ("1", op.research_group_external_id)("2", research_group.id)("3", research.research_group_id));

    auto research_token_sales = research_token_sale_service.get_by_research_id_and_status(research.id, research_token_sale_status::token_sale_active);

    FC_ASSERT(research_token_sales.size() == 0, "Another token sale in progress.");
    FC_ASSERT(op.start_time >= _db.head_block_time());

    FC_ASSERT(research.owned_tokens - op.amount_for_sale >= 0,
              "Tokens for sale is more than research balance. Provided amount: ${1}, available: ${2}.",
              ("1", op.amount_for_sale)("2", research.owned_tokens));

    research_service.decrease_owned_tokens(research, op.amount_for_sale);
    research_token_sale_service.start(research.id,
                                      op.start_time,
                                      op.end_time,
                                      op.amount_for_sale,
                                      op.soft_cap,
                                      op.hard_cap);
}

void update_research_group_metadata_evaluator::do_apply(const update_research_group_metadata_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_group_service = _db.obtain_service<dbs_research_group>();

    FC_ASSERT(account_service.account_exists(op.creator),
              "Account(creator) ${1} does not exist",
              ("1", op.creator));

    FC_ASSERT(research_group_service.research_group_exists(op.research_group_external_id),
              "Research group with external id: ${1} does not exists",
              ("1", op.research_group_external_id));

    const auto& research_group = research_group_service.get_research_group_by_account(op.research_group_external_id);

    FC_ASSERT(research_group_service.is_research_group_member(op.creator, research_group.id),
              "Account(creator) ${1} is not a member of ${2} research group",
              ("${1}", op.creator)("2", research_group.id));

    _db._temporary_public_impl().modify(research_group, [&](research_group_object& rg_o) {
        fc::from_string(rg_o.name, op.research_group_name);
        fc::from_string(rg_o.description, op.research_group_description);
    });
}

void update_research_metadata_evaluator::do_apply(const update_research_metadata_operation& op)
{
    auto& account_service = _db.obtain_service<dbs_account>();
    auto& research_service = _db.obtain_service<dbs_research>();
    auto& research_group_service = _db.obtain_service<dbs_research_group>();

    FC_ASSERT(account_service.account_exists(op.creator),
              "Account(creator) ${1} does not exist",
              ("1", op.creator));

    FC_ASSERT(research_group_service.research_group_exists(op.research_group_external_id),
              "Research group with external id: ${1} does not exists",
              ("1", op.research_group_external_id));

    const auto& research_group = research_group_service.get_research_group_by_account(op.research_group_external_id);

    FC_ASSERT(research_group_service.is_research_group_member(op.creator, research_group.id),
              "Account(creator) ${1} is not a member of ${2} research group",
              ("${1}", op.creator)("2", research_group.id));

    FC_ASSERT(research_service.research_exists(op.research_external_id),
              "Research with external id: ${1} does not exist",
              ("1", op.research_external_id));

    auto &research = research_service.get_research(op.research_external_id);

    FC_ASSERT(research.research_group_id == research_group.id,
              "Research ${1} does not attached to research group ${2}. Research's group id: ${3}.",
              ("1", op.research_group_external_id)("2", research_group.id)("3", research.research_group_id));

    _db._temporary_public_impl().modify(research, [&](research_object& r_o) {
        fc::from_string(r_o.title, op.research_title);
        fc::from_string(r_o.abstract, op.research_abstract);
        r_o.is_private = op.is_private;
    });
}


} // namespace chain
} // namespace deip
