#pragma once
#include <deip/chain/schema/account_object.hpp>
#include <deip/chain/schema/block_summary_object.hpp>
#include <deip/chain/schema/global_property_object.hpp>
//#include <deip/chain/history_object.hpp>
#include <deip/chain/schema/award_object.hpp>
#include <deip/chain/schema/award_recipient_object.hpp>
#include <deip/chain/schema/award_withdrawal_request_object.hpp>
#include <deip/chain/schema/account_balance_object.hpp>
#include <deip/chain/schema/asset_object.hpp>
#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/schema/discipline_object.hpp>
#include <deip/chain/schema/discipline_supply_object.hpp>
#include <deip/chain/schema/expert_token_object.hpp>
#include <deip/chain/schema/expertise_allocation_proposal_object.hpp>
#include <deip/chain/schema/expertise_allocation_proposal_vote_object.hpp>
#include <deip/chain/schema/nda_contract_object.hpp>
#include <deip/chain/schema/nda_contract_file_access_object.hpp>
#include <deip/chain/schema/proposal_object.hpp>
#include <deip/chain/schema/research_content_object.hpp>
#include <deip/chain/schema/research_discipline_relation_object.hpp>
#include <deip/chain/schema/research_group_object.hpp>
#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/schema/research_token_object.hpp>
#include <deip/chain/schema/research_token_sale_object.hpp>
#include <deip/chain/schema/review_object.hpp>
#include <deip/chain/schema/review_vote_object.hpp>
#include <deip/chain/schema/expertise_contribution_object.hpp>
#include <deip/chain/schema/transaction_object.hpp>
#include <deip/chain/schema/vesting_balance_object.hpp>
#include <deip/chain/schema/grant_application_object.hpp>
#include <deip/chain/schema/grant_application_review_object.hpp>
#include <deip/chain/schema/funding_opportunity_object.hpp>
#include <deip/chain/schema/research_license_object.hpp>
#include <deip/chain/schema/witness_objects.hpp>
#include <deip/witness/witness_objects.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace app {

using namespace deip::chain;
using research_group_token_refs_type = std::vector<std::reference_wrapper<const research_group_token_object>>;
using account_balance_refs_type = std::vector<std::reference_wrapper<const account_balance_object>>;

using deip::protocol::percent_type;
using deip::protocol::asset_symbol_type;
using deip::protocol::external_id_type;

typedef chain::change_recovery_account_request_object change_recovery_account_request_api_obj;
typedef chain::block_summary_object block_summary_api_obj;
typedef chain::withdraw_common_tokens_route_object withdraw_common_tokens_route_api_obj;
typedef chain::witness_vote_object witness_vote_api_obj;
typedef chain::witness_schedule_object witness_schedule_api_obj;
typedef chain::reward_fund_object reward_fund_api_obj;
typedef witness::account_bandwidth_object account_bandwidth_api_obj;


struct account_api_obj
{
    account_api_obj(const chain::account_object& a,
                    const chain::account_authority_object& auth,
                    const account_balance_refs_type account_balances)
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
        , is_research_group(a.is_research_group)
    {
        size_t n = a.proxied_vsf_votes.size();
        proxied_vsf_votes.reserve(n);
        for (size_t i = 0; i < n; i++)
        {
            proxied_vsf_votes.push_back(a.proxied_vsf_votes[i]);
        }

        owner = authority(auth.owner);
        active = authority(auth.active);
        last_owner_update = auth.last_owner_update;

        for (auto& pair : auth.active_overrides)
        {
            active_overrides.insert(std::pair<uint16_t, authority>(pair.first, authority(pair.second)));
        }

        for (auto& wrap : account_balances)
        {
            const auto& account_balance = wrap.get();
            balances.push_back(asset(account_balance.amount, account_balance.symbol));
        }
    }

    account_api_obj()
    {
    }

    account_id_type id;

    account_name_type name;
    authority owner;
    authority active;
    public_key_type memo_key;

    std::map<uint16_t, authority> active_overrides;

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
    bool is_research_group;

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
        block_num = block.block_num();
        signing_key = signee();
        transaction_ids.reserve(transactions.size());
        for (const signed_transaction& tx : transactions)
            transaction_ids.push_back(tx.id());
    }
    signed_block_api_obj()
    {
    }

    block_id_type block_id;
    uint32_t block_num;
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
        , external_id(d.external_id)
        , parent_id(d.parent_id._id)
        , parent_external_id(d.parent_external_id)
        , name(fc::to_string(d.name))
    {}

    // because fc::variant require for temporary object
    discipline_api_obj()
    {
    }

    int64_t id;
    string external_id;
    int64_t parent_id;
    string parent_external_id;
    std::string name;
};

struct research_group_api_obj
{
    research_group_api_obj(const chain::research_group_object& rg_o, const account_api_obj& acc_o)
        : id(rg_o.id._id)
        , external_id(rg_o.account)
        , creator(rg_o.creator)
        , permlink(fc::to_string(rg_o.permlink))
        , description(fc::to_string(rg_o.description))
        , quorum_percent(rg_o.default_quorum)
        , is_dao(rg_o.is_dao)
        , is_personal(rg_o.is_personal)
        , is_centralized(rg_o.is_centralized)
        , account(acc_o)
    {
    }

    research_group_api_obj(const chain::research_group_object& rg_o)
        : id(rg_o.id._id)
        , external_id(rg_o.account)
        , creator(rg_o.creator)
        , permlink(fc::to_string(rg_o.permlink))
        , description(fc::to_string(rg_o.description))
        , quorum_percent(rg_o.default_quorum)
        , is_dao(rg_o.is_dao)
        , is_personal(rg_o.is_personal)
        , is_centralized(rg_o.is_centralized)
    {
    }

    // because fc::variant require for temporary object
    research_group_api_obj()
    {
    }

    int64_t id;
    external_id_type external_id;
    account_name_type creator;
    std::string permlink;
    std::string description;
    percent_type quorum_percent;
    bool is_dao;
    bool is_personal;
    bool is_centralized;
    fc::optional<account_api_obj> account;
};

struct research_api_obj
{
    research_api_obj(const chain::research_object& r_o,
                     const vector<discipline_api_obj>& disciplines,
                     const research_group_api_obj& rg_api)

        : id(r_o.id._id)
        , external_id(r_o.external_id)
        , research_group_id(r_o.research_group_id._id)
        , description(fc::to_string(r_o.description))
        , permlink(fc::to_string(r_o.permlink))
        , is_finished(r_o.is_finished)
        , is_private(r_o.is_private)
        , created_at(r_o.created_at)
        , disciplines(disciplines.begin(), disciplines.end())
        , number_of_positive_reviews(r_o.number_of_positive_reviews)
        , number_of_negative_reviews(r_o.number_of_negative_reviews)
        , number_of_research_contents(r_o.number_of_research_contents)
        , last_update_time(r_o.last_update_time)
        , members(r_o.members.begin(), r_o.members.end())
        , research_group(rg_api)
    {
        for (const auto& kvp : r_o.eci_per_discipline) 
        {
            eci_per_discipline.emplace(std::make_pair(kvp.first._id, kvp.second.value));
        }

        for (const auto& st : r_o.security_tokens)
        {
            security_tokens.insert(st);
        }

        if (r_o.review_share.valid())
        {
            review_share = (*r_o.review_share).amount;
        }
        
        if (r_o.compensation_share.valid())
        {
            compensation_share = (*r_o.compensation_share).amount;
        }
    }

    // because fc::variant require for temporary object
    research_api_obj()
    {
    }

    int64_t id;
    external_id_type external_id;
    int64_t research_group_id;
    std::string description;
    std::string permlink;
    bool is_finished;
    bool is_private;
    time_point_sec created_at;
    optional<share_type> review_share;
    optional<share_type> compensation_share;
    vector<discipline_api_obj> disciplines;

    map<int64_t, int64_t> eci_per_discipline;
    set<asset> security_tokens;

    uint16_t number_of_positive_reviews;
    uint16_t number_of_negative_reviews;
    uint16_t number_of_research_contents;
    time_point_sec last_update_time;

    std::vector<account_name_type> members;

    research_group_api_obj research_group;
};

struct research_content_api_obj
{
    research_content_api_obj(const chain::research_content_object& rc_o)
        : id(rc_o.id._id)
        , external_id(rc_o.external_id)
        , research_id(rc_o.research_id._id)
        , content_type(rc_o.type)
        , authors(rc_o.authors.begin(), rc_o.authors.end())
        , title(fc::to_string(rc_o.title))
        , content(fc::to_string(rc_o.content))
        , permlink(fc::to_string(rc_o.permlink))
        , activity_state(rc_o.activity_state)
        , activity_window_start(rc_o.activity_window_start)
        , activity_window_end(rc_o.activity_window_end)
        , created_at(rc_o.created_at)
    {

        references.insert(rc_o.references.begin(), rc_o.references.end());

        for (const auto& kvp : rc_o.eci_per_discipline)
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
    external_id_type external_id;
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
    std::set<external_id_type> references;

    map<int64_t, int64_t> eci_per_discipline;
};


struct research_license_api_obj
{
    research_license_api_obj(const chain::research_license_object& rl_o)

        : id(rl_o.id._id)
        , external_id(rl_o.external_id)
        , licenser(rl_o.licenser)
        , licensee(rl_o.licensee)
        , terms(fc::to_string(rl_o.terms))
        , status(rl_o.status)
        , expiration_time(rl_o.expiration_time)
        , created_at(rl_o.created_at)

    {
        if (rl_o.fee.valid())
        {
            fee = *rl_o.fee;
        }
    }

    // because fc::variant require for temporary object
    research_license_api_obj()
    {
    }

    int64_t id;
    string external_id;

    external_id_type research_external_id;

    account_name_type licenser;
    account_name_type licensee;

    string terms;
    uint16_t status;

    time_point_sec expiration_time;
    time_point_sec created_at;

    optional<asset> fee;
};


struct expert_token_api_obj
{
    expert_token_api_obj(const chain::expert_token_object& d, const string& discipline_name)
        : id(d.id._id)
        , account_name(d.account_name)
        , discipline_id(d.discipline_id._id)
        , discipline_external_id(d.discipline_external_id)
        , discipline_name(discipline_name)
        , amount(d.amount)
    {}

    // because fc::variant require for temporary object
    expert_token_api_obj()
    {
    }

    int64_t id;
    string account_name;
    int64_t discipline_id;
    string discipline_external_id;
    string discipline_name;
    share_type amount;
};

struct proposal_api_obj
{
    proposal_api_obj(const chain::proposal_object& p_o)
        : id(p_o.id._id)
        , external_id(p_o.external_id)
        , creator(p_o.proposer)
        , proposed_transaction(p_o.proposed_transaction)
        , expiration_time(p_o.expiration_time)
        , review_period_time(p_o.review_period_time)
        , fail_reason(fc::to_string(p_o.fail_reason))
        , created_at(p_o.created_at)
    {
        required_active_approvals.insert(p_o.required_active_approvals.begin(), p_o.required_active_approvals.end());
        available_active_approvals.insert(p_o.available_active_approvals.begin(), p_o.available_active_approvals.end());
        required_owner_approvals.insert(p_o.required_owner_approvals.begin(), p_o.required_owner_approvals.end());
        available_owner_approvals.insert(p_o.available_owner_approvals.begin(), p_o.available_owner_approvals.end());
        available_key_approvals.insert(p_o.available_key_approvals.begin(), p_o.available_key_approvals.end());

        voted_accounts.insert(p_o.available_active_approvals.begin(), p_o.available_active_approvals.end());
        voted_accounts.insert(p_o.available_owner_approvals.begin(), p_o.available_owner_approvals.end());
    }

    // because fc::variant require for temporary object
    proposal_api_obj()
    {
    }

    int64_t id;

    string external_id;
    account_name_type creator;
    transaction proposed_transaction;

    time_point_sec expiration_time;
    optional<time_point_sec> review_period_time;
    string fail_reason;
    time_point_sec created_at;

    set<account_name_type> required_active_approvals;
    set<account_name_type> available_active_approvals;
    set<account_name_type> required_owner_approvals;
    set<account_name_type> available_owner_approvals;
    set<public_key_type> available_key_approvals;

    set<account_name_type> voted_accounts;
};

struct research_group_token_api_obj
{
    research_group_token_api_obj(const chain::research_group_token_object& rgt, const research_group_api_obj& rg)
        : id(rgt.id._id)
        , research_group_id(rgt.research_group_id._id)
        , amount(rgt.amount.value)
        , owner(rgt.owner)
        , research_group(rg)
    {
    }

    // because fc::variant require for temporary object
    research_group_token_api_obj()
    {
    }

    int64_t id;
    int64_t research_group_id; // deprecated
    uint32_t amount;
    account_name_type owner;
    research_group_api_obj research_group;
};

struct research_token_sale_api_obj
{
    research_token_sale_api_obj(const chain::research_token_sale_object& rts_o)
        : id(rts_o.id._id)
        , external_id(rts_o.external_id)
        , research_id(rts_o.research_id._id)
        , research_external_id(rts_o.research_external_id)
        , start_time(rts_o.start_time)
        , end_time(rts_o.end_time)
        , total_amount(rts_o.total_amount)
        , soft_cap(rts_o.soft_cap)
        , hard_cap(rts_o.hard_cap)
        , status(rts_o.status)
    {
        for (const auto& security_token_on_sale : rts_o.security_tokens_on_sale)
        {
            security_tokens_on_sale.insert(security_token_on_sale);
        }
    }

    // because fc::variant require for temporary object
    research_token_sale_api_obj()
    {
    }

    int64_t id;
    string external_id;
    int64_t research_id;
    string research_external_id;
    std::set<asset> security_tokens_on_sale;
    time_point_sec start_time;
    time_point_sec end_time;
    asset total_amount;
    asset soft_cap;
    asset hard_cap;
    uint16_t status;
};

struct research_token_sale_contribution_api_obj
{
    research_token_sale_contribution_api_obj(const chain::research_token_sale_contribution_object& co)
        : id(co.id._id)
        , research_token_sale(co.research_token_sale)
        , research_token_sale_id(co.research_token_sale_id._id)
        , owner(co.owner)
        , amount(co.amount)
        , contribution_time(co.contribution_time)
    {}

    // because fc::variant require for temporary object
    research_token_sale_contribution_api_obj()
    {
    }

    int64_t id;
    string research_token_sale;
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

struct expertise_contribution_object_api_obj
{
    expertise_contribution_object_api_obj(const chain::expertise_contribution_object& ec)
        :  id(ec.id._id)
        ,  discipline_id(ec.discipline_id._id)
        ,  research_id(ec.research_id._id)
        ,  research_content_id(ec.research_content_id._id)
        ,  eci(ec.eci)
    {}

    // because fc::variant require for temporary object
    expertise_contribution_object_api_obj()
    {
    }

    int64_t id;
    int64_t discipline_id;
    int64_t research_id;
    int64_t research_content_id;

    share_type eci;
};

struct review_api_obj
{
    review_api_obj(const chain::review_object& r, const vector<discipline_api_obj>& disciplines)
        : id(r.id._id)
        , external_id(r.external_id)
        , research_content_id(r.research_content_id._id)
        , research_content_external_id(r.research_content_external_id)
        , content(fc::to_string(r.content))
        , is_positive(r.is_positive)
        , author(r.author)
        , created_at(r.created_at)
        , assessment_model_v(r.assessment_model_v)
        , scores(r.assessment_criterias.begin(), r.assessment_criterias.end())
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
    string external_id;
    int64_t research_content_id;
    string research_content_external_id;
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
    research_token_api_obj(const chain::research_token_object& rt_o)
        : id(rt_o.id._id)
        , account_name(rt_o.account_name)
        , research_id(rt_o.research_id._id)
        , research_external_id(rt_o.research_external_id)
        , amount(rt_o.amount)
        , is_compensation(rt_o.is_compensation)
    {}

    // because fc::variant require for temporary object
    research_token_api_obj()
    {
    }

    int64_t id;
    account_name_type account_name;
    int64_t research_id;
    string research_external_id;
    share_type amount;
    bool is_compensation;
};

struct review_vote_api_obj
{
    review_vote_api_obj(const chain::review_vote_object& rvo)
        : id(rvo.id._id)
        , external_id(rvo.external_id)
        , discipline_id(rvo.discipline_id._id)
        , discipline_external_id(rvo.discipline_external_id)
        , voter(rvo.voter)
        , review_id(rvo.review_id._id)
        , review_external_id(rvo.review_external_id)
        , weight(rvo.weight)
        , voting_time(rvo.voting_time)
    {}

    // because fc::variant require for temporary object
    review_vote_api_obj()
    {
    }

    int64_t id;
    string external_id;
    int64_t discipline_id;
    string discipline_external_id;
    account_name_type voter;
    int64_t review_id;
    string review_external_id;
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

struct grant_application_api_obj
{
    grant_application_api_obj(const chain::grant_application_object& ga_o)
        :  id(ga_o.id._id)
        ,  funding_opportunity_number(ga_o.funding_opportunity_number)
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
    external_id_type funding_opportunity_number;
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
        ,  organization_id(fo_o.organization_id._id)
        ,  review_committee_id(fo_o.review_committee_id._id)
        ,  treasury_id(fo_o.treasury_id._id)
        ,  grantor(fo_o.grantor)
        ,  funding_opportunity_number(fo_o.funding_opportunity_number)
        ,  amount(fo_o.amount)
        ,  award_ceiling(fo_o.award_ceiling)
        ,  award_floor(fo_o.award_floor)
        ,  current_supply(fo_o.current_supply)
        ,  expected_number_of_awards(fo_o.expected_number_of_awards)
        ,  min_number_of_positive_reviews(fo_o.min_number_of_positive_reviews)
        ,  min_number_of_applications(fo_o.min_number_of_applications)
        ,  max_number_of_research_to_grant(fo_o.max_number_of_research_to_grant)
        ,  posted_date(fo_o.posted_date)
        ,  open_date(fo_o.open_date)
        ,  close_date(fo_o.close_date)
        ,  distribution_type(fo_o.distribution_type)
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
    research_group_id_type organization_id;
    research_group_id_type review_committee_id;
    research_group_id_type treasury_id;

    account_name_type grantor;
    external_id_type funding_opportunity_number;

    asset amount = asset(0, DEIP_SYMBOL);
    asset award_ceiling = asset(0, DEIP_SYMBOL);
    asset award_floor = asset(0, DEIP_SYMBOL);
    asset current_supply = asset(0, DEIP_SYMBOL);

    uint16_t expected_number_of_awards;

    uint16_t min_number_of_positive_reviews;
    uint16_t min_number_of_applications;
    uint16_t max_number_of_research_to_grant;

    fc::time_point_sec posted_date;
    fc::time_point_sec open_date;
    fc::time_point_sec close_date;

    std::map<std::string, std::string> additional_info;
    std::set<int64_t> target_disciplines;
    std::set<account_name_type> officers;

    uint16_t distribution_type;
};

struct asset_api_obj
{
    asset_api_obj(const chain::asset_object& a_o)
        : id(a_o.id._id)
        , symbol(a_o.symbol)
        , string_symbol(fc::to_string(a_o.string_symbol))
        , precision(a_o.precision)
        , issuer(a_o.issuer)
        , description(fc::to_string(a_o.description))
        , current_supply(a_o.current_supply)
        , max_supply(a_o.max_supply)
        , type(a_o.type)
    {
        if (a_o.tokenized_research.valid())
        {
            tokenized_research = *a_o.tokenized_research;
        }
    }

    // because fc::variant require for temporary object
    asset_api_obj()
    {
    }

    int64_t id;

    asset_symbol_type symbol;
    std::string string_symbol;
    uint8_t precision;
    account_name_type issuer;
    std::string description;
    share_type current_supply;
    share_type max_supply;
    uint8_t type;
    optional<external_id_type> tokenized_research;
};

struct account_balance_api_obj
{
    account_balance_api_obj(const chain::account_balance_object& ab_o)
        : id(ab_o.id._id)
        , asset_id(ab_o.asset_id._id)
        , asset_symbol(fc::to_string(ab_o.string_symbol))
        , owner(ab_o.owner)
        , amount(ab_o.to_asset())
    {
        if (ab_o.tokenized_research.valid())
        {
            tokenized_research = *ab_o.tokenized_research;
        }
    }

    // because fc::variant require for temporary object
    account_balance_api_obj()
    {
    }

    int64_t id;
    int64_t asset_id;
    string asset_symbol;
    account_name_type owner;
    asset amount;
    optional<external_id_type> tokenized_research;
};


struct award_recipient_api_obj
{
    award_recipient_api_obj(const chain::award_recipient_object& ar_o)
        : id(ar_o.id._id)
        , award_number(ar_o.award_number)
        , subaward_number(ar_o.subaward_number)
        , funding_opportunity_number(ar_o.funding_opportunity_number)
        , research_id(ar_o.research_id._id)
        , awardee(ar_o.awardee)
        , source(ar_o.source)
        , total_amount(ar_o.total_amount)
        , total_expenses(ar_o.total_expenses)
        , status(ar_o.status)
    {
    }

    award_recipient_api_obj()
    {
    }

    int64_t id;

    external_id_type award_number;
    external_id_type subaward_number;
    external_id_type funding_opportunity_number;

    int64_t research_id;
    account_name_type awardee;
    account_name_type source;
    asset total_amount;
    asset total_expenses;
    uint16_t status;
};

struct award_api_obj
{
    award_api_obj(const chain::award_object& award, const vector<award_recipient_api_obj>& awardees_list)
        : id(award.id._id)
        , funding_opportunity_number(award.funding_opportunity_number)
        , award_number(award.award_number)
        , awardee(award.awardee)
        , creator(award.creator)
        , status(award.status)
        , amount(award.amount)
        , university_id(award.university_id._id)
        , university_overhead(award.university_overhead)
    {
        awardees.insert(awardees.end(), awardees_list.begin(), awardees_list.end());
        university_fee = asset(((award.amount.amount * share_type(award.university_overhead.amount))) / DEIP_100_PERCENT, award.amount.symbol);
    }

    // because fc::variant require for temporary object
    award_api_obj()
    {
    }

    int64_t id;

    external_id_type funding_opportunity_number;
    external_id_type award_number;
    string awardee;
    string creator;
    uint16_t status;
    asset amount;
    int64_t university_id;
    percent university_overhead;
    asset university_fee;
    vector<award_recipient_api_obj> awardees;
};

struct award_withdrawal_request_api_obj
{
    award_withdrawal_request_api_obj(const chain::award_withdrawal_request_object& awr_o)
        : id(awr_o.id._id)
        , award_number(awr_o.award_number)
        , subaward_number(awr_o.subaward_number)
        , payment_number(awr_o.payment_number)
        , requester(awr_o.requester)
        , amount(awr_o.amount)
        , description(fc::to_string(awr_o.description))
        , status(awr_o.status)
        , time(awr_o.time)
        , attachment(fc::to_string(awr_o.attachment))
    {
    }

    // because fc::variant require for temporary object
    award_withdrawal_request_api_obj()
    {
    }

    int64_t id;

    external_id_type award_number;
    external_id_type subaward_number;
    external_id_type payment_number;

    account_name_type requester;

    asset amount = asset(0, DEIP_SYMBOL);
    std::string description;
    uint16_t status;
    fc::time_point_sec time;
    std::string attachment;
};

struct nda_contract_api_obj
{
    nda_contract_api_obj(const chain::nda_contract_object& c_o)
        : id(c_o.id._id)
        , contract_creator(c_o.contract_creator)
        , party_a(c_o.party_a)
        , party_a_research_group_id(c_o.party_a_research_group_id._id)
        , party_b(c_o.party_b)
        , party_b_research_group_id(c_o.party_b_research_group_id._id)
        , disclosing_party(c_o.disclosing_party.begin(), c_o.disclosing_party.end())
        , party_a_signature(fc::to_string(c_o.party_a_signature))
        , party_b_signature(fc::to_string(c_o.party_b_signature))
        , title(fc::to_string(c_o.title))
        , contract_hash(fc::to_string(c_o.contract_hash))
        , status(c_o.status)
        , created_at(c_o.created_at)
        , start_date(c_o.start_date)
        , end_date(c_o.end_date)

    {}

    // because fc::variant require for temporary object
    nda_contract_api_obj()
    {
    }

    int64_t id;
    account_name_type contract_creator;

    account_name_type party_a;
    int64_t party_a_research_group_id;

    account_name_type party_b;
    int64_t party_b_research_group_id;

    std::set<account_name_type> disclosing_party;

    std::string party_a_signature;
    std::string party_b_signature;

    std::string title;
    std::string contract_hash;
    uint16_t status;

    fc::time_point_sec created_at;
    fc::time_point_sec start_date;
    fc::time_point_sec end_date;
};

struct nda_contract_file_access_api_obj
{
    nda_contract_file_access_api_obj(const chain::nda_contract_file_access_object& cfa_o)
        : id(cfa_o.id._id)
        , contract_id(cfa_o.contract_id._id)
        , status(cfa_o.status)
        , requester(cfa_o.requester)
        , encrypted_payload_hash(fc::to_string(cfa_o.encrypted_payload_hash))
        , encrypted_payload_iv(fc::to_string(cfa_o.encrypted_payload_iv))
        , encrypted_payload_encryption_key(fc::to_string(cfa_o.encrypted_payload_encryption_key))
        , proof_of_encrypted_payload_encryption_key(fc::to_string(cfa_o.proof_of_encrypted_payload_encryption_key))

    {}

    // because fc::variant require for temporary object
    nda_contract_file_access_api_obj()
    {
    }

    int64_t id;
    int64_t contract_id;
    uint16_t status;

    account_name_type requester;
    std::string encrypted_payload_hash;
    std::string encrypted_payload_iv;
    std::string encrypted_payload_encryption_key;
    std::string proof_of_encrypted_payload_encryption_key;
};

}; // namespace app
} // namespace deip

// clang-format off

FC_REFLECT( deip::app::account_api_obj,
  (id)
  (name)
  (owner)
  (active)
  (memo_key)
  (active_overrides)
  (json_metadata)
  (proxy)
  (last_owner_update)
  (last_account_update)
  (created)
  (mined)
  (recovery_account)
  (last_account_recovery)
  (lifetime_vote_count)
  (can_vote)
  (common_tokens_balance)
  (expert_tokens_balance)
  (received_common_tokens)
  (common_tokens_withdraw_rate)
  (next_common_tokens_withdrawal)
  (withdrawn)
  (to_withdraw)
  (withdraw_routes)
  (proxied_vsf_votes)
  (witnesses_voted_for)
  (is_research_group)
  (average_bandwidth)
  (lifetime_bandwidth)
  (last_bandwidth_update)
  (average_market_bandwidth)
  (lifetime_market_bandwidth)
  (last_market_bandwidth_update)
  (balances)
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
                     (block_num)
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
            (external_id)
            (parent_id)
            (parent_external_id)
            (name))


FC_REFLECT( deip::app::research_api_obj,
            (id)
            (external_id)
            (research_group_id)
            (description)
            (permlink)
            (is_finished)
            (is_private)
            (created_at)
            (review_share)
            (compensation_share)
            (disciplines)
            (eci_per_discipline)
            (security_tokens)
            (number_of_positive_reviews)
            (number_of_negative_reviews)
            (number_of_research_contents)
            (last_update_time)
            (members)
            (research_group)
)

FC_REFLECT( deip::app::research_content_api_obj,
            (id)
            (external_id)
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
            (eci_per_discipline)
          )


FC_REFLECT( deip::app::research_license_api_obj,
            (id)
            (external_id)
            (research_external_id)
            (licenser)
            (licensee)
            (terms)
            (status)
            (expiration_time)
            (created_at)
            (fee)
)


FC_REFLECT( deip::app::expert_token_api_obj,
            (id)
            (account_name)
            (discipline_id)
            (discipline_external_id)
            (discipline_name)
            (amount)
)


FC_REFLECT( deip::app::proposal_api_obj,
            (id)
            (external_id)
            (creator)
            (proposed_transaction)
            (expiration_time)
            (review_period_time)
            (fail_reason)
            (created_at)
            (required_active_approvals)
            (available_active_approvals)
            (required_owner_approvals)
            (available_owner_approvals)
            (available_key_approvals)
            (voted_accounts)
)

FC_REFLECT( deip::app::research_group_token_api_obj,
            (id)
            (research_group_id)
            (amount)
            (owner)
            (research_group)
)

FC_REFLECT( deip::app::research_group_api_obj,
            (id)
            (external_id)
            (creator)
            (permlink)
            (description)
            (quorum_percent)
            (is_dao)
            (is_personal)
            (is_centralized)
            (account)
)

FC_REFLECT( deip::app::research_token_sale_api_obj,
            (id)
            (external_id)
            (research_id)
            (research_external_id)
            (security_tokens_on_sale)
            (start_time)
            (end_time)
            (total_amount)
            (soft_cap)
            (hard_cap)
            (status)
)

FC_REFLECT( deip::app::research_token_sale_contribution_api_obj,
            (id)
            (research_token_sale)
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


FC_REFLECT( deip::app::expertise_contribution_object_api_obj,
           (id)
           (discipline_id)
           (research_id)
           (research_content_id)
           (eci)
)

FC_REFLECT( deip::app::review_api_obj,
            (id)
            (external_id)
            (research_content_id)
            (research_content_external_id)
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
            (research_external_id)
            (amount)
            (is_compensation)
)


FC_REFLECT( deip::app::review_vote_api_obj,
            (id)
            (external_id)
            (discipline_id)
            (discipline_external_id)
            (voter)
            (review_id)
            (review_external_id)
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

FC_REFLECT( deip::app::grant_application_api_obj,
            (id)
            (funding_opportunity_number)
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
            (organization_id)
            (review_committee_id)
            (treasury_id)
            (grantor)
            (funding_opportunity_number)
            (amount)
            (award_ceiling)
            (award_floor)
            (current_supply)
            (expected_number_of_awards)
            (min_number_of_positive_reviews)
            (min_number_of_applications)
            (max_number_of_research_to_grant)
            (posted_date)
            (open_date)
            (close_date)
            (additional_info)
            (target_disciplines)
            (officers)
            (distribution_type)
)

FC_REFLECT( deip::app::asset_api_obj,
            (id)
            (symbol)
            (string_symbol)
            (precision)
            (issuer)
            (description)
            (current_supply)
            (max_supply)
            (type)
            (tokenized_research)
)

FC_REFLECT( deip::app::account_balance_api_obj,
            (id)
            (asset_id)
            (asset_symbol)
            (owner)
            (amount)
            (tokenized_research)
)


FC_REFLECT( deip::app::award_api_obj,
            (id)
            (funding_opportunity_number)
            (award_number)
            (awardee)
            (creator)
            (status)
            (amount)
            (university_id)
            (university_overhead)
            (university_fee)
            (awardees)
)

FC_REFLECT( deip::app::award_recipient_api_obj,
            (id)
            (award_number)
            (subaward_number)
            (funding_opportunity_number)
            (research_id)
            (awardee)
            (source)
            (total_amount)
            (total_expenses)
            (status)
)


FC_REFLECT( deip::app::award_withdrawal_request_api_obj,
            (id)
            (award_number)
            (subaward_number)
            (payment_number)
            (requester)
            (amount)
            (description)
            (status)
            (time)
            (attachment)
)

FC_REFLECT( deip::app::nda_contract_api_obj,
            (id)
            (contract_creator)
            (party_a)
            (party_a_research_group_id)
            (party_a_signature)
            (party_b)
            (party_b_research_group_id)
            (party_b_signature)
            (disclosing_party)
            (title)
            (contract_hash)
            (status)
            (created_at)
            (start_date)
            (end_date)

)

FC_REFLECT( deip::app::nda_contract_file_access_api_obj,
            (id)
            (contract_id)
            (status)
            (requester)
            (encrypted_payload_hash)
            (encrypted_payload_iv)
            (encrypted_payload_encryption_key)
            (proof_of_encrypted_payload_encryption_key)
)

    // clang-format on
