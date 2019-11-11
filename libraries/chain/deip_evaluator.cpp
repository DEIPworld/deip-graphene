#include <deip/chain/deip_evaluator.hpp>
#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/schema/witness_objects.hpp>
#include <deip/chain/schema/block_summary_object.hpp>

#include <deip/chain/util/reward.hpp>

#include <deip/chain/database/database.hpp> //replace to dbservice after _temporary_public_impl remove
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_witness.hpp>
#include <deip/chain/services/dbs_grant.hpp>
#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/services/dbs_proposal.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research_token_sale.hpp>
#include <deip/chain/services/dbs_vote.hpp>
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/services/dbs_research_group_invite.hpp>
#include <deip/chain/services/dbs_research_token.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_vesting_balance.hpp>
#include <deip/chain/services/dbs_proposal_execution.hpp>
#include <deip/chain/services/dbs_expertise_stats.hpp>
#include <deip/chain/services/dbs_expertise_allocation_proposal.hpp>
#include <deip/chain/services/dbs_offer_research_tokens.hpp>

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

inline void validate_permlink_0_1(const string& permlink)
{
    FC_ASSERT(permlink.size() > DEIP_MIN_PERMLINK_LENGTH && permlink.size() < DEIP_MAX_PERMLINK_LENGTH,
              "Permlink is not a valid size.");

    for (auto ch : permlink)
    {
        if (!std::islower(ch) && !std::isdigit(ch) && !(ch == '-'))
        {
            FC_ASSERT(false, "Invalid permlink character: ${ch}", ("ch", std::string(1, ch)));
        }
    }
}

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
        #ifdef IS_TEST_NET
            if (_db.has_hardfork(DEIP_HARDFORK_0_1))
            {
                FC_ASSERT(false, "Adding new witnesses is disabled currently");
            }
        #endif
        
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

void account_create_evaluator::do_apply(const account_create_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    const auto& creator = account_service.get_account(o.creator);

    // check creator balance
    FC_ASSERT(creator.balance >= o.fee, "Insufficient balance to create account.",
              ("creator.balance", creator.balance)("required", o.fee));

    // check fee
    FC_ASSERT(o.fee >= asset(DEIP_MIN_ACCOUNT_CREATION_FEE, DEIP_SYMBOL),
              "Insufficient Fee: ${f} required, ${p} provided.",
              ("f", asset(DEIP_MIN_ACCOUNT_CREATION_FEE, DEIP_SYMBOL))("p", o.fee));

    // check accounts existence

    account_service.check_account_existence(o.owner.account_auths);

    account_service.check_account_existence(o.active.account_auths);

    account_service.check_account_existence(o.posting.account_auths);

    // write in to DB

    account_service.create_account_by_faucets(o.new_account_name, o.creator, o.memo_key, o.json_metadata, o.owner,
                                              o.active, o.posting, o.fee);

    bool is_personal = true;

    std::map<uint16_t , share_type> personal_research_group_proposal_quorums;

    for (int i = First_proposal; i <= Last_proposal; i++)
        personal_research_group_proposal_quorums.insert(std::make_pair(i, DEIP_100_PERCENT));

    auto& personal_research_group = research_group_service.create_research_group(o.new_account_name,
                                                                                 o.new_account_name,
                                                                                 o.new_account_name,
                                                                                 DEIP_100_PERCENT,
                                                                                 personal_research_group_proposal_quorums,
                                                                                 is_personal);
    research_group_service.create_research_group_token(personal_research_group.id, DEIP_100_PERCENT, o.new_account_name);

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
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& from_account = account_service.get_account(o.from);
    const auto& to_account = account_service.get_account(o.to);

    FC_ASSERT(_db.get_balance(from_account, o.amount.symbol) >= o.amount,
              "Account does not have sufficient funds for transfer.");
    account_service.adjust_balance(from_account, -o.amount);
    account_service.adjust_balance(to_account, o.amount);
}

void transfer_to_common_tokens_evaluator::do_apply(const transfer_to_common_tokens_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    const auto& from_account = account_service.get_account(o.from);
    const auto& to_account = o.to.size() ? account_service.get_account(o.to) : from_account;

    FC_ASSERT(_db.get_balance(from_account, DEIP_SYMBOL) >= o.amount,
              "Account does not have sufficient DEIP for transfer.");

    account_service.adjust_balance(from_account, -o.amount);
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
    #ifdef IS_TEST_NET
        if (_db.has_hardfork(DEIP_HARDFORK_0_1))
        {
            FC_ASSERT(false, "Voting for witnesses is disabled currently");
        }
    #endif

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

void vote_for_review_evaluator::do_apply(const vote_for_review_operation& o)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_expert_token& expert_token_service = _db.obtain_service<dbs_expert_token>();
    dbs_discipline& discipline_service = _db.obtain_service<dbs_discipline>();
    dbs_review& review_service = _db.obtain_service<dbs_review>();
    dbs_expertise_stats& expertise_stats_service = _db.obtain_service<dbs_expertise_stats>();
    dbs_vote& vote_service = _db.obtain_service<dbs_vote>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();

    try
    {
        FC_ASSERT(o.weight != 0, "Vote weight cannot be 0.");

        const auto& voter = account_service.get_account(o.voter);
        FC_ASSERT(voter.can_vote, "Voter has declined their voting rights.");

        const auto& review = review_service.get(o.review_id);

        const auto& token = expert_token_service.get_expert_token_by_account_and_discipline(o.voter, o.discipline_id);
        const auto& discipline = discipline_service.get_discipline(o.discipline_id);

        // Validate that review has discipline we are trying to vote with
        const auto& find_itr = std::find(review.disciplines.begin(), review.disciplines.end(), o.discipline_id);
        const bool discipline_found = (find_itr != review.disciplines.end());
        FC_ASSERT(discipline_found, "Cannot vote with {d} token as review is not in this discipline",
                  ("d", discipline.name));


        const auto& review_vote_idx = _db._temporary_public_impl().get_index<review_vote_index>().indices()
                .get<by_voter_discipline_and_review>();
        const auto& itr = review_vote_idx.find(std::make_tuple(voter.name, o.discipline_id, o.review_id));

        FC_ASSERT(itr == review_vote_idx.end(), "You have already voted for this review");

        const int64_t elapsed_seconds   = (_db.head_block_time() - token.last_vote_time).to_seconds();

        const int64_t regenerated_power = (DEIP_100_PERCENT * elapsed_seconds) / DEIP_VOTE_REGENERATION_SECONDS;
        const int64_t current_power = std::min(int64_t(token.voting_power + regenerated_power), int64_t(DEIP_100_PERCENT));
        FC_ASSERT(current_power > 0, "Account currently does not have voting power.");

        const int64_t abs_weight = abs(o.weight);
        const int64_t used_power = (current_power * abs_weight) / DEIP_100_PERCENT;
        const int64_t denominated_used_power = used_power / 10;

        FC_ASSERT(used_power <= current_power, "Account does not have enough power to vote.");

        const uint64_t abs_used_tokens = ((uint128_t(token.amount.value) * used_power) / (DEIP_100_PERCENT)).to_uint64();

        _db._temporary_public_impl().modify(token, [&](expert_token_object& t) {
            t.voting_power = current_power - denominated_used_power;
            t.last_vote_time = _db.head_block_time();
        });

        FC_ASSERT(abs_used_tokens > 0, "Cannot vote with 0 tokens.");

        uint64_t max_vote_weight = 0;

        auto& vote = _db._temporary_public_impl().create<review_vote_object>([&](review_vote_object& v) {
            v.voter = voter.name;
            v.discipline_id = o.discipline_id;
            v.review_id = o.review_id;
            v.weight = abs_used_tokens;
            v.voting_time = _db.head_block_time();

            max_vote_weight = v.weight;

            /// discount weight by time
            uint128_t w(max_vote_weight);
            const uint64_t delta_t = std::min(uint64_t((v.voting_time - review.created_at).to_seconds()),
                                              uint64_t(DEIP_REVERSE_AUCTION_WINDOW_SECONDS));

            w *= delta_t;
            w /= DEIP_REVERSE_AUCTION_WINDOW_SECONDS;
            v.weight = w.to_uint64();
        });

        auto current_weight = review.get_evaluation(discipline.id);

        _db._temporary_public_impl().modify(review, [&](review_object& r) {
            r.weights_per_discipline[o.discipline_id] += vote.weight;
        });

        auto weight_modifier = _db.calculate_review_weight_modifier(review.id, token.discipline_id);

        _db._temporary_public_impl().modify(review, [&](review_object& r) {
            r.weight_modifiers[token.discipline_id] = weight_modifier;
        });

        auto new_weight = review.get_evaluation(discipline.id);
        auto weight_delta = new_weight - current_weight;

        auto& total_vote = vote_service.get_total_votes_by_content_and_discipline(review.research_content_id, discipline.id);
        _db._temporary_public_impl().modify(total_vote, [&](total_votes_object& tv) {
            tv.total_weight += weight_delta;
        });

        _db._temporary_public_impl().modify(discipline, [&](discipline_object& d) {
            d.total_active_weight += weight_delta;
        });

        auto& content = _db._temporary_public_impl().get<research_content_object>(review.research_content_id);
        _db._temporary_public_impl().modify(content, [&](research_content_object& rc_o) {
            rc_o.eci_per_discipline[o.discipline_id] += weight_delta;
        });

        research_service.calculate_eci(content.research_id);
        expertise_stats_service.update_used_expertise(abs_used_tokens);
    }
    FC_CAPTURE_AND_RETHROW((o))
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

void create_grant_evaluator::do_apply(const create_grant_operation& op)
{
    dbs_grant& grant_service = _db.obtain_service<dbs_grant>();
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_discipline& discipline_service = _db.obtain_service<dbs_discipline>();
    account_service.check_account_existence(op.owner);
    const auto& owner = account_service.get_account(op.owner);
    discipline_service.check_discipline_existence_by_name(op.target_discipline);
    auto& discipline = discipline_service.get_discipline_by_name(op.target_discipline);
    grant_service.create_grant(owner, op.balance, op.start_block, op.end_block, discipline.id, op.is_extendable, op.content_hash);
}

void create_proposal_evaluator::do_apply(const create_proposal_operation& op)
{
    dbs_proposal& proposal_service = _db.obtain_service<dbs_proposal>();
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();

    account_service.check_account_existence(op.creator);
    research_group_service.check_research_group_token_existence(op.creator, op.research_group_id);
    const uint32_t _lifetime_min = DAYS_TO_SECONDS(1);
    const uint32_t _lifetime_max = DAYS_TO_SECONDS(10);

    const auto& props = _db.get_dynamic_global_properties();

    auto sec_till_expiration = op.expiration_time.sec_since_epoch() - props.time.sec_since_epoch();

    FC_ASSERT(sec_till_expiration <= _lifetime_max && sec_till_expiration >= _lifetime_min,
             "Proposal life time is not in range of ${min} - ${max} seconds. The actual value was ${actual}",
             ("min", _lifetime_min)("max", _lifetime_max)("actual", sec_till_expiration));

    auto& research_group = research_group_service.get_research_group(op.research_group_id);

    deip::protocol::proposal_action_type action = static_cast<deip::protocol::proposal_action_type>(op.action);

    auto quorum_percent = research_group.quorum_percent;
    if (research_group.proposal_quorums.count(action) != 0) {
        quorum_percent  = research_group.proposal_quorums.at(action);
    }
    // the range must be checked in create_proposal_operation::validate()

    if (action == deip::protocol::proposal_action_type::invite_member ||
            action == deip::protocol::proposal_action_type::dropout_member ||
            action == deip::protocol::proposal_action_type::change_quorum ||
            action == deip::protocol::proposal_action_type::rebalance_research_group_tokens)
        FC_ASSERT(!research_group.is_personal,
                  "You cannot invite or dropout member, change quorum and rebalance tokens in personal research group");

    std::hash<std::string> hash_string;
    std::string unique_string = op.data;
    unique_string.erase(std::remove_if(unique_string.begin(),
                                   unique_string.end(),
                                   [](unsigned char x) { return std::isspace(x); }),
                    unique_string.end());
    unique_string += std::to_string(op.action);
    unique_string += std::to_string(op.research_group_id);

    auto hash = hash_string(unique_string);
    auto proposals = proposal_service.get_proposals_by_research_group_id(op.research_group_id);

    for (auto proposal_wrapper : proposals)
    {
        auto& proposal = proposal_wrapper.get();
        FC_ASSERT(hash != proposal.object_hash, "Proposal must be unique within research group");
    }
    auto& proposal = proposal_service.create_proposal(action, op.data, op.creator, op.research_group_id, op.expiration_time, quorum_percent, hash_string(unique_string));

    if (research_group.is_personal)
    {
        auto& proposal_execution_service = _db.obtain_service<dbs_proposal_execution>();
        proposal_execution_service.execute_proposal(proposal);
        proposal_service.complete(proposal);
    }

}

void create_research_group_evaluator::do_apply(const create_research_group_operation& op)
{
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_research_group_invite& research_group_invite_service = _db.obtain_service<dbs_research_group_invite>();
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    bool is_personal = false;

    std::map<uint16_t, share_type> proposal_quorums;

    for (auto& pair : op.proposal_quorums)
        proposal_quorums.insert(std::make_pair(pair.first, pair.second));

    const research_group_object& research_group = research_group_service.create_research_group(op.name,
                                                                                               op.permlink,
                                                                                               op.description,
                                                                                               op.quorum_percent,
                                                                                               proposal_quorums,
                                                                                               is_personal);
    
    research_group_service.create_research_group_token(research_group.id, DEIP_100_PERCENT, op.creator);

    for (const auto& invitee : op.invitees)
    {
        account_service.check_account_existence(invitee.account);
        research_group_invite_service.create(invitee.account, research_group.id, invitee.research_group_tokens_in_percent, invitee.cover_letter, op.creator);
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
    dbs_expertise_stats& expertise_stats_service = _db.obtain_service<dbs_expertise_stats>();
    dbs_vote& votes_service = _db.obtain_service<dbs_vote>();
    dbs_discipline& disciplines_service = _db.obtain_service<dbs_discipline>();

    account_service.check_account_existence(op.author);
    research_content_service.check_research_content_existence(op.research_content_id);
    auto& content = research_content_service.get(op.research_content_id);
    auto& research = research_service.get_research(content.research_id);
    auto reseach_group_tokens = research_group_service.get_research_group_tokens(research.research_group_id);

    for (auto& reseach_group_token : reseach_group_tokens)
        FC_ASSERT(reseach_group_token.get().owner != op.author, "You cannot review your own content");

    auto expertise_tokens = expertise_token_service.get_expert_tokens_by_account_name(op.author);
    auto research_discipline_relations = research_discipline_service.get_research_discipline_relations_by_research(content.research_id);
    std::map<discipline_id_type, share_type> review_disciplines_with_weight;
    std::set<discipline_id_type> review_disciplines;
    std::set<discipline_id_type> research_disciplines_ids;
    for (auto rdr : research_discipline_relations) {
        research_disciplines_ids.insert(rdr.get().discipline_id);
    }

    for (auto expert_token : expertise_tokens)
    {
        auto& token = expert_token.get();
        if (research_disciplines_ids.find(token.discipline_id) != research_disciplines_ids.end())
        {
            const int64_t elapsed_seconds   = (_db.head_block_time() - token.last_vote_time).to_seconds();

            const int64_t regenerated_power = (DEIP_100_PERCENT * elapsed_seconds) / DEIP_VOTE_REGENERATION_SECONDS;
            const int64_t current_power = std::min(int64_t(token.voting_power + regenerated_power), int64_t(DEIP_100_PERCENT));
            FC_ASSERT(current_power > 0, "Account currently does not have voting power.");

            const int64_t used_power = (DEIP_REVIEW_REQUIRED_POWER_PERCENT * op.weight) / DEIP_100_PERCENT;

            FC_ASSERT(used_power <= current_power, "Account does not have enough power to vote.");

            const uint64_t abs_used_tokens = ((uint128_t(token.amount.value) * current_power) / (DEIP_100_PERCENT)).to_uint64();

            _db._temporary_public_impl().modify(token, [&](expert_token_object& t) {
                t.voting_power = current_power - used_power;
                t.last_vote_time = _db.head_block_time();
            });
            review_disciplines_with_weight.insert(std::make_pair(token.discipline_id, abs_used_tokens));
            review_disciplines.insert(token.discipline_id);
        }
    }

    FC_ASSERT(review_disciplines.size() != 0, "Reviewer does not have enough expertise to make review.");

    auto& review = review_service.create(op.research_content_id, op.content, op.is_positive, op.author, review_disciplines);

    for (auto& review_discipline : review_disciplines) {
        auto &token = expertise_token_service.get_expert_token_by_account_and_discipline(op.author, review_discipline);

        auto used_expertise = review_disciplines_with_weight.at(token.discipline_id);

        _db._temporary_public_impl().modify(review, [&](review_object& r) {
            r.expertise_amounts_used[token.discipline_id] = used_expertise;
            r.weights_per_discipline[token.discipline_id] = used_expertise;
            r.weight_modifiers[token.discipline_id] = 1;
        });

        if (votes_service.is_exists_by_content_and_discipline(content.id, token.discipline_id)) {
            auto& total_votes = votes_service.get_total_votes_by_content_and_discipline(content.id, token.discipline_id);

            _db._temporary_public_impl().modify(total_votes, [&](total_votes_object& tv) {
               tv.total_weight += used_expertise;
            });

        } else {
            _db._temporary_public_impl().create<total_votes_object>([&](total_votes_object& tv) {
                tv.discipline_id = token.discipline_id;
                tv.research_content_id = content.id;
                tv.research_id = content.research_id;
                tv.total_weight = used_expertise;
                tv.content_type = content.type;
            });
        }

        auto& discipline = disciplines_service.get_discipline(token.discipline_id);
        _db._temporary_public_impl().modify(discipline, [&](discipline_object& d) {
            d.total_active_weight += used_expertise;
        });

        _db._temporary_public_impl().modify(content, [&](research_content_object& rc_o) { rc_o.eci_per_discipline[review_discipline] += review.get_evaluation(token.discipline_id); });

        research_service.calculate_eci(content.research_id);
        expertise_stats_service.update_used_expertise(used_expertise);
    }
}

void contribute_to_token_sale_evaluator::do_apply(const contribute_to_token_sale_operation& op)
{
    dbs_account &account_service = _db.obtain_service<dbs_account>();
    dbs_research_token_sale &research_token_sale_service = _db.obtain_service<dbs_research_token_sale>();

    research_token_sale_service.check_research_token_sale_existence(op.research_token_sale_id);
    account_service.check_account_existence(op.owner);

    auto& account = account_service.get_account(op.owner);

    FC_ASSERT(account.balance >= op.amount, "Not enough funds to contribute");

    auto& research_token_sale = research_token_sale_service.get_by_id(op.research_token_sale_id);
    FC_ASSERT(research_token_sale.status == research_token_sale_status::token_sale_active, "You cannot contribute to inactive, finished or expired token sale");

    bool is_hard_cap_reached = research_token_sale.total_amount + op.amount >= research_token_sale.hard_cap;

    asset amount_to_contribute = op.amount;
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

    account_service.adjust_balance(account_service.get_account(op.owner), -amount_to_contribute);
    research_token_sale_service.increase_tokens_amount(op.research_token_sale_id, amount_to_contribute);

    if (is_hard_cap_reached) {
        research_token_sale_service.update_status(op.research_token_sale_id, token_sale_finished);
        _db.distribute_research_tokens(op.research_token_sale_id);
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
    dbs_research_group_invite &research_group_invite_service = _db.obtain_service<dbs_research_group_invite>();

    auto& research_group_invite = research_group_invite_service.get(op.research_group_invite_id);

    account_service.check_account_existence(research_group_invite.account_name);
    research_group_service.check_research_group_existence(research_group_invite.research_group_id);

    auto amount = research_group_service.decrease_research_group_tokens_amount(research_group_invite.research_group_id,
                                                                               research_group_invite.research_group_token_amount,
                                                                               research_group_invite.token_source);
    research_group_service.create_research_group_token(research_group_invite.research_group_id,
                                                       amount,
                                                       research_group_invite.account_name);

    _db._temporary_public_impl().remove(research_group_invite);
}

void reject_research_group_invite_evaluator::do_apply(const reject_research_group_invite_operation& op)
{
    dbs_research_group_invite &research_group_invite_service = _db.obtain_service<dbs_research_group_invite>();

    research_group_invite_service.check_research_group_invite_existence(op.research_group_invite_id);

    auto& research_group_invite = research_group_invite_service.get(op.research_group_invite_id);

    _db._temporary_public_impl().remove(research_group_invite);

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

void set_expertise_tokens_evaluator::do_apply(const set_expertise_tokens_operation& op)
{
    dbs_expert_token& expert_token_service = _db.obtain_service<dbs_expert_token>();

    for (auto& discipline_to_add : op.disciplines_to_add)
    {
        FC_ASSERT(discipline_to_add.amount > 0, "Amount must be bigger than 0");
        const bool exist = expert_token_service.expert_token_exists_by_account_and_discipline(op.account_name, discipline_to_add.discipline_id);
        
        if (exist)
        {
            const expert_token_object& expert_token = expert_token_service.get_expert_token_by_account_and_discipline(op.account_name, discipline_to_add.discipline_id);
            _db._temporary_public_impl().modify(expert_token, [&](expert_token_object& et_o) {
                et_o.amount = discipline_to_add.amount;
            });
        } 
        else
            expert_token_service.create(op.account_name, discipline_to_add.discipline_id, discipline_to_add.amount, true);
    }
}

void research_update_evaluator::do_apply(const research_update_operation& op)
{
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    account_service.check_account_existence(op.owner);
    research_service.check_research_existence(op.research_id);

    auto& research = research_service.get_research(op.research_id);
    research_group_service.check_research_group_token_existence(op.owner, research.research_group_id);

    _db._temporary_public_impl().modify(research, [&](research_object& r_o) {
        fc::from_string(r_o.title, op.title);
        fc::from_string(r_o.abstract, op.abstract);
        fc::from_string(r_o.permlink, op.permlink);
    });
}

void create_vesting_balance_evaluator::do_apply(const create_vesting_balance_operation& op)
{
    dbs_vesting_balance& vesting_balance_service = _db.obtain_service<dbs_vesting_balance>();
    dbs_account &account_service = _db.obtain_service<dbs_account>();

    account_service.check_account_existence(op.creator);
    account_service.check_account_existence(op.owner);

    auto& creator = account_service.get_account(op.creator);
    FC_ASSERT(creator.balance >= op.balance, "Not enough funds to create vesting contract");

    account_service.adjust_balance(creator, -op.balance);
    vesting_balance_service.create(op.owner, op.balance, op.vesting_duration_seconds, op.period_duration_seconds, op.vesting_cliff_seconds);

}

void withdraw_vesting_balance_evaluator::do_apply(const withdraw_vesting_balance_operation& op)
{
    dbs_vesting_balance& vesting_balance_service = _db.obtain_service<dbs_vesting_balance>();
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    account_service.check_account_existence(op.owner);
    vesting_balance_service.check_existence(op.vesting_balance_id);

    const auto& vco = vesting_balance_service.get(op.vesting_balance_id);
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
            account_service.adjust_balance(_db.get_account(op.owner), op.amount);
        }
    }
}

void vote_proposal_evaluator::do_apply(const vote_proposal_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_proposal& proposal_service = _db.obtain_service<dbs_proposal>();
    dbs_proposal_execution& proposal_execution_service = _db.obtain_service<dbs_proposal_execution>();

    research_group_service.check_research_group_token_existence(op.voter, op.research_group_id);
    account_service.check_account_existence(op.voter);
    proposal_service.check_proposal_existence(op.proposal_id);

    const proposal_object& proposal = proposal_service.get_proposal(op.proposal_id);

    FC_ASSERT(proposal.voted_accounts.find(op.voter) == proposal.voted_accounts.end(),
              "Account \"${account}\" already voted", ("account", op.voter));

    FC_ASSERT(!proposal_service.is_expired(proposal), "Proposal '${id}' is expired.", ("id", op.proposal_id));
    FC_ASSERT(!proposal.is_completed, "Proposal '${id}' is approved already.", ("id", op.proposal_id));

    proposal_service.vote_for(op.proposal_id, op.voter);

    share_type total_voted_weight = 0;
    auto& votes = proposal_service.get_votes_for(proposal.id);
    for (const proposal_vote_object& vote : votes) {
        auto& rg_token = research_group_service.get_token_by_account_and_research_group(vote.voter, vote.research_group_id);
        total_voted_weight += rg_token.amount.value;
    }

    if (total_voted_weight  >= proposal.quorum_percent)
    {
        proposal_execution_service.execute_proposal(proposal);
        proposal_service.complete(proposal);
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
        research_token_service.create_research_token(op.receiver, op.amount, op.research_id);
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

    expert_token_service.check_expert_token_existence_by_account_and_discipline(op.sender, op.discipline_id);
    expert_token_service.check_expert_token_existence_by_account_and_discipline(op.receiver, op.discipline_id);

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
    expert_token_service.check_expert_token_existence_by_account_and_discipline(op.sender, op.discipline_id);

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

    expert_token_service.check_expert_token_existence_by_account_and_discipline(op.voter, proposal.discipline_id);
    auto& expert_token = expert_token_service.get_expert_token_by_account_and_discipline(op.voter, proposal.discipline_id);

    FC_ASSERT(expert_token.proxy.size() == 0, "A proxy is currently set, please clear the proxy before voting.");


    if (op.voting_power == DEIP_100_PERCENT)
        expertise_allocation_proposal_service.upvote(proposal, op.voter, expert_token.amount + expert_token.proxied_expertise_total());
    else if (op.voting_power == -DEIP_100_PERCENT)
        expertise_allocation_proposal_service.downvote(proposal, op.voter, expert_token.amount + expert_token.proxied_expertise_total());

}

void accept_research_token_offer_evaluator::do_apply(const accept_research_token_offer_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_offer_research_tokens& offer_service = _db.obtain_service<dbs_offer_research_tokens>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_research_token& research_token_service = _db.obtain_service<dbs_research_token>();

    account_service.check_account_existence(op.buyer);
    offer_service.check_offer_existence(op.offer_research_tokens_id);

    auto& buyer = account_service.get_account(op.buyer);
    auto& offer = offer_service.get(op.offer_research_tokens_id);
    auto& research = research_service.get_research(offer.research_id);

    FC_ASSERT(research.owned_tokens >= offer.amount, "Research group doesn't have enough tokens");
    FC_ASSERT(buyer.balance.amount >= offer.price.amount, "Buyer doesn't have enough funds.");

    research_service.decrease_owned_tokens(research, offer.amount);
    account_service.adjust_balance(buyer, -offer.price);

    research_token_service.create_research_token(op.buyer, offer.amount, offer.research_id);

    _db._temporary_public_impl().remove(offer);
}

void reject_research_token_offer_evaluator::do_apply(const reject_research_token_offer_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_offer_research_tokens& offer_service = _db.obtain_service<dbs_offer_research_tokens>();

    account_service.check_account_existence(op.buyer);
    offer_service.check_offer_existence(op.offer_research_tokens_id);

    auto& offer = offer_service.get(op.offer_research_tokens_id);

    _db._temporary_public_impl().remove(offer);
}

void adjust_account_balance_evaluator::do_apply(const adjust_account_balance_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();

    account_service.check_account_existence(op.account);
    auto& account = account_service.get_account(op.account);

    FC_ASSERT(account.balance + op.delta >= asset(0, DEIP_SYMBOL), "Account does not have sufficient funds for adjust.");

    account_service.adjust_balance(account, op.delta);
}

void request_review_evaluator::do_apply(const request_review_operation& op)
{
    dbs_account& account_service = _db.obtain_service<dbs_account>();
    dbs_research& research_service = _db.obtain_service<dbs_research>();
    dbs_research_content& research_content_service = _db.obtain_service<dbs_research_content>();
    dbs_review& review_service = _db.obtain_service<dbs_review>();

    account_service.check_account_existence(op.requester);
    research_service.check_research_existence(op.research_id);

    for (auto& account : op.accounts_list)
        account_service.check_account_existence(account);

    auto content_list = research_content_service.get_by_research_and_type(op.research_id, research_content_type::announcement);
    auto& content = content_list.front().get();

    dbs_research_group& research_group_service = _db.obtain_service<dbs_research_group>();
    dbs_research_discipline_relation& research_discipline_service = _db.obtain_service<dbs_research_discipline_relation>();
    dbs_expert_token& expertise_token_service = _db.obtain_service<dbs_expert_token>();
    dbs_expertise_stats& expertise_stats_service = _db.obtain_service<dbs_expertise_stats>();
    dbs_vote& votes_service = _db.obtain_service<dbs_vote>();
    dbs_discipline& disciplines_service = _db.obtain_service<dbs_discipline>();

    auto& research = research_service.get_research(content.research_id);
    auto reseach_group_tokens = research_group_service.get_research_group_tokens(research.research_group_id);

    for (auto& account : op.accounts_list)
    {
        auto expertise_tokens = expertise_token_service.get_expert_tokens_by_account_name(account);
        auto research_discipline_relations = research_discipline_service.get_research_discipline_relations_by_research(content.research_id);

        std::map<discipline_id_type, share_type> review_disciplines_with_weight;
        std::set<discipline_id_type> review_disciplines;
        std::set<discipline_id_type> research_disciplines_ids;
        for (auto rdr : research_discipline_relations) {
            research_disciplines_ids.insert(rdr.get().discipline_id);
        }

        for (auto expert_token : expertise_tokens)
        {
            auto& token = expert_token.get();
            if (research_disciplines_ids.find(token.discipline_id) != research_disciplines_ids.end())
            {
                const int64_t elapsed_seconds   = (_db.head_block_time() - token.last_vote_time).to_seconds();

                const int64_t regenerated_power = (DEIP_100_PERCENT * elapsed_seconds) / DEIP_VOTE_REGENERATION_SECONDS;
                const int64_t current_power = std::min(int64_t(token.voting_power + regenerated_power), int64_t(DEIP_100_PERCENT));
                FC_ASSERT(current_power > 0, "Account currently does not have voting power.");

                const int64_t used_power = (DEIP_REVIEW_REQUIRED_POWER_PERCENT * DEIP_100_PERCENT) / DEIP_100_PERCENT;

                FC_ASSERT(used_power <= current_power, "Account does not have enough power to vote.");

                const uint64_t abs_used_tokens = ((uint128_t(token.amount.value) * current_power) / (DEIP_100_PERCENT)).to_uint64();

                _db._temporary_public_impl().modify(token, [&](expert_token_object& t) {
                    t.voting_power = current_power - used_power;
                    t.last_vote_time = _db.head_block_time();
                });
                review_disciplines_with_weight.insert(std::make_pair(token.discipline_id, abs_used_tokens));
                review_disciplines.insert(token.discipline_id);
            }
        }

        if (review_disciplines.size() == 0)
        {
            ilog("Reviewer does not have enough expertise to make review.");
            continue;
        }

        std::string content_string = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vestibulum lobortis elementum. "
                                     "Integer pulvinar massa a scelerisque volutpat. Sed erat arcu, porttitor in vehicula quis, laoreet ut lectus. "
                                     "Quisque venenatis aliquet lacus sed finibus. In id ex risus. In ultrices varius quam a interdum. "
                                     "Duis a erat vitae urna sagittis malesuada. Vestibulum pharetra porta feugiat. "
                                     "Pellentesque semper hendrerit ligula vitae finibus. Morbi vel porttitor nulla, nec mollis elit. "
                                     "In placerat justo fringilla, euismod nibh et, placerat sapien. In imperdiet tortor eget varius consectetur. "
                                     "Sed posuere tincidunt erat, non suscipit nisl laoreet a. Maecenas congue maximus diam, et lobortis quam hendrerit rhoncus. "
                                     "Ut nulla nisl, posuere ac pharetra at, varius ac erat.";

        auto& review = review_service.create(content.id, content_string, true, account, review_disciplines);


        for (auto& review_discipline : review_disciplines) {
            auto &token = expertise_token_service.get_expert_token_by_account_and_discipline(account, review_discipline);

            auto used_expertise = review_disciplines_with_weight.at(token.discipline_id);

            _db._temporary_public_impl().modify(review, [&](review_object& r) {
                r.expertise_amounts_used[token.discipline_id] = used_expertise;
                r.weights_per_discipline[token.discipline_id] = used_expertise;
                r.weight_modifiers[token.discipline_id] = 1;
            });

            if (votes_service.is_exists_by_content_and_discipline(content.id, token.discipline_id)) {
                auto& total_votes = votes_service.get_total_votes_by_content_and_discipline(content.id, token.discipline_id);

                _db._temporary_public_impl().modify(total_votes, [&](total_votes_object& tv) {
                    tv.total_weight += used_expertise;
                });

            } else {
                _db._temporary_public_impl().create<total_votes_object>([&](total_votes_object& tv) {
                    tv.discipline_id = token.discipline_id;
                    tv.research_content_id = content.id;
                    tv.research_id = content.research_id;
                    tv.total_weight = used_expertise;
                    tv.content_type = content.type;
                });
            }

            auto& discipline = disciplines_service.get_discipline(token.discipline_id);
            _db._temporary_public_impl().modify(discipline, [&](discipline_object& d) {
                d.total_active_weight += used_expertise;
            });

            _db._temporary_public_impl().modify(content, [&](research_content_object& rc_o) {
                rc_o.eci_per_discipline[review_discipline] += review.get_evaluation(token.discipline_id);
            });

            research_service.calculate_eci(content.research_id);
            expertise_stats_service.update_used_expertise(used_expertise);
        }

    }
}

} // namespace chain
} // namespace deip 
