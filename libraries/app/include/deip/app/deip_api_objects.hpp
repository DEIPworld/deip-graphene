#pragma once
#include <deip/chain/schema/account_object.hpp>
#include <deip/chain/schema/block_summary_object.hpp>
#include <deip/chain/schema/global_property_object.hpp>
//#include <deip/chain/history_object.hpp>
#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/schema/transaction_object.hpp>
#include <deip/chain/schema/witness_objects.hpp>
#include <deip/chain/schema/discipline_supply_object.hpp>
#include <deip/chain/schema/proposal_object.hpp>
#include <deip/chain/schema/proposal_vote_object.hpp>
#include <deip/chain/schema/discipline_object.hpp>
#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/schema/research_content_object.hpp>
#include <deip/chain/schema/expert_token_object.hpp>
#include <deip/chain/schema/research_token_sale_object.hpp>
#include <deip/chain/schema/research_group_object.hpp>
#include <deip/chain/schema/research_discipline_relation_object.hpp>
#include <deip/chain/schema/research_group_invite_object.hpp>
#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/schema/total_votes_object.hpp>
#include <deip/chain/schema/review_object.hpp>
#include <deip/chain/schema/research_token_object.hpp>
#include <deip/chain/schema/vesting_balance_object.hpp>
#include <deip/chain/schema/offer_research_tokens_object.hpp>
#include <deip/chain/schema/grant_object.hpp>
#include <deip/chain/schema/grant_application_object.hpp>

#include <deip/witness/witness_objects.hpp>

#include <deip/chain/database/database.hpp>
#include <deip/chain/schema/vote_object.hpp>
#include <deip/chain/schema/review_vote_object.hpp>
#include <deip/chain/schema/expertise_allocation_proposal_object.hpp>
#include <deip/chain/schema/expertise_allocation_proposal_vote_object.hpp>

namespace deip {
namespace app {

using namespace deip::chain;
using research_group_token_refs_type = std::vector<std::reference_wrapper<const research_group_token_object>>;

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
        , balance(a.balance)
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

    asset balance;
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
    discipline_supply_api_obj(const chain::discipline_supply_object& b)
        : id(b.id._id)
        , owner(b.owner)
        , target_discipline(b.target_discipline._id)
        , created(b.created)
        , balance(b.balance)
        , per_block(b.per_block)
        , start_block(b.start_block)
        , end_block(b.end_block)
        , is_extendable(b.is_extendable)
        , content_hash(fc::to_string(b.content_hash))
    {
    }

    // because fc::variant require for temporary object
    discipline_supply_api_obj()
    {
    }

    int64_t id;

    account_name_type owner;
    int64_t target_discipline;

    time_point_sec created;

    asset balance;
    share_type per_block;
    uint32_t start_block;
    uint32_t end_block;

    bool is_extendable;
    string content_hash;
};

struct discipline_api_obj
{
    discipline_api_obj(const chain::discipline_object& d)
        : id(d.id._id)
        ,  parent_id(d.parent_id._id)
        ,  name(d.name)
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
            references.insert(reference._id);

        external_references.insert(
            rc.external_references.begin(), 
            rc.external_references.end()
        );

        for (const auto& kvp : rc.eci_per_discipline) {
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
        ,  research_group_id(p.research_group_id._id)
        ,  action(p.action)
        ,  creation_time(p.creation_time)
        ,  expiration_time(p.expiration_time)
        ,  creator(p.creator)
        ,  data(fc::to_string(p.data))
        ,  quorum_percent(p.quorum_percent.value)
        ,  current_votes_amount(p.current_votes_amount)
        ,  is_completed(p.is_completed)
        ,  voted_accounts(p.voted_accounts.begin(), p.voted_accounts.end())
        ,  votes(_votes)
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
    uint16_t quorum_percent;
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
    fc::fixed_string_16 owner;
};

struct research_group_api_obj
{
    research_group_api_obj(const chain::research_group_object& rg)
        : id(rg.id._id)
        ,  name(fc::to_string(rg.name))
        ,  permlink(fc::to_string(rg.permlink))
        ,  description(fc::to_string(rg.description))
        ,  quorum_percent(rg.quorum_percent.value)
        ,  is_personal(rg.is_personal)
        ,  balance(rg.balance)
    {
        for (auto& proposal_quorum : rg.proposal_quorums)
            proposal_quorums.insert(std::make_pair(static_cast<uint16_t>(proposal_quorum.first), proposal_quorum.second.value));
    }

    // because fc::variant require for temporary object
    research_group_api_obj()
    {
    }

    int64_t id;
    std::string name;
    std::string permlink;
    std::string description;
    uint32_t quorum_percent;
    std::map<uint16_t, uint32_t> proposal_quorums;
    bool is_personal;
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
    research_group_invite_api_obj(const chain::research_group_invite_object& co)
        : id(co.id._id)
        ,  account_name(co.account_name)
        ,  research_group_id(co.research_group_id._id)
        ,  research_group_token_amount(co.research_group_token_amount)
    {}

    // because fc::variant require for temporary object
    research_group_invite_api_obj()
    {
    }

    int64_t id;
    account_name_type account_name;
    int64_t research_group_id;
    share_type research_group_token_amount;
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
    {
        this->disciplines = disciplines;

        for (const auto& d : disciplines) {
            auto& discipline_id = d.id;

            this->weight_per_discipline.emplace(std::make_pair(discipline_id, r.weights_per_discipline.at(discipline_id).value));
            this->evaluation_per_discipline.emplace(std::make_pair(discipline_id, r.get_evaluation(discipline_id).value));
            this->expertise_amounts_used.emplace(std::make_pair(discipline_id, r.expertise_amounts_used.at(discipline_id).value));
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

    map<int64_t, int64_t> weight_per_discipline;
    map<int64_t, int64_t> evaluation_per_discipline;
    map<int64_t, int64_t> expertise_amounts_used;
};

struct research_token_api_obj
{
   research_token_api_obj(const chain::research_token_object& rt)
            : id(rt.id._id)
            , account_name(rt.account_name)
            , research_id(rt.research_id._id)            
            , amount(rt.amount)
    {}

    // because fc::variant require for temporary object
    research_token_api_obj()
    {
    }

    int64_t id;
    account_name_type account_name;
    int64_t research_id;
    share_type amount;
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
            , quorum_percent(eapo.quorum_percent.value)
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
    uint16_t quorum_percent;

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
        :  id(g_o.id._id)
        ,  target_discipline(g_o.target_discipline._id)
        ,  amount(g_o.amount)
        ,  owner(g_o.owner)
        ,  min_number_of_positive_reviews(g_o.min_number_of_positive_reviews)
        ,  max_researches_to_grant(g_o.max_researches_to_grant)
        ,  created_at(g_o.created_at)
        ,  start_time(g_o.start_time)
        ,  end_time(g_o.end_time)

    {}

    // because fc::variant require for temporary object
    grant_api_obj()
    {
    }

    int64_t id;
    int64_t target_discipline;
    asset amount;

    account_name_type owner;

    int16_t min_number_of_positive_reviews;
    int16_t min_number_of_applications;
    int16_t max_researches_to_grant;

    fc::time_point_sec created_at;
    fc::time_point_sec start_time;
    fc::time_point_sec end_time;
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
};

}; // namespace app
} // namespace deip

// clang-format off

FC_REFLECT( deip::app::account_api_obj,
             (id)(name)(owner)(active)(posting)(memo_key)(json_metadata)(proxy)(last_owner_update)(last_account_update)
             (created)(mined)
             (recovery_account)(last_account_recovery)
             (lifetime_vote_count)(can_vote)
             (balance)
             (common_tokens_balance)(expert_tokens_balance)(received_common_tokens)(common_tokens_withdraw_rate)(next_common_tokens_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)
             (proxied_vsf_votes)(witnesses_voted_for)
             (average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update)
             (average_market_bandwidth)(lifetime_market_bandwidth)(last_market_bandwidth_update)
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
            (owner)
            (target_discipline)
            (created)
            (balance)
            (per_block)
            (start_block)
            (end_block)
            (is_extendable)
            (content_hash)
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
            (name)
            (permlink)
            (description)
            (quorum_percent)
            (proposal_quorums)
            (is_personal)
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
            (weight_per_discipline)
            (evaluation_per_discipline)
            (expertise_amounts_used)
)

FC_REFLECT( deip::app::research_token_api_obj,
            (id)
            (account_name)
            (research_id)
            (amount)
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
            (target_discipline)
            (amount)
            (owner)
            (min_number_of_positive_reviews)
            (min_number_of_applications)
            (max_researches_to_grant)
            (created_at)
            (start_time)
            (end_time)

)

FC_REFLECT( deip::app::grant_application_api_obj,
            (id)
            (grant_id)
            (research_id)
            (application_hash)
            (creator)
            (created_at)

)

// clang-format on