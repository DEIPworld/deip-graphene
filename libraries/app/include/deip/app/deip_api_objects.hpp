#pragma once
#include <deip/chain/schema/account_object.hpp>
#include <deip/chain/schema/block_summary_object.hpp>
#include <deip/chain/schema/global_property_object.hpp>
//#include <deip/chain/history_object.hpp>
#include <deip/chain/schema/award_object.hpp>
#include <deip/chain/schema/award_research_relation_object.hpp>
#include <deip/chain/schema/account_balance_object.hpp>
#include <deip/chain/schema/asset_object.hpp>
#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/schema/discipline_object.hpp>
#include <deip/chain/schema/discipline_supply_object.hpp>
#include <deip/chain/schema/expert_token_object.hpp>
#include <deip/chain/schema/expertise_allocation_proposal_object.hpp>
#include <deip/chain/schema/expertise_allocation_proposal_vote_object.hpp>
#include <deip/chain/schema/proposal_object.hpp>
#include <deip/chain/schema/proposal_vote_object.hpp>
#include <deip/chain/schema/research_content_object.hpp>
#include <deip/chain/schema/research_discipline_relation_object.hpp>
#include <deip/chain/schema/research_group_invite_object.hpp>
#include <deip/chain/schema/research_group_object.hpp>
#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/schema/research_token_object.hpp>
#include <deip/chain/schema/research_token_sale_object.hpp>
#include <deip/chain/schema/review_object.hpp>
#include <deip/chain/schema/review_vote_object.hpp>
#include <deip/chain/schema/total_votes_object.hpp>
#include <deip/chain/schema/transaction_object.hpp>
#include <deip/chain/schema/vesting_balance_object.hpp>
#include <deip/chain/schema/offer_research_tokens_object.hpp>
#include <deip/chain/schema/grant_object.hpp>
#include <deip/chain/schema/grant_application_object.hpp>
#include <deip/chain/schema/grant_application_review_object.hpp>
#include <deip/chain/schema/grant_application_review_object.hpp>
#include <deip/chain/schema/funding_opportunity_object.hpp>
#include <deip/chain/schema/vote_object.hpp>
#include <deip/chain/schema/witness_objects.hpp>
#include <deip/witness/witness_objects.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace app {

using namespace deip::chain;
using research_group_token_refs_type = std::vector<std::reference_wrapper<const research_group_token_object>>;
using deip::protocol::percent_type;

typedef chain::change_recovery_account_request_object change_recovery_account_request_api_obj;
typedef chain::block_summary_object block_summary_api_obj;
typedef chain::withdraw_common_tokens_route_object withdraw_common_tokens_route_api_obj;
typedef chain::witness_vote_object witness_vote_api_obj;
typedef chain::witness_schedule_object witness_schedule_api_obj;
typedef chain::reward_fund_object reward_fund_api_obj;
typedef witness::account_bandwidth_object account_bandwidth_api_obj;

struct account_api_obj
{
    account_api_obj(const chain::account_object& a, const chain::database& db)
        : id(a.id)
        , name(a.name)
        , memo_key(a.memo_key)
        , json_metadata(fc::to_string(a.json_metadata))
        , proxy(a.proxy)
        , last_account_update(a.last_account_update)
        , created(a.created)
        , mined(a.mined)
        , recovery_account(a.recovery_account)
        , last_account_recovery(a.last_account_recovery)
        , lifetime_vote_count(a.lifetime_vote_count)
        , can_vote(a.can_vote)
        , common_tokens_balance(a.common_tokens_balance)
        , expert_tokens_balance(a.expertise_tokens_balance)
        , common_tokens_withdraw_rate(a.common_tokens_withdraw_rate)
        , next_common_tokens_withdrawal(a.next_common_tokens_withdrawal)
        , withdrawn(a.withdrawn)
        , to_withdraw(a.to_withdraw)
        , withdraw_routes(a.withdraw_routes)
        , witnesses_voted_for(a.witnesses_voted_for)
    {
        size_t n = a.proxied_vsf_votes.size();
        proxied_vsf_votes.reserve(n);
        for (size_t i = 0; i < n; i++)
            proxied_vsf_votes.push_back(a.proxied_vsf_votes[i]);

        const auto& auth = db.get<account_authority_object, by_account>(name);
        owner = authority(auth.owner);
        active = authority(auth.active);
        posting = authority(auth.posting);
        last_owner_update = auth.last_owner_update;

        if (db.has_index<witness::account_bandwidth_index>())
        {
            auto forum_bandwidth = db.find<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
                boost::make_tuple(name, witness::bandwidth_type::forum));

            if (forum_bandwidth != nullptr)
            {
                average_bandwidth = forum_bandwidth->average_bandwidth;
                lifetime_bandwidth = forum_bandwidth->lifetime_bandwidth;
                last_bandwidth_update = forum_bandwidth->last_bandwidth_update;
            }

            auto market_bandwidth = db.find<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
                boost::make_tuple(name, witness::bandwidth_type::market));

            if (market_bandwidth != nullptr)
            {
                average_market_bandwidth = market_bandwidth->average_bandwidth;
                lifetime_market_bandwidth = market_bandwidth->lifetime_bandwidth;
                last_market_bandwidth_update = market_bandwidth->last_bandwidth_update;
            }
        }

        auto it_pair = db.get_index<account_balance_index>().indicies().get<by_owner>().equal_range(a.name);
        auto it = it_pair.first;
        const auto it_end = it_pair.second;
        while (it != it_end)
        {
            const account_balance_object& account_balance = std::cref(*it).get();
            balances.push_back(asset(account_balance.amount, account_balance.symbol));
            ++it;
        }
    }

    account_api_obj()
    {
    }

    account_id_type id;

    account_name_type name;
    authority owner;
    authority active;
    authority posting;
    public_key_type memo_key;
    string json_metadata;
    account_name_type proxy;

    time_point_sec last_owner_update;
    time_point_sec last_account_update;

    time_point_sec created;
    bool mined = false;
    account_name_type recovery_account;
    time_point_sec last_account_recovery;
    uint32_t lifetime_vote_count = 0;

    bool can_vote = false;

    share_type common_tokens_balance;
    share_type expert_tokens_balance;

    share_type received_common_tokens;
    share_type common_tokens_withdraw_rate;
    time_point_sec next_common_tokens_withdrawal;
    share_type withdrawn;
    share_type to_withdraw;
    uint16_t withdraw_routes = 0;

    vector<share_type> proxied_vsf_votes;

    uint16_t witnesses_voted_for;

    share_type average_bandwidth = 0;
    share_type lifetime_bandwidth = 0;
    time_point_sec last_bandwidth_update;

    share_type average_market_bandwidth = 0;
    share_type lifetime_market_bandwidth = 0;
    time_point_sec last_market_bandwidth_update;

    vector<asset> balances;
};

struct owner_authority_history_api_obj
{
    owner_authority_history_api_obj(const chain::owner_authority_history_object& o)
        : id(o.id)
        , account(o.account)
        , previous_owner_authority(authority(o.previous_owner_authority))
        , last_valid_time(o.last_valid_time)
    {
    }

    owner_authority_history_api_obj()
    {
    }

    owner_authority_history_id_type id;

    account_name_type account;
    authority previous_owner_authority;
    time_point_sec last_valid_time;
};

struct account_recovery_request_api_obj
{
    account_recovery_request_api_obj(const chain::account_recovery_request_object& o)
        : id(o.id)
        , account_to_recover(o.account_to_recover)
        , new_owner_authority(authority(o.new_owner_authority))
        , expires(o.expires)
    {
    }

    account_recovery_request_api_obj()
    {
    }

    account_recovery_request_id_type id;
    account_name_type account_to_recover;
    authority new_owner_authority;
    time_point_sec expires;
};

struct account_history_api_obj
{
};

struct witness_api_obj
{
    witness_api_obj(const chain::witness_object& w)
        : id(w.id)
        , owner(w.owner)
        , created(w.created)
        , url(fc::to_string(w.url))
        , total_missed(w.total_missed)
        , last_aslot(w.last_aslot)
        , last_confirmed_block_num(w.last_confirmed_block_num)
        , signing_key(w.signing_key)
        , props(w.props)
        , sbd_exchange_rate(w.sbd_exchange_rate)
        , last_sbd_exchange_update(w.last_sbd_exchange_update)
        , votes(w.votes)
        , virtual_last_update(w.virtual_last_update)
        , virtual_position(w.virtual_position)
        , virtual_scheduled_time(w.virtual_scheduled_time)
        , last_work(w.last_work)
        , running_version(w.running_version)
        , hardfork_version_vote(w.hardfork_version_vote)
        , hardfork_time_vote(w.hardfork_time_vote)
    {
    }

    witness_api_obj()
    {
    }

    witness_id_type id;
    account_name_type owner;
    time_point_sec created;
    string url;
    uint32_t total_missed = 0;
    uint64_t last_aslot = 0;
    uint64_t last_confirmed_block_num = 0;
    public_key_type signing_key;
    chain_properties props;
    price sbd_exchange_rate;
    time_point_sec last_sbd_exchange_update;
    share_type votes;
    fc::uint128 virtual_last_update;
    fc::uint128 virtual_position;
    fc::uint128 virtual_scheduled_time;
    digest_type last_work;
    version running_version;
    hardfork_version hardfork_version_vote;
    time_point_sec hardfork_time_vote;
};

struct signed_block_api_obj : public signed_block
{
    signed_block_api_obj(const signed_block& block)
        : signed_block(block)
    {
        block_id = id();
        signing_key = signee();
        transaction_ids.reserve(transactions.size());
        for (const signed_transaction& tx : transactions)
            transaction_ids.push_back(tx.id());
    }
    signed_block_api_obj()
    {
    }

    block_id_type block_id;
    public_key_type signing_key;
    vector<transaction_id_type> transaction_ids;
};

struct dynamic_global_property_api_obj : public dynamic_global_property_object
{
    dynamic_global_property_api_obj(const dynamic_global_property_object& gpo, const chain::database& db)
        : dynamic_global_property_object(gpo)
    {
        if (db.has_index<witness::reserve_ratio_index>())
        {
            const auto& r = db.find(witness::reserve_ratio_id_type());

            if (BOOST_LIKELY(r != nullptr))
            {
                current_reserve_ratio = r->current_reserve_ratio;
                average_block_size = r->average_block_size;
                max_virtual_bandwidth = r->max_virtual_bandwidth;
            }
        }
    }

    dynamic_global_property_api_obj(const dynamic_global_property_object& gpo)
        : dynamic_global_property_object(gpo)
    {
    }

    dynamic_global_property_api_obj()
    {
    }

    uint32_t current_reserve_ratio = 0;
    uint64_t average_block_size = 0;
    uint128_t max_virtual_bandwidth = 0;
};

struct discipline_supply_api_obj
{
    discipline_supply_api_obj(const chain::discipline_supply_object& ds_o)
        : id(ds_o.id._id)
        , grantor(ds_o.grantor)
        , target_discipline(ds_o.target_discipline._id)
        , created(ds_o.created)
        , balance(ds_o.balance)
        , per_block(ds_o.per_block)
        , start_time(ds_o.start_time)
        , end_time(ds_o.end_time)
        , is_extendable(ds_o.is_extendable)
        , content_hash(fc::to_string(ds_o.content_hash))
    {
        for (auto& pair : ds_o.additional_info)
        {
            std::string key = fc::to_string(pair.first);
            std::string val = fc::to_string(pair.second);
            additional_info.insert(std::pair<string, string>(key, val));
        }
    }

    // because fc::variant require for temporary object
    discipline_supply_api_obj()
    {
    }

    int64_t id;

    account_name_type grantor;
    int64_t target_discipline;

    time_point_sec created;

    asset balance;
    share_type per_block;
    time_point_sec start_time;
    time_point_sec end_time;

    bool is_extendable;
    string content_hash;

    std::map<std::string, std::string> additional_info;
};

struct discipline_api_obj
{
    discipline_api_obj(const chain::discipline_object& d)
        : id(d.id._id)
        ,  parent_id(d.parent_id._id)
        ,  name(fc::to_string(d.name))
        ,  total_active_weight(d.total_active_weight)
        ,  total_expertise_amount(d.total_expertise_amount)
    {}

    // because fc::variant require for temporary object
    discipline_api_obj()
    {
    }

    int64_t id;
    int64_t parent_id;
    std::string name;
    share_type total_active_weight;
    share_type total_expertise_amount;
};

struct research_api_obj
{
    research_api_obj(const chain::research_object& r, const vector<discipline_api_obj>& disciplines, const string& group_permlink)
        : id(r.id._id)
        ,  research_group_id(r.research_group_id._id)
        ,  title(fc::to_string(r.title))
        ,  abstract(fc::to_string(r.abstract))
        ,  permlink(fc::to_string(r.permlink))
        ,  is_finished(r.is_finished)
        ,  owned_tokens(r.owned_tokens)
        ,  review_share_in_percent(r.review_share_in_percent)
        ,  created_at(r.created_at)
        ,  dropout_compensation_in_percent(r.dropout_compensation_in_percent)
        ,  disciplines(disciplines.begin(), disciplines.end())
        ,  group_permlink(group_permlink)
        ,  number_of_positive_reviews(r.number_of_positive_reviews)
        ,  number_of_negative_reviews(r.number_of_negative_reviews)
        ,  last_update_time(r.last_update_time)
        ,  contents_amount(r.contents_amount)
        ,  members(r.members.begin(), r.members.end())
        ,  is_private(r.is_private)
    {
        for (const auto& kvp : r.eci_per_discipline) {
            discipline_id_type discipline_id = kvp.first;
            share_type weight = kvp.second;
            eci_per_discipline.emplace(std::make_pair(discipline_id._id, weight.value));
        }
    }

    // because fc::variant require for temporary object
    research_api_obj()
    {
    }

    int64_t id;
    int64_t research_group_id;
    std::string title;
    std::string abstract;
    std::string permlink;
    bool is_finished;
    share_type owned_tokens;
    uint16_t review_share_in_percent;
    time_point_sec created_at;
    int16_t dropout_compensation_in_percent;
    vector<discipline_api_obj> disciplines;
    string group_permlink;

    map<int64_t, int64_t> eci_per_discipline;

    uint16_t number_of_positive_reviews;
    uint16_t number_of_negative_reviews;

    time_point_sec last_update_time;
    uint16_t contents_amount;

    std::vector<account_name_type> members;

    bool is_private;
};

struct research_content_api_obj
{
    research_content_api_obj(const chain::research_content_object& rc)
        : id(rc.id._id)
        ,  research_id(rc.research_id._id)
        ,  content_type(rc.type)
        ,  authors(rc.authors.begin(), rc.authors.end())
        ,  title(fc::to_string(rc.title))        
        ,  content(fc::to_string(rc.content))
        ,  permlink(fc::to_string(rc.permlink))
        ,  activity_state(rc.activity_state)
        ,  activity_window_start(rc.activity_window_start)
        ,  activity_window_end(rc.activity_window_end)
        ,  created_at(rc.created_at)
    {
        for (auto reference : rc.references)
        {
            references.insert(reference._id);
        }

        for (auto& str : rc.external_references)
        {
            std::string val = fc::to_string(str);
            external_references.insert(val);
        }

        for (const auto& kvp : rc.eci_per_discipline) 
        {
            discipline_id_type discipline_id = kvp.first;
            share_type weight = kvp.second;
            eci_per_discipline.emplace(std::make_pair(discipline_id._id, weight.value));
        }
    }

    // because fc::variant require for temporary object
    research_content_api_obj()
    {
    }

    int64_t id;
    int64_t research_id;
    research_content_type content_type;
    std::set<account_name_type> authors;
    std::string title;
    std::string content;
    std::string permlink;
    research_content_activity_state activity_state;
    fc::time_point_sec activity_window_start;
    fc::time_point_sec activity_window_end;
    fc::time_point_sec created_at;

    std::set<string> external_references;
    std::set<int64_t> references;

    map<int64_t, int64_t> eci_per_discipline;
};

struct expert_token_api_obj
{
    expert_token_api_obj(const chain::expert_token_object& d, const string& discipline_name)
        : id(d.id._id)
        ,  account_name(d.account_name)
        ,  discipline_id(d.discipline_id._id)
        ,  discipline_name(discipline_name)
        ,  amount(d.amount)
    {}

    // because fc::variant require for temporary object
    expert_token_api_obj()
    {
    }

    int64_t id;
    string account_name;
    int64_t discipline_id;
    string discipline_name;
    share_type amount;
};

struct proposal_vote_api_obj
{
    proposal_vote_api_obj(const chain::proposal_vote_object& pv)
        : id(pv.id._id)
        ,  proposal_id(pv.proposal_id._id)
        ,  research_group_id(pv.research_group_id._id)
        ,  voting_time(pv.voting_time)
        ,  voter(pv.voter)
    {}

    // because fc::variant require for temporary object
    proposal_vote_api_obj()
    {
    }

    int64_t id;
    int64_t proposal_id;
    int64_t research_group_id;
    fc::time_point_sec voting_time;
    account_name_type voter;
};

struct proposal_api_obj
{
    proposal_api_obj(const chain::proposal_object& p, const vector<proposal_vote_api_obj>& _votes)
        : id(p.id._id)
        , research_group_id(p.research_group_id._id)
        , action(p.action)
        , creation_time(p.creation_time)
        , expiration_time(p.expiration_time)
        , creator(p.creator)
        , data(fc::to_string(p.data))
        , quorum_percent(p.quorum)
        , current_votes_amount(p.current_votes_amount)
        , is_completed(p.is_completed)
        , voted_accounts(p.voted_accounts.begin(), p.voted_accounts.end())
        , votes(_votes)
    {}

    // because fc::variant require for temporary object
    proposal_api_obj()
    {
    }

    int64_t id;
    int64_t research_group_id;
    int8_t action;
    fc::time_point_sec creation_time;
    fc::time_point_sec expiration_time;
    std::string creator;
    std::string data;
    percent_type quorum_percent;
    share_type current_votes_amount;
    bool is_completed;

    flat_set<account_name_type> voted_accounts;
    vector<proposal_vote_api_obj> votes;
};

struct research_group_token_api_obj
{
    research_group_token_api_obj(const chain::research_group_token_object& rgt)
        : id(rgt.id._id)
        ,  research_group_id(rgt.research_group_id._id)
        ,  amount(rgt.amount.value)
        ,  owner(rgt.owner)
    {}

    // because fc::variant require for temporary object
    research_group_token_api_obj()
    {
    }

    int64_t id;
    int64_t research_group_id;
    uint32_t amount;
    account_name_type owner;
};

struct research_group_api_obj
{
    research_group_api_obj(const chain::research_group_object& rg)
        :  id(rg.id._id)
        ,  creator(rg.creator)
        ,  name(fc::to_string(rg.name))
        ,  permlink(fc::to_string(rg.permlink))
        ,  description(fc::to_string(rg.description))
        ,  quorum_percent(rg.default_quorum)
        ,  is_dao(rg.is_dao)
        ,  is_personal(rg.is_personal)
        ,  is_centralized(rg.is_centralized)
        ,  balance(rg.balance)
    {
        proposal_quorums.insert(rg.action_quorums.begin(), rg.action_quorums.end());
    }

    // because fc::variant require for temporary object
    research_group_api_obj()
    {
    }

    int64_t id;
    account_name_type creator;
    std::string name;
    std::string permlink;
    std::string description;
    percent_type quorum_percent;
    std::map<research_group_quorum_action, percent_type> proposal_quorums;
    bool is_dao;
    bool is_personal;
    bool is_centralized;
    asset balance;
};

struct research_token_sale_api_obj
{
    research_token_sale_api_obj(const chain::research_token_sale_object& rts)
        : id(rts.id._id)
        ,  research_id(rts.research_id._id)
        ,  start_time(rts.start_time)
        ,  end_time(rts.end_time)
        ,  total_amount(rts.total_amount)
        ,  balance_tokens(rts.balance_tokens)
        ,  soft_cap(rts.soft_cap)
        ,  hard_cap(rts.hard_cap)
        ,  status(rts.status)
    {}

    // because fc::variant require for temporary object
    research_token_sale_api_obj()
    {
    }

    int64_t id;
    int64_t research_id;
    time_point_sec start_time;
    time_point_sec end_time;
    asset total_amount;
    share_type balance_tokens;
    asset soft_cap;
    asset hard_cap;
    uint16_t status;
};

struct research_token_sale_contribution_api_obj
{
    research_token_sale_contribution_api_obj(const chain::research_token_sale_contribution_object& co)
        : id(co.id._id)
        ,  research_token_sale_id(co.research_token_sale_id._id)
        ,  owner(co.owner)
        ,  amount(co.amount)
        ,  contribution_time(co.contribution_time)
    {}

    // because fc::variant require for temporary object
    research_token_sale_contribution_api_obj()
    {
    }

    int64_t id;
    int64_t research_token_sale_id;
    account_name_type owner;
    asset amount;
    time_point_sec contribution_time;
};

struct research_discipline_relation_api_obj
{
    research_discipline_relation_api_obj(const chain::research_discipline_relation_object& re)
            : id(re.id._id)
            ,  research_id(re.research_id._id)
            ,  discipline_id(re.discipline_id._id)
            ,  votes_count(re.votes_count)
            ,  research_eci(re.research_eci)
    {}
    // because fc::variant require for temporary object
    research_discipline_relation_api_obj()
    {
    }

    int64_t id;
    int64_t research_id;
    int64_t discipline_id;
    uint16_t votes_count;
    share_type research_eci;
};

struct research_group_invite_api_obj
{
    research_group_invite_api_obj(const chain::research_group_invite_object& invite)
        : id(invite.id._id)
        , account_name(invite.account_name)
        , research_group_id(invite.research_group_id._id)
        , research_group_token_amount(invite.research_group_token_amount)
        , cover_letter(fc::to_string(invite.cover_letter))
        , is_head(invite.is_head)
    {}

    // because fc::variant require for temporary object
    research_group_invite_api_obj()
    {
    }

    int64_t id;
    account_name_type account_name;
    int64_t research_group_id;
    share_type research_group_token_amount;
    std::string cover_letter;
    bool is_head;
};

struct research_listing_api_obj
{
    research_listing_api_obj(const research_api_obj& r,
                             const research_group_api_obj& rg,
                             const vector<account_name_type>& group_members,
                             const int64_t& votes_count)
        : research_id(r.id)
        , title(r.title)
        , abstract(r.abstract)
        , permlink(r.permlink)
        , owned_tokens(r.owned_tokens)
        , review_share_in_percent(r.review_share_in_percent)
        , created_at(r.created_at)
        , group_members(group_members.begin(), group_members.end())
        , disciplines(r.disciplines.begin(), r.disciplines.end())
        , votes_count(votes_count)
        , group_id(rg.id)
        , group_permlink(rg.permlink)
        , last_update_time(r.last_update_time)
        , contents_amount(r.contents_amount)
        , members(r.members.begin(), r.members.end())
        , number_of_positive_reviews(r.number_of_positive_reviews)
        , number_of_negative_reviews(r.number_of_negative_reviews)
        , is_private(r.is_private)
    {
        for (const auto& kvp : r.eci_per_discipline)
        {
            discipline_id_type discipline_id = kvp.first;
            share_type weight = kvp.second;
            eci_per_discipline.emplace(std::make_pair(discipline_id._id, weight.value));
        }
    }

    // because fc::variant require for temporary object
    research_listing_api_obj()
    {
    }

    int64_t research_id;
    string title;
    string abstract;
    string permlink;
    share_type owned_tokens;
    uint16_t review_share_in_percent;
    time_point_sec created_at;
    vector<account_name_type> group_members;
    vector<discipline_api_obj> disciplines;
    int64_t votes_count;
    int64_t group_id;
    string group_permlink;
    map<int64_t, int64_t> eci_per_discipline;
    time_point_sec last_update_time;
    uint16_t contents_amount;
    std::vector<account_name_type> members;
    uint16_t number_of_positive_reviews;
    uint16_t number_of_negative_reviews;
    bool is_private;
};

struct total_votes_api_obj
{
    total_votes_api_obj(const chain::total_votes_object& vo)
        : id(vo.id._id)
        ,  discipline_id(vo.discipline_id._id)
        ,  research_id(vo.research_id._id)
        ,  research_content_id(vo.research_content_id._id)
        ,  total_weight(vo.total_weight)
    {}

    // because fc::variant require for temporary object
    total_votes_api_obj()
    {
    }

    int64_t id;
    int64_t discipline_id;
    int64_t research_id;
    int64_t research_content_id;

    share_type total_weight;
};

struct review_api_obj
{
    review_api_obj(const chain::review_object& r, const vector<discipline_api_obj>& disciplines)
            : id(r.id._id)
            , research_content_id(r.research_content_id._id)
            , content(fc::to_string(r.content))
            , is_positive(r.is_positive)
            , author(r.author)
            , created_at(r.created_at)
            , assessment_model_v(r.assessment_model_v)
            , scores(r.scores.begin(), r.scores.end())
    {
        this->disciplines = disciplines;

        for (const auto& d : disciplines) {
            auto& discipline_id = d.id;

            this->expertise_tokens_amount_by_discipline.emplace(std::make_pair(discipline_id, r.expertise_tokens_amount_by_discipline.at(discipline_id).value));
        }
    }

    // because fc::variant require for temporary object
    review_api_obj()
    {
    }

    int64_t id;
    int64_t research_content_id;
    string content;
    bool is_positive;
    account_name_type author;
    time_point_sec created_at;
    vector<discipline_api_obj> disciplines;
    map<int64_t, int64_t> expertise_tokens_amount_by_discipline;
    int32_t assessment_model_v;
    map<uint16_t, uint16_t> scores;
};

struct research_token_api_obj
{
    research_token_api_obj(const chain::research_token_object& rt)
        : id(rt.id._id)
        , account_name(rt.account_name)
        , research_id(rt.research_id._id)
        , amount(rt.amount)
        , is_compensation(rt.is_compensation)
    {}

    // because fc::variant require for temporary object
    research_token_api_obj()
    {
    }

    int64_t id;
    account_name_type account_name;
    int64_t research_id;
    share_type amount;
    bool is_compensation;
};

struct vote_api_obj
{
    vote_api_obj(const chain::vote_object& vo)
            : id(vo.id._id)
            , discipline_id(vo.discipline_id._id)
            , voter(vo.voter)
            , research_id(vo.research_id._id)
            , research_content_id(vo.research_content_id._id)
            , tokens_amount(vo.tokens_amount)
            , weight(vo.weight)
            , voting_power(vo.voting_power)
            , voting_time(vo.voting_time)
    {}

    // because fc::variant require for temporary object
    vote_api_obj()
    {
    }

    int64_t id;
    int64_t discipline_id;
    account_name_type voter;
    int64_t research_id;
    int64_t research_content_id;

    share_type tokens_amount;
    int64_t weight;
    uint16_t voting_power;
    time_point_sec voting_time;

};

struct review_vote_api_obj
{
    review_vote_api_obj(const chain::review_vote_object& rvo)
            : id(rvo.id._id)
            , discipline_id(rvo.discipline_id._id)
            , voter(rvo.voter)
            , review_id(rvo.review_id._id)
            , weight(rvo.weight)
            , voting_time(rvo.voting_time)
    {}

    // because fc::variant require for temporary object
    review_vote_api_obj()
    {
    }

    int64_t id;
    int64_t discipline_id;
    account_name_type voter;
    int64_t review_id;

    int64_t weight;
    time_point_sec voting_time;

};

struct expertise_allocation_proposal_api_obj
{
    expertise_allocation_proposal_api_obj(const chain::expertise_allocation_proposal_object& eapo)
            : id(eapo.id._id)
            , claimer(eapo.claimer)
            , discipline_id(eapo.discipline_id._id)
            , amount(DEIP_EXPERTISE_CLAIM_AMOUNT)
            , total_voted_expertise(eapo.total_voted_expertise)
            , quorum_percent(eapo.quorum)
            , creation_time(eapo.creation_time)
            , expiration_time(eapo.expiration_time)
            , description(fc::to_string(eapo.description))
    {}

    // because fc::variant require for temporary object
    expertise_allocation_proposal_api_obj()
    {
    }

    int64_t id;
    account_name_type claimer;
    int64_t discipline_id;

    share_type amount;

    int64_t total_voted_expertise;
    percent_type quorum_percent;

    time_point_sec creation_time;
    time_point_sec expiration_time;

    string description;

};

struct expertise_allocation_proposal_vote_api_obj
{
    expertise_allocation_proposal_vote_api_obj(const chain::expertise_allocation_proposal_vote_object& eapvo)
            : id(eapvo.id._id)
            , expertise_allocation_proposal_id(eapvo.expertise_allocation_proposal_id._id)
            , discipline_id(eapvo.discipline_id._id)
            , voter(eapvo.voter)
            , weight(eapvo.weight.value)
            , voting_time(eapvo.voting_time)

    {}

    // because fc::variant require for temporary object
    expertise_allocation_proposal_vote_api_obj()
    {
    }

    int64_t id;
    int64_t expertise_allocation_proposal_id;
    int64_t discipline_id;

    account_name_type voter;
    int64_t weight;

    time_point_sec voting_time;
};

struct vesting_balance_api_obj
{
    vesting_balance_api_obj(const chain::vesting_balance_object& vbo)
        : id(vbo.id._id)
        ,  owner(vbo.owner)
        ,  balance(vbo.balance)
        ,  withdrawn(vbo.withdrawn)
        ,  vesting_cliff_seconds(vbo.vesting_cliff_seconds) 
        ,  vesting_duration_seconds(vbo.vesting_duration_seconds)
        ,  period_duration_seconds(vbo.period_duration_seconds)
        ,  start_timestamp(vbo.start_timestamp)
    {}

    // because fc::variant require for temporary object
    vesting_balance_api_obj()
    {
    }

    int64_t id;
    account_name_type owner;
    asset balance;
    asset withdrawn;
    uint32_t vesting_cliff_seconds;
    uint32_t vesting_duration_seconds;
    uint32_t period_duration_seconds;
    time_point_sec start_timestamp;
};

struct offer_research_tokens_api_obj
{
    offer_research_tokens_api_obj(const chain::offer_research_tokens_object& orto)
        :  id(orto.id._id)
        ,  sender(orto.sender)
        ,  receiver(orto.receiver)
        ,  research_id(orto.research_id._id)
        ,  amount(orto.amount.value)
        ,  price(orto.price)
    {}

    // because fc::variant require for temporary object
    offer_research_tokens_api_obj()
    {
    }

    int64_t id;
    account_name_type sender;
    account_name_type receiver;

    int64_t research_id;
    uint32_t amount;
    asset price;
};


struct eci_and_expertise_stats_api_obj
{
    eci_and_expertise_stats_api_obj()
    {}

    uint32_t max_research_eci_in_discipline;
    uint32_t average_research_eci_in_discipline;
    uint32_t average_content_eci_in_discipline;
    uint32_t average_expertise_in_discipline;
};

struct grant_api_obj
{
    grant_api_obj(const chain::grant_object& g_o)
        : id(g_o.id._id)
        , grantor(g_o.grantor)
        , amount(g_o.amount)
        , review_committee_id(g_o.review_committee_id._id)
        , min_number_of_positive_reviews(g_o.min_number_of_positive_reviews)
        , min_number_of_applications(g_o.min_number_of_applications)
        , max_number_of_research_to_grant(g_o.max_number_of_research_to_grant)
        , created_at(g_o.created_at)
        , start_date(g_o.start_date)
        , end_date(g_o.end_date)

    {
        target_disciplines.insert(g_o.target_disciplines.begin(), g_o.target_disciplines.end());
    }
    

    // because fc::variant require for temporary object
    grant_api_obj()
    {
    }

    int64_t id;
    account_name_type grantor;
    asset amount;
    int64_t review_committee_id;

    uint16_t min_number_of_positive_reviews;
    uint16_t min_number_of_applications;
    uint16_t max_number_of_research_to_grant;

    fc::time_point_sec created_at;
    fc::time_point_sec start_date;
    fc::time_point_sec end_date;

    std::set<discipline_id_type> target_disciplines;
};

struct grant_application_api_obj
{
    grant_application_api_obj(const chain::grant_application_object& ga_o)
        :  id(ga_o.id._id)
        ,  grant_id(ga_o.grant_id._id)
        ,  research_id(ga_o.research_id._id)
        ,  application_hash(fc::to_string(ga_o.application_hash))
        ,  creator(ga_o.creator)
        ,  created_at(ga_o.created_at)
        ,  status(ga_o.status)

    {}

    // because fc::variant require for temporary object
    grant_application_api_obj()
    {
    }

    int64_t id;
    int64_t grant_id;
    int64_t research_id;
    std::string application_hash;
    account_name_type creator;
    fc::time_point_sec created_at;

    grant_application_status status;
};

struct grant_application_review_api_obj
{
    grant_application_review_api_obj(const chain::grant_application_review_object& r, const vector<discipline_api_obj>& disciplines)
            : id(r.id._id)
            , grant_application_id(r.grant_application_id._id)
            , content(fc::to_string(r.content))
            , is_positive(r.is_positive)
            , author(r.author)
            , created_at(r.created_at)
    {
        this->disciplines = disciplines;
    }

    // because fc::variant require for temporary object
    grant_application_review_api_obj()
    {
    }

    int64_t id;
    int64_t grant_application_id;
    string content;
    bool is_positive;
    account_name_type author;
    time_point_sec created_at;
    vector<discipline_api_obj> disciplines;
};

struct funding_opportunity_api_obj
{
    funding_opportunity_api_obj(const chain::funding_opportunity_object& fo_o)
        :  id(fo_o.id._id)
        ,  review_committee_id(fo_o.review_committee_id._id)
        ,  grantor(fo_o.grantor)
        ,  funding_opportunity_number(fc::to_string(fo_o.funding_opportunity_number))
        ,  amount(fo_o.amount)
        ,  award_ceiling(fo_o.award_ceiling)
        ,  award_floor(fo_o.award_floor)
        ,  awarded(fo_o.awarded)
        ,  expected_number_of_awards(fo_o.expected_number_of_awards)
        ,  posted_date(fo_o.posted_date)
        ,  open_date(fo_o.open_date)
        ,  close_date(fo_o.close_date)
    {
        for (auto& pair : fo_o.additional_info)
        {
            std::string key = fc::to_string(pair.first);
            std::string val = fc::to_string(pair.second);
            additional_info.insert(std::pair<string, string>(key, val));
        }

        for (auto& discipline_id : fo_o.target_disciplines)
        {
            target_disciplines.insert(discipline_id._id);
        }

        officers.insert(fo_o.officers.begin(), fo_o.officers.end());
    }

    // because fc::variant require for temporary object
    funding_opportunity_api_obj()
    {
    }

    int64_t id;
    research_group_id_type review_committee_id;
    account_name_type grantor;
    string funding_opportunity_number;

    asset amount = asset(0, DEIP_SYMBOL);
    asset award_ceiling = asset(0, DEIP_SYMBOL);
    asset award_floor = asset(0, DEIP_SYMBOL);
    asset awarded = asset(0, DEIP_SYMBOL);

    uint16_t expected_number_of_awards;

    fc::time_point_sec posted_date;
    fc::time_point_sec open_date;
    fc::time_point_sec close_date;

    std::map<std::string, std::string> additional_info;
    std::set<int64_t> target_disciplines;
    std::set<account_name_type> officers;
};

struct asset_api_obj
{
    asset_api_obj(const chain::asset_object& a_o)
        :  id(a_o.id._id)
        ,  symbol(a_o.symbol)
        ,  string_symbol(fc::to_string(a_o.string_symbol))
        ,  precision(a_o.precision)
        ,  issuer(a_o.issuer)
        ,  name(fc::to_string(a_o.name))
        ,  description(fc::to_string(a_o.description))
        ,  current_supply(a_o.current_supply)

    {}

    // because fc::variant require for temporary object
    asset_api_obj()
    {
    }

    int64_t id;

    asset_symbol_type symbol;
    std::string string_symbol;
    uint8_t precision;
    account_name_type issuer;
    std::string name;
    std::string description;

    share_type current_supply;
};

struct account_balance_api_obj
{
    account_balance_api_obj(const chain::account_balance_object& ab_o)
        : id(ab_o.id._id)
        , asset_id(ab_o.asset_id._id)
        , owner(ab_o.owner)
        , amount(asset(ab_o.amount, ab_o.symbol))
    {}

    // because fc::variant require for temporary object
    account_balance_api_obj()
    {
    }

    int64_t id;

    int64_t asset_id;
    account_name_type owner;
    asset amount;
};

struct research_group_organization_contract_api_obj
{
    research_group_organization_contract_api_obj(const chain::research_group_organization_contract_object& contract)
        : id(contract.id._id)
        , organization_id(contract.organization_id._id)
        , research_group_id(contract.research_group_id._id)
        , unilateral_termination_allowed(contract.unilateral_termination_allowed)
        , notes(fc::to_string(contract.notes))
        , type(contract.type)

    {
        organization_agents.insert(contract.organization_agents.begin(), contract.organization_agents.end());
    }

    // because fc::variant require for temporary object
    research_group_organization_contract_api_obj()
    {
    }

    int64_t id;

    int64_t organization_id;
    int64_t research_group_id;
    bool unilateral_termination_allowed;
    string notes;
    uint16_t type;

    std::set<account_name_type> organization_agents;
};

struct award_api_obj
{
    award_api_obj(const chain::award_object& award)
        : id(award.id._id)
        , funding_opportunity_id(award.funding_opportunity_id._id)
        , creator(award.creator)
        , status(award.status)
        , amount(award.amount)

    {
    }

    // because fc::variant require for temporary object
    award_api_obj()
    {
    }

    int64_t id;

    int64_t funding_opportunity_id;
    account_name_type creator;
    award_status status;
    asset amount;
};

struct award_research_relation_api_obj
{
    award_research_relation_api_obj(const chain::award_research_relation_object& arr_o)
        : id(arr_o.id._id)
        , award_id(arr_o.award_id._id)
        , research_id(arr_o.research_id._id)
        , research_group_id(arr_o.research_group_id._id)
        , awardee(arr_o.awardee)
        , total_amount(arr_o.total_amount)
        , total_expenses(arr_o.total_expenses)
        , university_id(arr_o.university_id._id)
        , university_overhead(arr_o.university_overhead)
    {
    }

    // because fc::variant require for temporary object
    award_research_relation_api_obj()
    {
    }

    int64_t id;

    int64_t award_id;
    int64_t research_id;
    int64_t research_group_id;
    account_name_type awardee;
    asset total_amount;
    asset total_expenses;

    int64_t university_id;
    share_type university_overhead;
};

}; // namespace app
} // namespace deip

// clang-format off

FC_REFLECT( deip::app::account_api_obj,
             (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)
             (created)(mined)
             (recovery_account)(last_account_recovery)
             (lifetime_vote_count)(can_vote)
             (common_tokens_balance)(expert_tokens_balance)(received_common_tokens)(common_tokens_withdraw_rate)(next_common_tokens_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)
             (proxied_vsf_votes)(witnesses_voted_for)
             (average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update)
             (average_market_bandwidth)(lifetime_market_bandwidth)(last_market_bandwidth_update)(balances)
          )

FC_REFLECT( deip::app::owner_authority_history_api_obj,
             (id)
             (account)
             (previous_owner_authority)
             (last_valid_time)
          )

FC_REFLECT( deip::app::account_recovery_request_api_obj,
             (id)
             (account_to_recover)
             (new_owner_authority)
             (expires)
          )

FC_REFLECT( deip::app::witness_api_obj,
             (id)
             (owner)
             (created)
             (url)(votes)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
             (last_aslot)(last_confirmed_block_num)(signing_key)
             (props)
             (sbd_exchange_rate)(last_sbd_exchange_update)
             (last_work)
             (running_version)
             (hardfork_version_vote)(hardfork_time_vote)
          )

FC_REFLECT_DERIVED( deip::app::signed_block_api_obj, (deip::protocol::signed_block),
                     (block_id)
                     (signing_key)
                     (transaction_ids)
                  )

FC_REFLECT_DERIVED( deip::app::dynamic_global_property_api_obj, (deip::chain::dynamic_global_property_object),
                     (current_reserve_ratio)
                     (average_block_size)
                     (max_virtual_bandwidth)
                  )

FC_REFLECT( deip::app::discipline_supply_api_obj,
            (id)
            (grantor)
            (target_discipline)
            (created)
            (balance)
            (per_block)
            (start_time)
            (end_time)
            (is_extendable)
            (content_hash)
            (additional_info)
          )

FC_REFLECT( deip::app::discipline_api_obj,
            (id)
            (parent_id)
            (name)
            (total_active_weight)
            (total_expertise_amount)
          )


FC_REFLECT( deip::app::research_api_obj,
            (id)
            (research_group_id)
            (title)
            (abstract)
            (permlink)
            (is_finished)
            (owned_tokens)
            (review_share_in_percent)
            (created_at)
            (dropout_compensation_in_percent)
            (disciplines)
            (group_permlink)
            (eci_per_discipline)
            (number_of_positive_reviews)
            (number_of_negative_reviews)
            (last_update_time)
            (contents_amount)
            (members)
            (is_private)
          )

FC_REFLECT( deip::app::research_content_api_obj,
            (id)
            (research_id)
            (content_type)
            (title)
            (content)
            (permlink)
            (authors)
            (activity_state)
            (activity_window_start)
            (activity_window_end)
            (created_at)
            (references)
            (external_references)
            (eci_per_discipline)
          )

FC_REFLECT( deip::app::expert_token_api_obj,
            (id)
            (account_name)
            (discipline_id)
            (discipline_name)
            (amount)
)


FC_REFLECT( deip::app::proposal_api_obj,
            (id)
            (research_group_id)
            (action)
            (creation_time)
            (expiration_time)
            (creator)
            (data)
            (quorum_percent)
            (current_votes_amount)
            (is_completed)
            (voted_accounts)
            (votes)
)

FC_REFLECT( deip::app::proposal_vote_api_obj,
            (id)
            (proposal_id)
            (research_group_id)
            (voting_time)
            (voter)
)

FC_REFLECT( deip::app::research_group_token_api_obj,
            (id)
            (research_group_id)
            (amount)
            (owner)
)

FC_REFLECT( deip::app::research_group_api_obj,
            (id)
            (creator)
            (name)
            (permlink)
            (description)
            (quorum_percent)
            (proposal_quorums)
            (is_dao)
            (is_personal)
            (is_centralized)
            (balance)
)

FC_REFLECT( deip::app::research_token_sale_api_obj,
            (id)
            (research_id)
            (start_time)
            (end_time)
            (total_amount)
            (balance_tokens)
            (soft_cap)
            (hard_cap)
            (status)
)

FC_REFLECT( deip::app::research_token_sale_contribution_api_obj,
            (id)
            (research_token_sale_id)
            (owner)
            (amount)
            (contribution_time)
)

FC_REFLECT( deip::app::research_discipline_relation_api_obj,
            (id)
            (research_id)
            (discipline_id)
            (votes_count)
            (research_eci)
)

FC_REFLECT( deip::app::research_group_invite_api_obj,
            (id)
            (account_name)
            (research_group_id)
            (research_group_token_amount)
            (cover_letter)
            (is_head)
)

FC_REFLECT( deip::app::research_listing_api_obj,
           (research_id)
           (title)
           (abstract)
           (permlink)
           (owned_tokens)
           (review_share_in_percent)
           (created_at)
           (group_members)
           (disciplines)
           (votes_count)
           (group_id)
           (group_permlink)
           (eci_per_discipline)
           (last_update_time)
           (contents_amount)
           (members)
           (number_of_positive_reviews)
           (number_of_negative_reviews)
           (is_private)
)

FC_REFLECT( deip::app::total_votes_api_obj,
           (id)
           (discipline_id)
           (research_id)
           (research_content_id)
           (total_weight)
)

FC_REFLECT( deip::app::review_api_obj,
            (id)
            (research_content_id)
            (content)
            (is_positive)
            (author)
            (created_at)
            (disciplines)
            (expertise_tokens_amount_by_discipline)
            (assessment_model_v)
            (scores)
)

FC_REFLECT( deip::app::research_token_api_obj,
            (id)
            (account_name)
            (research_id)
            (amount)
            (is_compensation)
)

FC_REFLECT( deip::app::vote_api_obj,
            (id)
            (discipline_id)
            (voter)
            (research_id)
            (research_content_id)
            (weight)
            (voting_time)
            (voting_power)
            (tokens_amount)

)

FC_REFLECT( deip::app::review_vote_api_obj,
            (id)
            (discipline_id)
            (voter)
            (review_id)
            (weight)
            (voting_time)

)

FC_REFLECT( deip::app::expertise_allocation_proposal_api_obj,
            (id)
            (claimer)
            (discipline_id)
            (amount)
            (total_voted_expertise)
            (quorum_percent)
            (creation_time)
            (expiration_time)
            (description)
)

FC_REFLECT( deip::app::expertise_allocation_proposal_vote_api_obj,
            (id)
            (expertise_allocation_proposal_id)
            (discipline_id)
            (voter)
            (weight)
            (voting_time)
)

FC_REFLECT( deip::app::vesting_balance_api_obj,
            (id)
            (owner)
            (balance)
            (withdrawn)
            (vesting_cliff_seconds)
            (vesting_duration_seconds)
            (period_duration_seconds)
            (start_timestamp)
)

FC_REFLECT( deip::app::offer_research_tokens_api_obj,
            (id)
            (sender)
            (receiver)
            (research_id)
            (amount)
            (price)

)

FC_REFLECT( deip::app::eci_and_expertise_stats_api_obj,
            (max_research_eci_in_discipline)
            (average_research_eci_in_discipline)
            (average_content_eci_in_discipline)
            (average_expertise_in_discipline)

)

FC_REFLECT( deip::app::grant_api_obj,
            (id)
            (grantor)       
            (amount)
            (review_committee_id)
            (min_number_of_positive_reviews)
            (min_number_of_applications)
            (max_number_of_research_to_grant)
            (created_at)
            (start_date)
            (end_date)
            (target_disciplines)
)

FC_REFLECT( deip::app::grant_application_api_obj,
            (id)
            (grant_id)
            (research_id)
            (application_hash)
            (creator)
            (created_at)
            (status)
)

FC_REFLECT( deip::app::grant_application_review_api_obj,
            (id)
            (grant_application_id)
            (content)
            (is_positive)
            (author)
            (created_at)
            (disciplines)
)

FC_REFLECT( deip::app::funding_opportunity_api_obj,
            (id)
            (review_committee_id)
            (grantor)
            (funding_opportunity_number)
            (amount)
            (award_ceiling)
            (award_floor)
            (awarded)
            (expected_number_of_awards)
            (posted_date)
            (open_date)
            (close_date)
            (additional_info)
            (target_disciplines)
            (officers)
)

FC_REFLECT( deip::app::asset_api_obj,
            (id)
            (symbol)
            (string_symbol)
            (precision)
            (issuer)
            (name)
            (description)
            (current_supply)
)

FC_REFLECT( deip::app::account_balance_api_obj,
            (id)
            (asset_id)
            (owner)
            (amount)
)

FC_REFLECT( deip::app::research_group_organization_contract_api_obj,
            (id)
            (organization_id)
            (research_group_id)
            (unilateral_termination_allowed)
            (notes)
            (type)
            (organization_agents)
)

FC_REFLECT( deip::app::award_api_obj,
            (id)
            (funding_opportunity_id)
            (creator)
            (status)
            (amount)
)

FC_REFLECT( deip::app::award_research_relation_api_obj,
            (id)
            (award_id)
            (research_id)
            (research_group_id)
            (awardee)
            (total_amount)
            (total_expenses)
            (university_id)
            (university_overhead)
)

// clang-format on