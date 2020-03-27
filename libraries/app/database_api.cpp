#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/app/database_api.hpp>

#include <deip/protocol/get_config.hpp>

#include <deip/chain/util/reward.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/crypto/hex.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

#include <cctype>

#include <cfenv>
#include <iostream>

#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_asset.hpp>
#include <deip/chain/services/dbs_award.hpp>
#include <deip/chain/services/dbs_discipline_supply.hpp>
#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/services/dbs_research_token_sale.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/services/dbs_research_group_invite.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/services/dbs_expertise_contribution.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_research_token.hpp>
#include <deip/chain/services/dbs_expertise_allocation_proposal.hpp>
#include <deip/chain/services/dbs_vesting_balance.hpp>
#include <deip/chain/services/dbs_offer_research_tokens.hpp>
#include <deip/chain/services/dbs_grant.hpp>
#include <deip/chain/services/dbs_grant_application.hpp>
#include <deip/chain/services/dbs_grant_application_review.hpp>
#include <deip/chain/services/dbs_funding_opportunity.hpp>

#define GET_REQUIRED_FEES_MAX_RECURSION 4
#define MAX_LIMIT 1000

namespace deip {
namespace app {

class database_api_impl;

class database_api_impl : public std::enable_shared_from_this<database_api_impl>
{
public:
    database_api_impl(const deip::app::api_context& ctx);
    ~database_api_impl();

    // Subscriptions
    void set_block_applied_callback(std::function<void(const variant& block_id)> cb);

    // Blocks and transactions
    optional<signed_block_api_obj> get_block(uint32_t block_num) const;

    // Globals
    fc::variant_object get_config() const;
    dynamic_global_property_api_obj get_dynamic_global_properties() const;
    chain_id_type get_chain_id() const;

    // Keys
    vector<set<string>> get_key_references(vector<public_key_type> key) const;

    // Accounts
    vector<extended_account> get_accounts(const vector<string>& names) const;
    vector<account_id_type> get_account_references(account_id_type account_id) const;
    vector<optional<account_api_obj>> lookup_account_names(const vector<string>& account_names) const;
    set<string> lookup_accounts(const string& lower_bound_name, uint32_t limit) const;
    uint64_t get_account_count() const;

    // discipline_supplies
    vector<discipline_supply_api_obj> get_discipline_supplies(const set<string>& names) const;
    set<string> lookup_discipline_supply_grantors(const string& lower_bound_name, uint32_t limit) const;

    // Witnesses
    vector<optional<witness_api_obj>> get_witnesses(const vector<witness_id_type>& witness_ids) const;
    fc::optional<witness_api_obj> get_witness_by_account(const string& account_name) const;
    set<account_name_type> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit) const;
    uint64_t get_witness_count() const;

    // Researches

    fc::optional<research_api_obj> get_research_by_id(const research_id_type& research_id) const;
    fc::optional<research_api_obj> get_research_by_permlink(const research_group_id_type& research_group_id, const string& permlink) const;
    fc::optional<research_api_obj> get_research_by_absolute_permlink(const string& research_group_permlink, const string& research_permlink) const;

    // Research groups
    fc::optional<research_group_api_obj> get_research_group_by_id(const research_group_id_type& research_group_id) const;
    fc::optional<research_group_api_obj> get_research_group_by_permlink(const string& permlink) const;

    // Research contents
    fc::optional<research_content_api_obj> get_research_content_by_id(const research_content_id_type& id) const;
    fc::optional<research_content_api_obj> get_research_content_by_permlink(const research_id_type& research_id, const string& permlink) const;
    fc::optional<research_content_api_obj> get_research_content_by_absolute_permlink(const string& research_group_permlink, const string& research_permlink, const string& research_content_permlink) const;

    // Disciplines
    fc::optional<discipline_api_obj> get_discipline(const discipline_id_type& id) const;
    fc::optional<discipline_api_obj> get_discipline_by_name(const string& name) const;

    // Proposals
    fc::optional<proposal_api_obj> get_proposal(const proposal_id_type id) const;

    // Research group tokens
    fc::optional<research_group_token_api_obj> get_research_group_token_by_account_and_research_group_id(const account_name_type account,
                                                                                                         const research_group_id_type& research_group_id) const;

    // Expert tokens
    fc::optional<expert_token_api_obj> get_expert_token(const expert_token_id_type id) const;
    fc::optional<expert_token_api_obj> get_common_token_by_account_name(const account_name_type account_name) const;
    fc::optional<expert_token_api_obj> get_expert_token_by_account_name_and_discipline_id(const account_name_type account_name,
                                                                                          const discipline_id_type discipline_id) const;

    // Research token sales
    fc::optional<research_token_sale_api_obj> get_research_token_sale_by_id(const research_token_sale_id_type research_token_sale_id) const;
    fc::optional<research_token_sale_contribution_api_obj> get_research_token_sale_contribution_by_id(const research_token_sale_contribution_id_type research_token_sale_contribution_id) const;
    fc::optional<research_token_sale_contribution_api_obj> get_research_token_sale_contribution_by_contributor_and_research_token_sale_id(const account_name_type owner,
                                                                                                                                          const research_token_sale_id_type research_token_sale_id) const;

    // Research group invites
    fc::optional<research_group_invite_api_obj> get_research_group_invite_by_id(const research_group_invite_id_type& research_group_invite_id) const;
    fc::optional<research_group_invite_api_obj> get_research_group_invite(const account_name_type& account_name, const research_group_id_type& research_group_id) const;

    // Total votes
    fc::optional<expertise_contribution_object_api_obj> get_expertise_contribution_by_research_content_and_discipline(const research_content_id_type& research_content_id, const discipline_id_type& discipline_id) const;
    vector<expertise_contribution_object_api_obj> get_expertise_contributions_by_research(const research_id_type& research_id) const;
    vector<expertise_contribution_object_api_obj> get_expertise_contributions_by_research_and_discipline(const research_id_type& research_id, const discipline_id_type& discipline_id) const;
    vector<expertise_contribution_object_api_obj> get_expertise_contributions_by_research_content(const research_content_id_type& research_content_id) const;

    // Reviews
    fc::optional<review_api_obj> get_review_by_id(const review_id_type& review_id) const;

    // Research tokens
    fc::optional<research_token_api_obj> get_research_token_by_id(const research_token_id_type& research_token_id) const;
    fc::optional<research_token_api_obj> get_research_token_by_account_name_and_research_id(const account_name_type &account_name,
                                                                                            const research_id_type &research_id) const;

    // Expertise allocation proposals
    fc::optional<expertise_allocation_proposal_api_obj> get_expertise_allocation_proposal_by_id(const expertise_allocation_proposal_id_type& id) const;
    fc::optional<expertise_allocation_proposal_api_obj> get_expertise_allocation_proposals_by_claimer_and_discipline(const account_name_type& claimer,
                                                                                                                     const discipline_id_type& discipline_id) const;

    // Expertise allocation votes
    fc::optional<expertise_allocation_proposal_vote_api_obj> get_expertise_allocation_proposal_vote_by_id(const expertise_allocation_proposal_vote_id_type& id) const;
    fc::optional<expertise_allocation_proposal_vote_api_obj>
    get_expertise_allocation_proposal_vote_by_voter_and_expertise_allocation_proposal_id(const account_name_type& voter,
                                                                                         const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id) const;

    // Vesting balances
    fc::optional<vesting_balance_api_obj> get_vesting_balance_by_id(const vesting_balance_id_type& vesting_balance_id) const;

    // Offer research tokens
    fc::optional<offer_research_tokens_api_obj> get_offer(const offer_research_tokens_id_type& id) const;
    fc::optional<offer_research_tokens_api_obj> get_offer_by_receiver_and_research_id(const account_name_type& receiver, const research_id_type& research_id) const;

    // Grants
    fc::optional<grant_api_obj> get_grant_with_announced_application_window(const grant_id_type& id) const;
    vector<grant_api_obj> get_grants_with_announced_application_window_by_grantor(const string& grantor) const;

    // Funding opportunities
    fc::optional<funding_opportunity_api_obj> get_funding_opportunity_announcement(const funding_opportunity_id_type& id) const;
    fc::optional<funding_opportunity_api_obj> get_funding_opportunity_announcement_by_number(const string& number) const;
    vector<funding_opportunity_api_obj> get_funding_opportunity_announcements_by_organization(const research_group_id_type& research_group_id) const;
    vector<funding_opportunity_api_obj> get_funding_opportunity_announcements_listing(const uint16_t&, const uint16_t& limit) const;

    // Grant applications
    fc::optional<grant_application_api_obj> get_grant_application(const grant_application_id_type& id) const;

    // Assets
    fc::optional<asset_api_obj> get_asset(const asset_id_type& id) const;
    fc::optional<asset_api_obj> get_asset_by_string_symbol(const std::string& string_symbol) const;

    // Account balances
    fc::optional<account_balance_api_obj> get_account_balance(const account_balance_id_type& id) const;
    fc::optional<account_balance_api_obj> get_account_balance_by_owner_and_asset_symbol(const account_name_type& owner, const string& symbol) const;

    // Organizational contracts
    fc::optional<research_group_organization_contract_api_obj> get_organizational_contract(const research_group_organization_contract_id_type& id) const;
    fc::optional<research_group_organization_contract_api_obj> get_organizational_contract_by_organization_and_research_group_and_type(const research_group_id_type& organization_id, const research_group_id_type& research_group_id, const uint16_t& type) const;
    vector<research_group_organization_contract_api_obj> get_organizational_contracts_by_research_group(const research_group_id_type& research_group_id) const;
    vector<research_group_organization_contract_api_obj> get_organizational_contracts_by_organization(const research_group_id_type& organization_id) const;

    // Discipline supplies
    fc::optional<discipline_supply_api_obj> get_discipline_supply(const discipline_supply_id_type& id) const;

    // Awards
    fc::optional<award_api_obj> get_award(const string& award_number) const;
    vector<award_api_obj> get_awards_by_funding_opportunity(const string& funding_opportunity_number) const;

    // Awardees
    fc::optional<award_recipient_api_obj> get_award_recipient(const award_recipient_id_type& id) const;
    vector<award_recipient_api_obj> get_award_recipients_by_award(const string& award_number) const;
    vector<award_recipient_api_obj> get_award_recipients_by_account(const account_name_type& awardee) const;
    vector<award_recipient_api_obj> get_award_recipients_by_funding_opportunity(const string& funding_opportunity_number) const;

    fc::optional<award_withdrawal_request_api_obj> get_award_withdrawal_request(const string& award_number, const string& payment_number) const;
    vector<award_withdrawal_request_api_obj> get_award_withdrawal_requests_by_award(const string& award_number) const;
    vector<award_withdrawal_request_api_obj> get_award_withdrawal_requests_by_award_and_subaward(const string& award_number, const string& subaward_number) const;
    vector<award_withdrawal_request_api_obj> get_award_withdrawal_requests_by_award_and_status(const string& award_number, const award_withdrawal_request_status& status) const;

    // Authority / validation
    std::string get_transaction_hex(const signed_transaction& trx) const;
    set<public_key_type> get_required_signatures(const signed_transaction& trx,
                                                 const flat_set<public_key_type>& available_keys) const;
    set<public_key_type> get_potential_signatures(const signed_transaction& trx) const;
    bool verify_authority(const signed_transaction& trx) const;
    bool verify_account_authority(const string& name_or_id, const flat_set<public_key_type>& signers) const;

    // signal handlers
    void on_applied_block(const chain::signed_block& b);

    std::function<void(const fc::variant&)> _block_applied_callback;

    deip::chain::database& _db;

    boost::signals2::scoped_connection _block_applied_connection;

    bool _disable_get_block = false;
};

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Subscriptions                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api::set_block_applied_callback(std::function<void(const variant& block_id)> cb)
{
    my->_db.with_read_lock([&]() { my->set_block_applied_callback(cb); });
}

void database_api_impl::on_applied_block(const chain::signed_block& b)
{
    try
    {
        _block_applied_callback(fc::variant(signed_block_header(b)));
    }
    catch (...)
    {
        _block_applied_connection.release();
    }
}

void database_api_impl::set_block_applied_callback(std::function<void(const variant& block_header)> cb)
{
    _block_applied_callback = cb;
    _block_applied_connection = connect_signal(_db.applied_block, *this, &database_api_impl::on_applied_block);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api(const deip::app::api_context& ctx)
    : my(new database_api_impl(ctx))
    , _app(ctx.app)
{
}

database_api::~database_api()
{
}

database_api_impl::database_api_impl(const deip::app::api_context& ctx)
    : _db(*ctx.app.chain_database())
{
    wlog("creating database api ${x}", ("x", int64_t(this)));

    _disable_get_block = ctx.app._disable_get_block;
}

database_api_impl::~database_api_impl()
{
    elog("freeing database api ${x}", ("x", int64_t(this)));
}

void database_api::on_api_startup()
{
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////


optional<signed_block_api_obj> database_api::get_block(uint32_t block_num) const
{
    FC_ASSERT(!my->_disable_get_block, "get_block is disabled on this node.");

    return my->_db.with_read_lock([&]() { return my->get_block(block_num); });
}

optional<signed_block_api_obj> database_api_impl::get_block(uint32_t block_num) const
{
    return _db.fetch_block_by_number(block_num);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////=

fc::variant_object database_api::get_config() const
{
    return my->_db.with_read_lock([&]() { return my->get_config(); });
}

fc::variant_object database_api_impl::get_config() const
{
    return deip::protocol::get_config();
}

dynamic_global_property_api_obj database_api::get_dynamic_global_properties() const
{
    return my->_db.with_read_lock([&]() { return my->get_dynamic_global_properties(); });
}

chain_properties database_api::get_chain_properties() const
{
    return my->_db.with_read_lock([&]() { return my->_db.get_witness_schedule_object().median_props; });
}

dynamic_global_property_api_obj database_api_impl::get_dynamic_global_properties() const
{
    return dynamic_global_property_api_obj(_db.get(dynamic_global_property_id_type()), _db);
}

chain_id_type database_api_impl::get_chain_id() const
{
    return _db.get_chain_id();
}

witness_schedule_api_obj database_api::get_witness_schedule() const
{
    return my->_db.with_read_lock([&]() { return my->_db.get(witness_schedule_id_type()); });
}

hardfork_version database_api::get_hardfork_version() const
{
    return my->_db.with_read_lock([&]() { return my->_db.get(hardfork_property_id_type()).current_hardfork_version; });
}

scheduled_hardfork database_api::get_next_scheduled_hardfork() const
{
    return my->_db.with_read_lock([&]() {
        scheduled_hardfork shf;
        const auto& hpo = my->_db.get(hardfork_property_id_type());
        shf.hf_version = hpo.next_hardfork;
        shf.live_time = hpo.next_hardfork_time;
        return shf;
    });
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Keys                                                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<set<string>> database_api::get_key_references(vector<public_key_type> key) const
{
    return my->_db.with_read_lock([&]() { return my->get_key_references(key); });
}

/**
 *  @return all accounts that referr to the key or account id in their owner or active authorities.
 */
vector<set<string>> database_api_impl::get_key_references(vector<public_key_type> keys) const
{
    FC_ASSERT(false,
              "database_api::get_key_references has been deprecated. Please use "
              "account_by_key_api::get_key_references instead.");
    vector<set<string>> final_result;
    return final_result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<extended_account> database_api::get_accounts(const vector<string>& names) const
{
    return my->_db.with_read_lock([&]() { return my->get_accounts(names); });
}

vector<extended_account> database_api_impl::get_accounts(const vector<string>& names) const
{
    const auto& idx = _db.get_index<account_index>().indices().get<by_name>();
    const auto& vidx = _db.get_index<witness_vote_index>().indices().get<by_account_witness>();
    vector<extended_account> results;

    for (auto name : names)
    {
        auto itr = idx.find(name);
        if (itr != idx.end())
        {
            results.push_back(extended_account(*itr, _db));

            auto vitr = vidx.lower_bound(boost::make_tuple(itr->id, witness_id_type()));
            while (vitr != vidx.end() && vitr->account == itr->id)
            {
                results.back().witness_votes.insert(_db.get(vitr->witness).owner);
                ++vitr;
            }
        }
    }

    return results;
}

vector<account_id_type> database_api::get_account_references(account_id_type account_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_account_references(account_id); });
}

vector<account_id_type> database_api_impl::get_account_references(account_id_type account_id) const
{
    /*const auto& idx = _db.get_index<account_index>();
    const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
    const auto& refs = aidx.get_secondary_index<deip::chain::account_member_index>();
    auto itr = refs.account_to_account_memberships.find(account_id);
    vector<account_id_type> result;

    if( itr != refs.account_to_account_memberships.end() )
    {
       result.reserve( itr->second.size() );
       for( auto item : itr->second ) result.push_back(item);
    }
    return result;*/
    FC_ASSERT(false, "database_api::get_account_references --- Needs to be refactored for deip.");
}

vector<optional<account_api_obj>> database_api::lookup_account_names(const vector<string>& account_names) const
{
    return my->_db.with_read_lock([&]() { return my->lookup_account_names(account_names); });
}

vector<optional<account_api_obj>> database_api_impl::lookup_account_names(const vector<string>& account_names) const
{
    vector<optional<account_api_obj>> result;
    result.reserve(account_names.size());

    for (auto& name : account_names)
    {
        auto itr = _db.find<account_object, by_name>(name);

        if (itr)
        {
            result.push_back(account_api_obj(*itr, _db));
        }
        else
        {
            result.push_back(optional<account_api_obj>());
        }
    }

    return result;
}

set<string> database_api::lookup_accounts(const string& lower_bound_name, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() { return my->lookup_accounts(lower_bound_name, limit); });
}

set<string> database_api_impl::lookup_accounts(const string& lower_bound_name, uint32_t limit) const
{
    FC_ASSERT(limit <= MAX_LIMIT);
    const auto& accounts_by_name = _db.get_index<account_index>().indices().get<by_name>();
    set<string> result;

    for (auto itr = accounts_by_name.lower_bound(lower_bound_name); limit-- && itr != accounts_by_name.end(); ++itr)
    {
        result.insert(itr->name);
    }

    return result;
}

uint64_t database_api::get_account_count() const
{
    return my->_db.with_read_lock([&]() { return my->get_account_count(); });
}

vector<account_api_obj> database_api::get_all_accounts() const
{
    vector<account_api_obj> results;

    const auto& idx = my->_db.get_index<account_index>().indicies().get<by_id>();
    auto it = idx.lower_bound(0);
    const auto it_end = idx.cend();
    while (it != it_end)
    {
        results.push_back(account_api_obj(*it, my->_db));
        ++it;
    }

    return results;
}

uint64_t database_api_impl::get_account_count() const
{
    return _db.get_index<account_index>().indices().size();
}

vector<owner_authority_history_api_obj> database_api::get_owner_history(string account) const
{
    return my->_db.with_read_lock([&]() {
        vector<owner_authority_history_api_obj> results;

        const auto& hist_idx = my->_db.get_index<owner_authority_history_index>().indices().get<by_account>();
        auto itr = hist_idx.lower_bound(account);

        while (itr != hist_idx.end() && itr->account == account)
        {
            results.push_back(owner_authority_history_api_obj(*itr));
            ++itr;
        }

        return results;
    });
}

optional<account_recovery_request_api_obj> database_api::get_recovery_request(string account) const
{
    return my->_db.with_read_lock([&]() {
        optional<account_recovery_request_api_obj> result;

        const auto& rec_idx = my->_db.get_index<account_recovery_request_index>().indices().get<by_account>();
        auto req = rec_idx.find(account);

        if (req != rec_idx.end())
            result = account_recovery_request_api_obj(*req);

        return result;
    });
}

vector<withdraw_route> database_api::get_withdraw_routes(string account, withdraw_route_type type) const
{
    return my->_db.with_read_lock([&]() {
        vector<withdraw_route> result;

        const auto& acc = my->_db.get_account(account);

        if (type == outgoing || type == all)
        {
            const auto& by_route = my->_db.get_index<withdraw_common_tokens_route_index>().indices().get<by_withdraw_route>();
            auto route = by_route.lower_bound(acc.id);

            while (route != by_route.end() && route->from_account == acc.id)
            {
                withdraw_route r;
                r.from_account = account;
                r.to_account = my->_db.get(route->to_account).name;
                r.percent = route->percent;
                r.auto_common_token = route->auto_common_token;

                result.push_back(r);

                ++route;
            }
        }

        if (type == incoming || type == all)
        {
            const auto& by_dest = my->_db.get_index<withdraw_common_tokens_route_index>().indices().get<by_destination>();
            auto route = by_dest.lower_bound(acc.id);

            while (route != by_dest.end() && route->to_account == acc.id)
            {
                withdraw_route r;
                r.from_account = my->_db.get(route->from_account).name;
                r.to_account = account;
                r.percent = route->percent;
                r.auto_common_token = route->auto_common_token;

                result.push_back(r);

                ++route;
            }
        }

        return result;
    });
}

optional<account_bandwidth_api_obj> database_api::get_account_bandwidth(string account,
                                                                        witness::bandwidth_type type) const
{
    optional<account_bandwidth_api_obj> result;

    if (my->_db.has_index<witness::account_bandwidth_index>())
    {
        auto band = my->_db.find<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
            boost::make_tuple(account, type));
        if (band != nullptr)
            result = *band;
    }

    return result;
}

vector<account_api_obj> database_api::get_accounts_by_expert_discipline(const discipline_id_type& discipline_id, uint32_t from, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() {
        FC_ASSERT(limit <= MAX_LIMIT && limit > 0);
        FC_ASSERT(discipline_id > 0, "Cannot use root discipline.");
        FC_ASSERT(from >= 0, "From must be >= 0");

        vector<account_api_obj> result;
        result.reserve(limit);

        chain::dbs_account& account_service = my->_db.obtain_service<chain::dbs_account>();

        auto accounts_by_expert_discipline = account_service.get_accounts_by_expert_discipline(discipline_id);

        if (from >= accounts_by_expert_discipline.size())
            return result;

        if (from + limit >= accounts_by_expert_discipline.size())
            limit = accounts_by_expert_discipline.size() - from;

        for (auto i = from; i < from + limit; i++)
            result.push_back(account_api_obj(accounts_by_expert_discipline[i].get(), my->_db));

        return result;
    });
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<witness_api_obj>> database_api::get_witnesses(const vector<witness_id_type>& witness_ids) const
{
    return my->_db.with_read_lock([&]() { return my->get_witnesses(witness_ids); });
}

vector<optional<witness_api_obj>> database_api_impl::get_witnesses(const vector<witness_id_type>& witness_ids) const
{
    vector<optional<witness_api_obj>> result;
    result.reserve(witness_ids.size());
    std::transform(witness_ids.begin(), witness_ids.end(), std::back_inserter(result),
                   [this](witness_id_type id) -> optional<witness_api_obj> {
                       if (auto o = _db.find(id))
                           return *o;
                       return {};
                   });
    return result;
}

fc::optional<witness_api_obj> database_api::get_witness_by_account(string account_name) const
{
    return my->_db.with_read_lock([&]() { return my->get_witness_by_account(account_name); });
}

vector<witness_api_obj> database_api::get_witnesses_by_vote(string from, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() {
        // idump((from)(limit));
        FC_ASSERT(limit <= MAX_LIMIT);

        vector<witness_api_obj> result;
        result.reserve(limit);

        const auto& name_idx = my->_db.get_index<witness_index>().indices().get<by_name>();
        const auto& vote_idx = my->_db.get_index<witness_index>().indices().get<by_vote_name>();

        auto itr = vote_idx.begin();
        if (from.size())
        {
            auto nameitr = name_idx.find(from);
            FC_ASSERT(nameitr != name_idx.end(), "invalid witness name ${n}", ("n", from));
            itr = vote_idx.iterator_to(*nameitr);
        }

        while (itr != vote_idx.end() && result.size() < limit && itr->votes > 0)
        {
            result.push_back(witness_api_obj(*itr));
            ++itr;
        }
        return result;
    });
}

fc::optional<witness_api_obj> database_api_impl::get_witness_by_account(const string& account_name) const
{
    const auto& idx = _db.get_index<witness_index>().indices().get<by_name>();
    auto itr = idx.find(account_name);
    if (itr != idx.end())
        return witness_api_obj(*itr);
    return {};
}

set<account_name_type> database_api::lookup_witness_accounts(const string& lower_bound_name, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() { return my->lookup_witness_accounts(lower_bound_name, limit); });
}

set<account_name_type> database_api_impl::lookup_witness_accounts(const string& lower_bound_name, uint32_t limit) const
{
    FC_ASSERT(limit <= MAX_LIMIT);
    const auto& witnesses_by_id = _db.get_index<witness_index>().indices().get<by_id>();

    // get all the names and look them all up, sort them, then figure out what
    // records to return.  This could be optimized, but we expect the
    // number of witnesses to be few and the frequency of calls to be rare
    set<account_name_type> witnesses_by_account_name;
    for (const witness_api_obj& witness : witnesses_by_id)
        if (witness.owner >= lower_bound_name) // we can ignore anything below lower_bound_name
            witnesses_by_account_name.insert(witness.owner);

    auto end_iter = witnesses_by_account_name.begin();
    while (end_iter != witnesses_by_account_name.end() && limit--)
        ++end_iter;
    witnesses_by_account_name.erase(end_iter, witnesses_by_account_name.end());
    return witnesses_by_account_name;
}

uint64_t database_api::get_witness_count() const
{
    return my->_db.with_read_lock([&]() { return my->get_witness_count(); });
}

uint64_t database_api_impl::get_witness_count() const
{
    return _db.get_index<witness_index>().indices().size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::string database_api::get_transaction_hex(const signed_transaction& trx) const
{
    return my->_db.with_read_lock([&]() { return my->get_transaction_hex(trx); });
}

std::string database_api_impl::get_transaction_hex(const signed_transaction& trx) const
{
    return fc::to_hex(fc::raw::pack(trx));
}

set<public_key_type> database_api::get_required_signatures(const signed_transaction& trx,
                                                           const flat_set<public_key_type>& available_keys) const
{
    return my->_db.with_read_lock([&]() { return my->get_required_signatures(trx, available_keys); });
}

set<public_key_type> database_api_impl::get_required_signatures(const signed_transaction& trx,
                                                                const flat_set<public_key_type>& available_keys) const
{
    //   wdump((trx)(available_keys));
    auto result = trx.get_required_signatures(
        get_chain_id(), available_keys,
        [&](string account_name) {
            return authority(_db.get<account_authority_object, by_account>(account_name).active);
        },
        [&](string account_name) {
            return authority(_db.get<account_authority_object, by_account>(account_name).owner);
        },
        [&](string account_name) {
            return authority(_db.get<account_authority_object, by_account>(account_name).posting);
        },
        DEIP_MAX_SIG_CHECK_DEPTH);
    //   wdump((result));
    return result;
}

set<public_key_type> database_api::get_potential_signatures(const signed_transaction& trx) const
{
    return my->_db.with_read_lock([&]() { return my->get_potential_signatures(trx); });
}

set<public_key_type> database_api_impl::get_potential_signatures(const signed_transaction& trx) const
{
    //   wdump((trx));
    set<public_key_type> result;
    trx.get_required_signatures(
        get_chain_id(), flat_set<public_key_type>(),
        [&](account_name_type account_name) {
            const auto& auth = _db.get<account_authority_object, by_account>(account_name).active;
            for (const auto& k : auth.get_keys())
                result.insert(k);
            return authority(auth);
        },
        [&](account_name_type account_name) {
            const auto& auth = _db.get<account_authority_object, by_account>(account_name).owner;
            for (const auto& k : auth.get_keys())
                result.insert(k);
            return authority(auth);
        },
        [&](account_name_type account_name) {
            const auto& auth = _db.get<account_authority_object, by_account>(account_name).posting;
            for (const auto& k : auth.get_keys())
                result.insert(k);
            return authority(auth);
        },
        DEIP_MAX_SIG_CHECK_DEPTH);

    //   wdump((result));
    return result;
}

bool database_api::verify_authority(const signed_transaction& trx) const
{
    return my->_db.with_read_lock([&]() { return my->verify_authority(trx); });
}

bool database_api_impl::verify_authority(const signed_transaction& trx) const
{
    trx.verify_authority(get_chain_id(),
                         [&](string account_name) {
                             return authority(_db.get<account_authority_object, by_account>(account_name).active);
                         },
                         [&](string account_name) {
                             return authority(_db.get<account_authority_object, by_account>(account_name).owner);
                         },
                         [&](string account_name) {
                             return authority(_db.get<account_authority_object, by_account>(account_name).posting);
                         },
                         DEIP_MAX_SIG_CHECK_DEPTH);
    return true;
}

bool database_api::verify_account_authority(const string& name_or_id, const flat_set<public_key_type>& signers) const
{
    return my->_db.with_read_lock([&]() { return my->verify_account_authority(name_or_id, signers); });
}

bool database_api_impl::verify_account_authority(const string& name, const flat_set<public_key_type>& keys) const
{
    FC_ASSERT(name.size() > 0);
    auto account = _db.find<account_object, by_name>(name);
    FC_ASSERT(account, "no such account");

    /// reuse trx.verify_authority by creating a dummy transfer
    signed_transaction trx;
    transfer_operation op;
    op.from = account->name;
    trx.operations.emplace_back(op);

    return verify_authority(trx);
}

u256 to256(const fc::uint128& t)
{
    u256 result(t.high_bits());
    result <<= 65;
    result += t.low_bits();
    return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// discipline_supplies                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////
vector<discipline_supply_api_obj> database_api::get_discipline_supplies(const set<string>& names) const
{
    return my->_db.with_read_lock([&]() { return my->get_discipline_supplies(names); });
}

vector<discipline_supply_api_obj> database_api_impl::get_discipline_supplies(const set<string>& names) const
{
    FC_ASSERT(names.size() <= DEIP_LIMIT_API_DISCIPLINE_SUPPLIES_LIST_SIZE, "names size must be less or equal than ${1}",
              ("1", DEIP_LIMIT_API_DISCIPLINE_SUPPLIES_LIST_SIZE));

    vector<discipline_supply_api_obj> results;

    chain::dbs_discipline_supply& discipline_supply_service = _db.obtain_service<chain::dbs_discipline_supply>();

    for (const auto& name : names)
    {
        auto discipline_supplies = discipline_supply_service.get_discipline_supplies_by_grantor(name);
        if (results.size() + discipline_supplies.size() > DEIP_LIMIT_API_DISCIPLINE_SUPPLIES_LIST_SIZE)
        {
            break;
        }

        for (const chain::discipline_supply_object& discipline_supply : discipline_supplies)
        {
            results.push_back(discipline_supply_api_obj(discipline_supply));
        }
    }

    return results;
}

set<string> database_api::lookup_discipline_supply_grantors(const string& lower_bound_name, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() { return my->lookup_discipline_supply_grantors(lower_bound_name, limit); });
}

set<string> database_api_impl::lookup_discipline_supply_grantors(const string& lower_bound_name, uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_LIMIT_API_DISCIPLINE_SUPPLIES_LIST_SIZE, "limit must be less or equal than ${1}",
              ("1", DEIP_LIMIT_API_DISCIPLINE_SUPPLIES_LIST_SIZE));

    chain::dbs_discipline_supply& discipline_supply_service = _db.obtain_service<chain::dbs_discipline_supply>();

    return discipline_supply_service.lookup_discipline_supply_grantors(lower_bound_name, limit);
}

/**
 *  This call assumes root already stored as part of state, it will
 *  modify root.replies to contain links to the reply posts and then
 *  add the reply discussions to the state. This method also fetches
 *  any accounts referenced by authors.
 *
 */

vector<account_name_type> database_api::get_active_witnesses() const
{
    return my->_db.with_read_lock([&]() {
        const auto& wso = my->_db.get_witness_schedule_object();
        size_t n = wso.current_shuffled_witnesses.size();
        vector<account_name_type> result;
        result.reserve(n);
        for (size_t i = 0; i < n; i++)
            result.push_back(wso.current_shuffled_witnesses[i]);
        return result;
    });
}

state database_api::get_state(string path) const
{
    return my->_db.with_read_lock([&]() {
        state _state;
        _state.props = get_dynamic_global_properties();
        _state.current_route = path;

        try
        {
            if (path.size() && path[0] == '/')
                path = path.substr(1); /// remove '/' from front

            if (!path.size())
                path = "trending";

            set<string> accounts;

            vector<string> part;
            part.reserve(4);
            boost::split(part, path, boost::is_any_of("/"));
            part.resize(std::max(part.size(), size_t(4))); // at least 4

            auto tag = fc::to_lower(part[1]);

            if (part[0].size() && part[0][0] == '@')
            {
                auto acnt = part[0].substr(1);
                _state.accounts[acnt] = extended_account(my->_db.get_account(acnt), my->_db);

                // auto& eacnt = _state.accounts[acnt];
                if (part[1] == "transfers")
                {
                    // TODO: rework this garbage method - split it into sensible parts
                    // auto history = get_account_history(acnt, uint64_t(-1), 10000);
                    // for (auto& item : history)
                    // {
                    //     switch (item.second.op.which())
                    //     {
                    //     case operation::tag<withdraw_common_tokens_operation>::value:
                    //     case operation::tag<transfer_operation>::value:
                    //     case operation::tag<account_witness_vote_operation>::value:
                    //     case operation::tag<account_witness_proxy_operation>::value:
                    //         //   eacnt.vote_history[item.first] =  item.second;
                    //         break;
                    //     case operation::tag<account_create_operation>::value:
                    //     case operation::tag<account_update_operation>::value:
                    //     case operation::tag<witness_update_operation>::value:
                    //     case operation::tag<producer_reward_operation>::value:
                    //     default:
                    //         eacnt.other_history[item.first] = item.second;
                    //     }
                    // }
                }
            }
            /// pull a complete discussion

            else if (part[0] == "witnesses" || part[0] == "~witnesses")
            {
                auto wits = get_witnesses_by_vote("", 50);
                for (const auto& w : wits)
                {
                    _state.witnesses[w.owner] = w;
                }
            }

            else
            {
                elog("What... no matches");
            }

            for (const auto& a : accounts)
            {
                _state.accounts.erase("");
                _state.accounts[a] = extended_account(my->_db.get_account(a), my->_db);
            }

            _state.witness_schedule = my->_db.get_witness_schedule_object();
        }
        catch (const fc::exception& e)
        {
            _state.error = e.to_detail_string();
        }
        return _state;
    });
}

vector<discipline_api_obj> database_api::get_all_disciplines() const
{
    return my->_db.with_read_lock([&]() {
        vector<discipline_api_obj> results;

        chain::dbs_discipline &discipline_service = my->_db.obtain_service<chain::dbs_discipline>();

        auto disciplines = discipline_service.get_disciplines();

        for (const chain::discipline_object &discipline : disciplines) {
            results.push_back(discipline_api_obj(discipline));
        }

        return results;
    });
}

fc::optional<discipline_api_obj> database_api::get_discipline(const discipline_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_discipline(id); });
}

fc::optional<discipline_api_obj> database_api_impl::get_discipline(const discipline_id_type& id) const
{
    const auto& idx = _db.get_index<discipline_index>().indices().get<by_id>();
    auto itr = idx.find(id);
    if (itr != idx.end()) {
        chain::dbs_discipline &discipline_service = _db.obtain_service<chain::dbs_discipline>();
        return discipline_service.get_discipline(id);
    }

    return {};
}

fc::optional<discipline_api_obj> database_api::get_discipline_by_name(const string& name) const
{
    return my->_db.with_read_lock([&]() { return my->get_discipline_by_name(name); });
}

fc::optional<discipline_api_obj> database_api_impl::get_discipline_by_name(const string& name) const
{
    const auto& idx = _db.get_index<discipline_index>().indices().get<by_discipline_name>();
    auto itr = idx.find(name, fc::strcmp_less());

    if (itr != idx.end()) {
        chain::dbs_discipline &discipline_service = _db.obtain_service<chain::dbs_discipline>();
        return discipline_service.get_discipline_by_name(name);
    }

    return {};
}

vector<discipline_api_obj> database_api::get_disciplines_by_parent_id(const discipline_id_type parent_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<discipline_api_obj> results;

        chain::dbs_discipline &discipline_service = my->_db.obtain_service<chain::dbs_discipline>();

        auto disciplines = discipline_service.get_disciplines_by_parent_id(parent_id);

        for (const chain::discipline_object &discipline : disciplines) {
            results.push_back(discipline_api_obj(discipline));
        }

        return results;
    });
}


optional<research_api_obj> database_api::get_research_by_id(const research_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_by_id(id); });
}

fc::optional<research_api_obj> database_api_impl::get_research_by_id(const research_id_type& research_id) const
{
    const auto& idx = _db.get_index<research_index>().indices().get<by_id>();
    auto itr = idx.find(research_id);
    if (itr != idx.end()) {
        chain::dbs_research &research_service = _db.obtain_service<chain::dbs_research>();
        chain::dbs_discipline &discipline_service = _db.obtain_service<chain::dbs_discipline>();

        auto& research = research_service.get_research(research_id);

        vector<discipline_api_obj> disciplines;
        chain::dbs_research_discipline_relation& research_discipline_relation_service
                = _db.obtain_service<chain::dbs_research_discipline_relation>();

        auto research_discipline_relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research_id);

        for (const chain::research_discipline_relation_object& research_discipline_relation : research_discipline_relations)
        {
            disciplines.push_back( discipline_service.get_discipline(research_discipline_relation.discipline_id));
        }

        optional<research_group_api_obj> research_group = get_research_group_by_id(research.research_group_id);

        return research_api_obj(research, disciplines, research_group->permlink);
    }

    return {};
}

fc::optional<research_api_obj> database_api::get_research_by_permlink(const research_group_id_type& research_group_id, const string& permlink) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_by_permlink(research_group_id, permlink); });
}

fc::optional<research_api_obj> database_api_impl::get_research_by_permlink(const research_group_id_type& research_group_id, const string& permlink) const
{
    const auto& idx = _db.get_index<research_index>().indices().get<by_permlink>();
    auto itr = idx.find(std::make_tuple(research_group_id, permlink));
    if (itr != idx.end()) {
        chain::dbs_research &research_service = _db.obtain_service<chain::dbs_research>();
        chain::dbs_discipline &discipline_service = _db.obtain_service<chain::dbs_discipline>();

        auto& research = research_service.get_research_by_permlink(research_group_id, permlink);

        vector<discipline_api_obj> disciplines;
        chain::dbs_research_discipline_relation& research_discipline_relation_service
                = _db.obtain_service<chain::dbs_research_discipline_relation>();

        auto research_discipline_relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research.id);

        for (const chain::research_discipline_relation_object& research_discipline_relation : research_discipline_relations)
        {
            disciplines.push_back( discipline_service.get_discipline(research_discipline_relation.discipline_id));
        }

        optional<research_group_api_obj> research_group = get_research_group_by_id(research.research_group_id);

        return research_api_obj(research, disciplines, research_group->permlink);
    }

    return {};
}

fc::optional<research_api_obj> database_api::get_research_by_absolute_permlink(const string& research_group_permlink, const string& research_permlink) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_by_absolute_permlink(research_group_permlink, research_permlink); });
}

fc::optional<research_api_obj> database_api_impl::get_research_by_absolute_permlink(const string& research_group_permlink, const string& research_permlink) const
{
    fc::optional<research_api_obj> research;
    const auto& idx = _db.get_index<research_group_index>().indices().get<by_permlink>();
    auto itr = idx.find(research_group_permlink, fc::strcmp_less());

    if (itr != idx.end()) 
    {
        const research_group_object& research_group = *itr;
        research = get_research_by_permlink(research_group.id, research_permlink);
    }

    return research;
}

vector<research_api_obj> database_api::get_researches_by_discipline_id(const uint64_t from,
                                                                       const uint32_t limit,
                                                                       const discipline_id_type& discipline_id) const
{
    return my->_db.with_read_lock([&]() {
        FC_ASSERT(limit <= MAX_LIMIT);

        vector<research_api_obj> result;
        vector<research_id_type> researches;
        result.reserve(limit);

        const auto& rdr_idx = my->_db.get_index<research_discipline_relation_index>().indicies().get<by_discipline_id>();

        auto rdr_itr = rdr_idx.find(discipline_id);

        while (rdr_itr != rdr_idx.end())
        {
            researches.push_back(rdr_itr->research_id);
            ++rdr_itr;
        }
        std::sort(researches.begin(), researches.end());

        FC_ASSERT(from + limit <= researches.size(), "from + limit cannot be bigger than size ${n}", ("n", researches.size()));

        for (auto i = from; i < from + limit; i++) {
            auto& research = my->_db.get<research_object>(researches[i]);

            vector<discipline_api_obj> disciplines = get_disciplines_by_research(research.id);
            auto research_group = get_research_group_by_id(research.research_group_id);
            result.push_back(research_api_obj(research, disciplines, research_group->permlink));
        }

        return result;
    });
}

vector<research_api_obj> database_api::get_researches_by_research_group_id(const research_group_id_type& research_group_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_api_obj> results;

        chain::dbs_research &research_service = my->_db.obtain_service<chain::dbs_research>();
        auto researches = research_service.get_researches_by_research_group(research_group_id);

        for (const chain::research_object &research : researches) {
            vector<discipline_api_obj> disciplines = get_disciplines_by_research(research.id);
            auto research_group = get_research_group_by_id(research.research_group_id);
            results.push_back(research_api_obj(research, disciplines, research_group->permlink));
        }

        return results;
    });
}

bool database_api::check_research_existence_by_permlink(const research_group_id_type& research_group_id, const string& permlink) const
{
    const auto& idx = my->_db.get_index<research_index>().indices().get<by_permlink>();
    auto itr = idx.find(std::make_tuple(research_group_id, permlink));
    return itr != idx.end();
}

fc::optional<research_content_api_obj> database_api::get_research_content_by_id(const research_content_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_content_by_id(id); });
}

fc::optional<research_content_api_obj> database_api_impl::get_research_content_by_id(const research_content_id_type& id) const
{
    const auto& idx = _db.get_index<research_content_index>().indices().get<by_id>();
    auto itr = idx.find(id);

    if (itr != idx.end()) {
        chain::dbs_research_content &research_content_service = _db.obtain_service<chain::dbs_research_content>();
        return research_content_service.get(id);
    }

    return {};
}

fc::optional<research_content_api_obj> database_api::get_research_content_by_permlink(const research_id_type& research_id, const string& permlink) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_content_by_permlink(research_id, permlink); });
}

fc::optional<research_content_api_obj> database_api_impl::get_research_content_by_permlink(const research_id_type& research_id, const string& permlink) const
{
    const auto& idx = _db.get_index<research_content_index>().indices().get<by_permlink>();
    auto itr = idx.find(std::make_tuple(research_id, permlink));
    if (itr != idx.end()) {
        chain::dbs_research_content &research_content_service = _db.obtain_service<chain::dbs_research_content>();
        return research_content_service.get_by_permlink(research_id, permlink);
    }

    return {};
}

fc::optional<research_content_api_obj> database_api::get_research_content_by_absolute_permlink(const string& research_group_permlink, const string& research_permlink, const string& research_content_permlink) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_content_by_absolute_permlink(research_group_permlink, research_permlink, research_content_permlink); });
}

fc::optional<research_content_api_obj>
database_api_impl::get_research_content_by_absolute_permlink(const string& research_group_permlink, const string& research_permlink, const string& research_content_permlink) const
{
    fc::optional<research_content_api_obj> research_content;
    const auto& idx = _db.get_index<research_group_index>().indices().get<by_permlink>();
    auto itr = idx.find(research_group_permlink, fc::strcmp_less());

    if (itr != idx.end()) {
        const research_group_object& research_group = *itr;
        fc::optional<research_api_obj> research = get_research_by_permlink(research_group.id, research_permlink);
        if (research.valid()) 
        {
            research_content = get_research_content_by_permlink(research->id, research_content_permlink);
        }
    }

    return research_content;
}

vector<research_content_api_obj> database_api::get_all_research_content(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_content_api_obj> results;
        chain::dbs_research_content &research_content_service = my->_db.obtain_service<chain::dbs_research_content>();
        auto contents = research_content_service.get_by_research_id(research_id);

        for (const chain::research_content_object &content : contents) {
            results.push_back(research_content_api_obj(content));
        }

        return results;
    });
}

vector<research_content_api_obj> database_api::get_research_content_by_type(const research_id_type& research_id, const research_content_type& type) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_content_api_obj> results;
        chain::dbs_research_content &research_content_service = my->_db.obtain_service<chain::dbs_research_content>();
        auto contents = research_content_service.get_by_research_and_type(research_id, type);

        for (const chain::research_content_object &content : contents) {
            results.push_back(research_content_api_obj(content));
        }
        return results;
    });
}

vector<research_content_api_obj> database_api::get_all_milestones_by_research_id(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_content_api_obj> results;
        chain::dbs_research_content &research_content_service = my->_db.obtain_service<chain::dbs_research_content>();
        auto contents = research_content_service.get_all_milestones_by_research_id(research_id);

        for (const chain::research_content_object &content : contents) {
            results.push_back(research_content_api_obj(content));
        }
        return results;
    });
}

fc::optional<expert_token_api_obj> database_api::get_expert_token(const expert_token_id_type id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expert_token(id); });
}

fc::optional<expert_token_api_obj> database_api_impl::get_expert_token(const expert_token_id_type id) const
{
    const auto& idx = _db.get_index<expert_token_index>().indices().get<by_id>();
    auto itr = idx.find(id);

    if (itr != idx.end()) {
        chain::dbs_discipline& discipline_service = _db.obtain_service<chain::dbs_discipline>();
        auto& discipline = discipline_service.get_discipline(itr->discipline_id);
        return expert_token_api_obj(*itr, fc::to_string(discipline.name));
    }

    return {};
}

vector<expert_token_api_obj> database_api::get_expert_tokens_by_account_name(const account_name_type account_name) const
{
    return my->_db.with_read_lock([&]() {
        vector<expert_token_api_obj> results;
        chain::dbs_expert_token &expert_token_service = my->_db.obtain_service<chain::dbs_expert_token>();
        chain::dbs_discipline& discipline_service = my->_db.obtain_service<chain::dbs_discipline>();
        auto expert_tokens = expert_token_service.get_expert_tokens_by_account_name(account_name);

        for (const chain::expert_token_object &expert_token : expert_tokens) {
            auto& discipline = discipline_service.get_discipline(expert_token.discipline_id);
            if (expert_token.discipline_id != 0)
                results.push_back(expert_token_api_obj(expert_token, fc::to_string(discipline.name)));
        }
        return results;
    });
}

vector<expert_token_api_obj> database_api::get_expert_tokens_by_discipline_id(const discipline_id_type discipline_id) const
{
      return my->_db.with_read_lock([&]() {
        vector<expert_token_api_obj> results;

        chain::dbs_expert_token &expert_token_service = my->_db.obtain_service<chain::dbs_expert_token>();
        chain::dbs_discipline& discipline_service = my->_db.obtain_service<chain::dbs_discipline>();
        auto expert_tokens = expert_token_service.get_expert_tokens_by_discipline_id(discipline_id);

        for (const chain::expert_token_object &expert_token : expert_tokens) {
            auto& discipline = discipline_service.get_discipline(expert_token.discipline_id);
            results.push_back(expert_token_api_obj(expert_token, fc::to_string(discipline.name)));
        }

        return results;
    });
}

fc::optional<expert_token_api_obj> database_api::get_common_token_by_account_name(const account_name_type account_name) const
{
    return my->_db.with_read_lock([&]() { return my->get_common_token_by_account_name(account_name); });
}

fc::optional<expert_token_api_obj> database_api_impl::get_common_token_by_account_name(const account_name_type account_name) const
{
    const auto& idx = _db.get_index<expert_token_index>().indices().get<by_account_and_discipline>();
    auto itr = idx.find(std::make_tuple(account_name, 0));
    if (itr != idx.end()) {
        chain::dbs_discipline& discipline_service = _db.obtain_service<chain::dbs_discipline>();
        auto& discipline = discipline_service.get_discipline(itr->discipline_id);
        return expert_token_api_obj(*itr, fc::to_string(discipline.name));
    }

    return {};
}

fc::optional<expert_token_api_obj> database_api::get_expert_token_by_account_name_and_discipline_id(const account_name_type account_name,
                                                                                                    const discipline_id_type discipline_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expert_token_by_account_name_and_discipline_id(account_name, discipline_id); });
}

fc::optional<expert_token_api_obj> database_api_impl::get_expert_token_by_account_name_and_discipline_id(const account_name_type account_name,
                                                                                                         const discipline_id_type discipline_id) const
{
    const auto& idx = _db.get_index<expert_token_index>().indices().get<by_account_and_discipline>();
    auto itr = idx.find(std::make_tuple(account_name, discipline_id));
    if (itr != idx.end()) {
        chain::dbs_discipline& discipline_service = _db.obtain_service<chain::dbs_discipline>();
        auto& discipline = discipline_service.get_discipline(itr->discipline_id);
        return expert_token_api_obj(*itr, fc::to_string(discipline.name));
    }

    return {};
}

vector<proposal_api_obj>
database_api::get_proposals_by_research_group_id(const research_group_id_type research_group_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<proposal_api_obj> results;

        chain::dbs_proposal& proposal_service = my->_db.obtain_service<chain::dbs_proposal>();
        auto proposals = proposal_service.get_proposals_by_research_group_id(research_group_id);

        for (const chain::proposal_object& proposal : proposals)
        {
            auto& votes = proposal_service.get_votes_for(proposal.id);
            vector<proposal_vote_api_obj> votes_for;
            for (const proposal_vote_object& vote : votes)
                votes_for.push_back(vote);
            results.push_back(proposal_api_obj(proposal, votes_for));
        }

        return results;
    });
}

fc::optional<proposal_api_obj> database_api::get_proposal(const proposal_id_type id) const
{
    return my->_db.with_read_lock([&]() { return my->get_proposal(id); });
}

fc::optional<proposal_api_obj> database_api_impl::get_proposal(const proposal_id_type id) const
{
    const auto& idx = _db.get_index<proposal_index>().indices().get<by_id>();
    auto itr = idx.find(id);

    if (itr != idx.end()) {
        chain::dbs_proposal &proposal_service = _db.obtain_service<chain::dbs_proposal>();
        vector<proposal_vote_api_obj> votes_for;
        auto& votes = proposal_service.get_votes_for(id);
        for (const proposal_vote_object& vote : votes)
            votes_for.push_back(vote);
        return proposal_api_obj(*itr, votes_for);
    }

    return {};
}

vector<research_group_token_api_obj>
database_api::get_research_group_tokens_by_account(const account_name_type account) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_group_token_api_obj> results;

        chain::dbs_research_group& research_group_service = my->_db.obtain_service<chain::dbs_research_group>();
        auto research_group_tokens = research_group_service.get_tokens_by_account(account);

        for (const chain::research_group_token_object& research_group_token : research_group_tokens)
        {
            results.push_back(research_group_token_api_obj(research_group_token));
        }

        return results;
    });
}

vector<research_group_token_api_obj>
database_api::get_research_group_tokens_by_research_group(const research_group_id_type& research_group_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_group_token_api_obj> results;

        chain::dbs_research_group& research_group_service = my->_db.obtain_service<chain::dbs_research_group>();
        auto research_group_tokens = research_group_service.get_research_group_tokens(research_group_id);

        for (const chain::research_group_token_object& research_group_token : research_group_tokens)
        {
            results.push_back(research_group_token_api_obj(research_group_token));
        }

        return results;
    });
}

fc::optional<research_group_token_api_obj> database_api::get_research_group_token_by_account_and_research_group_id(const account_name_type account,
                                                                                                                   const research_group_id_type& research_group_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_group_token_by_account_and_research_group_id(account, research_group_id); });
}

fc::optional<research_group_token_api_obj> database_api_impl::get_research_group_token_by_account_and_research_group_id(const account_name_type account,
                                                                                                                        const research_group_id_type& research_group_id) const
{
    const auto& idx = _db.get_index<research_group_token_index>().indices().get<by_owner>();
    auto itr = idx.find(std::make_tuple(account, research_group_id));

    if (itr != idx.end())
        return *itr;

    return {};
}

fc::optional<research_group_api_obj> database_api::get_research_group_by_id(const research_group_id_type research_group_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_group_by_id(research_group_id); });
}

fc::optional<research_group_api_obj> database_api_impl::get_research_group_by_id(const research_group_id_type& research_group_id) const
{
    const auto& idx = _db.get_index<research_group_index>().indices().get<by_id>();
    auto itr = idx.find(research_group_id);
    if (itr != idx.end()) {
        chain::dbs_research_group &research_group_service = _db.obtain_service<chain::dbs_research_group>();
        return research_group_service.get_research_group(research_group_id);
    }

    return {};
}

fc::optional<research_group_api_obj> database_api::get_research_group_by_permlink(const string& permlink) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_group_by_permlink(permlink); });
}

fc::optional<research_group_api_obj> database_api_impl::get_research_group_by_permlink(const string& permlink) const
{
    const auto& idx = _db.get_index<research_group_index>().indices().get<by_permlink>();
    auto itr = idx.find(permlink, fc::strcmp_less());
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<research_group_api_obj> database_api::get_all_research_groups(const bool& is_personal_need) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_group_api_obj> results;
        chain::dbs_research_group &research_group_service = my->_db.obtain_service<chain::dbs_research_group>();
        auto research_groups = research_group_service.get_all_research_groups(is_personal_need);

        for (const chain::research_group_object &research_group : research_groups) {
            results.push_back(research_group_api_obj(research_group));
        }

        return results;
    });
}

bool database_api::check_research_group_existence_by_permlink(const string& permlink) const
{
    const auto& idx = my->_db.get_index<research_group_index>().indices().get<by_permlink>();
    auto itr = idx.find(permlink, fc::strcmp_less());
    if (itr != idx.end())
        return true;
    else
        return false;
}


fc::optional<research_token_sale_api_obj>
database_api::get_research_token_sale_by_id(const research_token_sale_id_type research_token_sale_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_token_sale_by_id(research_token_sale_id); });
}

fc::optional<research_token_sale_api_obj>
database_api_impl::get_research_token_sale_by_id(const research_token_sale_id_type research_token_sale_id) const
{
    const auto& idx = _db.get_index<research_token_sale_index>().indices().get<by_id>();
    auto itr = idx.find(research_token_sale_id);
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<research_token_sale_api_obj> database_api::get_research_token_sale(const uint32_t& from = 0, uint32_t limit = 100) const
{
    return my->_db.with_read_lock([&]() {
    FC_ASSERT(limit <= MAX_LIMIT);
    const auto& research_token_sale_by_id = my->_db.get_index<research_token_sale_index>().indices().get<by_id>();
    vector<research_token_sale_api_obj> result;
    result.reserve(limit);

    for (auto itr = research_token_sale_by_id.lower_bound(from); limit-- && itr != research_token_sale_by_id.end(); ++itr)
    {
        result.push_back(research_token_sale_api_obj(*itr));
    }

    return result;
    });
}

vector<research_token_sale_api_obj>
database_api::get_research_token_sales_by_research_id(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_token_sale_api_obj> results;
        chain::dbs_research_token_sale& research_token_sale_service
                = my->_db.obtain_service<chain::dbs_research_token_sale>();

        auto research_token_sales = research_token_sale_service.get_by_research_id(research_id);

        for (const chain::research_token_sale_object& research_token_sale : research_token_sales)
        {
            results.push_back(research_token_sale);
        }

        return results;
    });
}

vector<research_token_sale_api_obj>
database_api::get_research_token_sales_by_research_id_and_status(const research_id_type& research_id, const research_token_sale_status status)
{
    return my->_db.with_read_lock([&]() {
        vector<research_token_sale_api_obj> results;
        chain::dbs_research_token_sale& research_token_sale_service
                = my->_db.obtain_service<chain::dbs_research_token_sale>();

        auto research_token_sales = research_token_sale_service.get_by_research_id_and_status(research_id, status);

        for (const chain::research_token_sale_object& research_token_sale : research_token_sales)
        {
            results.push_back(research_token_sale);
        }

        return results;
    });
}

fc::optional<research_token_sale_contribution_api_obj>
database_api::get_research_token_sale_contribution_by_id(const research_token_sale_contribution_id_type research_token_sale_contribution_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_token_sale_contribution_by_id(research_token_sale_contribution_id); });
}

fc::optional<research_token_sale_contribution_api_obj>
database_api_impl::get_research_token_sale_contribution_by_id(const research_token_sale_contribution_id_type research_token_sale_contribution_id) const
{
    const auto& idx = _db.get_index<research_token_sale_contribution_index>().indices().get<by_id>();
    auto itr = idx.find(research_token_sale_contribution_id);
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<research_token_sale_contribution_api_obj>
database_api::get_research_token_sale_contributions_by_research_token_sale_id(const research_token_sale_id_type research_token_sale_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_token_sale_contribution_api_obj> results;
        chain::dbs_research_token_sale& research_token_sale_service
            = my->_db.obtain_service<chain::dbs_research_token_sale>();

        auto research_token_sale_contributions = research_token_sale_service.get_research_token_sale_contributions_by_research_token_sale_id(research_token_sale_id);

        for (const chain::research_token_sale_contribution_object& research_token_sale_contribution : research_token_sale_contributions)
        {
            results.push_back(research_token_sale_contribution);
        }

        return results;
    });
}

fc::optional<research_token_sale_contribution_api_obj>
database_api::get_research_token_sale_contribution_by_contributor_and_research_token_sale_id(const account_name_type owner, const research_token_sale_id_type research_token_sale_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_token_sale_contribution_by_contributor_and_research_token_sale_id(owner, research_token_sale_id); });
}

fc::optional<research_token_sale_contribution_api_obj>
database_api_impl::get_research_token_sale_contribution_by_contributor_and_research_token_sale_id(const account_name_type owner, const research_token_sale_id_type research_token_sale_id) const
{
    const auto& idx = _db.get_index<research_token_sale_contribution_index>().indices().get<by_owner_and_research_token_sale_id>();
    auto itr = idx.find(std::make_tuple(owner, research_token_sale_id));
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<research_token_sale_contribution_api_obj>
database_api::get_research_token_sale_contributions_by_contributor(const account_name_type owner) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_token_sale_contribution_api_obj> results;
        chain::dbs_research_token_sale& research_token_sale_service = my->_db.obtain_service<chain::dbs_research_token_sale>();
        auto research_token_sale_contributions = research_token_sale_service.get_research_token_sale_contributions_by_contributor(owner);

        for (const chain::research_token_sale_contribution_object& research_token_sale_contribution : research_token_sale_contributions)
        {
            results.push_back(research_token_sale_contribution);
        }
        return results;
    });
}

vector<discipline_api_obj>
database_api::get_disciplines_by_research(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<discipline_api_obj> results;
        chain::dbs_research_discipline_relation& research_discipline_relation_service
                = my->_db.obtain_service<chain::dbs_research_discipline_relation>();

        auto research_discipline_relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research_id);

        for (const chain::research_discipline_relation_object& research_discipline_relation : research_discipline_relations)
        {
            results.push_back(*get_discipline(research_discipline_relation.discipline_id));
        }

        return results;
    });
}

fc::optional<research_group_invite_api_obj>
database_api::get_research_group_invite_by_id(const research_group_invite_id_type& research_group_invite_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_group_invite_by_id(research_group_invite_id); });
}

fc::optional<research_group_invite_api_obj>
database_api_impl::get_research_group_invite_by_id(const research_group_invite_id_type& research_group_invite_id) const
{
    const auto& idx = _db.get_index<research_group_invite_index>().indices().get<by_id>();
    auto itr = idx.find(research_group_invite_id);
    if (itr != idx.end())
        return *itr;

    return {};
}

fc::optional<research_group_invite_api_obj> database_api::get_research_group_invite_by_account_name_and_research_group_id(
    const account_name_type& account_name, const research_group_id_type& research_group_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_group_invite(account_name, research_group_id); });
}

fc::optional<research_group_invite_api_obj> database_api_impl::get_research_group_invite(const account_name_type& account_name, const research_group_id_type& research_group_id) const
{
    const auto& idx = _db.get_index<research_group_invite_index>().indices().get<by_account_and_research_group_id>();
    auto itr = idx.find(std::make_tuple(account_name, research_group_id));
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<research_group_invite_api_obj>
database_api::get_research_group_invites_by_account_name(const account_name_type& account_name) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_group_invite_api_obj> results;
        chain::dbs_research_group_invite& research_group_invite_service
            = my->_db.obtain_service<chain::dbs_research_group_invite>();

        auto research_group_invites = research_group_invite_service.get_research_group_invites_by_account_name(account_name);

        for (const chain::research_group_invite_object& research_group_invite : research_group_invites)
        {
            results.push_back(research_group_invite);
        }

        return results;
    });
}

vector<research_group_invite_api_obj> database_api::get_research_group_invites_by_research_group_id(const research_group_id_type& research_group_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_group_invite_api_obj> results;
        chain::dbs_research_group_invite& research_group_invite_service
            = my->_db.obtain_service<chain::dbs_research_group_invite>();

        auto research_group_invites = research_group_invite_service.get_research_group_invites_by_research_group_id(research_group_id);

        for (const chain::research_group_invite_object& research_group_invite : research_group_invites)
        {
            results.push_back(research_group_invite);
        }

        return results;
    });
}

vector<research_listing_api_obj> database_api::get_research_listing(const discipline_id_type& discipline_id,
                                                                    const uint64_t& from,
                                                                    const uint32_t& limit = 100) const
{
    return my->_db.with_read_lock([&]() {
        FC_ASSERT(limit <= MAX_LIMIT, "Limit of ${l} is greater than maximum allowed", ("l", limit));

        vector<research_listing_api_obj> results;
        results.reserve(limit);

      chain::dbs_expertise_contribution& expertise_contribution_service = my->_db.obtain_service<dbs_expertise_contribution>();

        auto researches = get_researches_by_discipline_id(from, limit, discipline_id);
        for (auto research : researches) {
            auto research_group_members = get_research_group_tokens_by_research_group(research.research_group_id);
            vector<account_name_type> group_members;
            for (auto member : research_group_members) {
                group_members.push_back(member.owner);
            }

            auto research_group = get_research_group_by_id(research.research_group_id);
            auto expertise_contributions = expertise_contribution_service.get_expertise_contributions_by_research(research.id);

            research_listing_api_obj listing_api_obj = research_listing_api_obj(research, *research_group, group_members, expertise_contributions.size());
            results.push_back(listing_api_obj);
        }

        return results;

    });
}

vector<research_listing_api_obj> database_api::get_all_researches_listing(const discipline_id_type& discipline_id,
                                                                          const uint32_t& limit = 0) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_listing_api_obj> results;
        chain::dbs_expertise_contribution& expertise_contributions_service = my->_db.obtain_service<dbs_expertise_contribution>();

        vector<research_api_obj> researches;


        if (discipline_id != 0) {
            const auto& rdr_idx = my->_db.get_index<research_discipline_relation_index>().indicies().get<by_discipline_id>();
            auto rdr_itr = rdr_idx.find(discipline_id);
            while (rdr_itr != rdr_idx.end())
            {
                auto& research = my->_db.get<research_object>(rdr_itr->research_id);
                vector<discipline_api_obj> disciplines = get_disciplines_by_research(research.id);
                auto research_group = get_research_group_by_id(research.research_group_id);
                researches.push_back(research_api_obj(research, disciplines, research_group->permlink));
                ++rdr_itr;
            }
        } else {
            const auto& rdr_idx = my->_db.get_index<research_index>().indicies().get<by_id>();
            auto rdr_itr = rdr_idx.lower_bound(0);
            while (rdr_itr != rdr_idx.end())
            {
                auto& research = *rdr_itr;
                vector<discipline_api_obj> disciplines = get_disciplines_by_research(research.id);
                auto research_group = get_research_group_by_id(research.research_group_id);
                researches.push_back(research_api_obj(research, disciplines, research_group->permlink));
                ++rdr_itr;
            }
        }

        for (auto research : researches) {
             auto research_group_members = get_research_group_tokens_by_research_group(research.research_group_id);
            vector<account_name_type> group_members;
            for (auto member : research_group_members) {
                group_members.push_back(member.owner);
            }
            
            auto research_group = get_research_group_by_id(research.research_group_id);
            auto expertise_contributions = expertise_contributions_service.get_expertise_contributions_by_research(research.id);

            research_listing_api_obj listing_api_obj = research_listing_api_obj(research, *research_group, group_members, expertise_contributions.size());

            if (limit != 0) {
                if (results.size() + 1 > limit) {
                    break;
                }
            }
            
            results.push_back(listing_api_obj);
        }

        return results;

    });
}

vector<expertise_contribution_object_api_obj> database_api::get_expertise_contributions_by_research(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_contributions_by_research(research_id); });
}

vector<expertise_contribution_object_api_obj> database_api_impl::get_expertise_contributions_by_research(const research_id_type& research_id) const
{
    vector<expertise_contribution_object_api_obj> results;
    chain::dbs_expertise_contribution& expertise_contributions_service = _db.obtain_service<chain::dbs_expertise_contribution>();

    auto expertise_contributions = expertise_contributions_service.get_expertise_contributions_by_research(research_id);
    for (const chain::expertise_contribution_object& contrib : expertise_contributions)
    {
        results.push_back(contrib);
    }

    return results;
}

vector<expertise_contribution_object_api_obj> database_api::get_expertise_contributions_by_research_and_discipline(
  const research_id_type& research_id,
  const discipline_id_type& discipline_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_contributions_by_research_and_discipline(research_id, discipline_id); });
}

vector<expertise_contribution_object_api_obj> database_api_impl::get_expertise_contributions_by_research_and_discipline(
  const research_id_type& research_id, 
  const discipline_id_type& discipline_id) const
{
    vector<expertise_contribution_object_api_obj> results;
    chain::dbs_expertise_contribution& expertise_contributions_service = _db.obtain_service<chain::dbs_expertise_contribution>();

    auto expertise_contributions = expertise_contributions_service.get_expertise_contributions_by_research_and_discipline(research_id, discipline_id);
    for (const chain::expertise_contribution_object& contrib : expertise_contributions)
    {
        results.push_back(contrib);
    }

    return results;
}


vector<expertise_contribution_object_api_obj> database_api::get_expertise_contributions_by_research_content(
  const research_content_id_type& research_content_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_contributions_by_research_content(research_content_id); });
}

vector<expertise_contribution_object_api_obj> database_api_impl::get_expertise_contributions_by_research_content(const research_content_id_type& research_content_id) const
{
    vector<expertise_contribution_object_api_obj> results;
    chain::dbs_expertise_contribution& expertise_contributions_service = _db.obtain_service<chain::dbs_expertise_contribution>();

    auto expertise_contributions = expertise_contributions_service.get_expertise_contributions_by_research_content(research_content_id);

    for (const chain::expertise_contribution_object& contrib : expertise_contributions)
    {
        results.push_back(contrib);
    }
    return results;
}


fc::optional<expertise_contribution_object_api_obj> database_api::get_expertise_contribution_by_research_content_and_discipline(
  const research_content_id_type& research_content_id,
  const discipline_id_type& discipline_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_contribution_by_research_content_and_discipline(research_content_id, discipline_id); });
}

fc::optional<expertise_contribution_object_api_obj> database_api_impl::get_expertise_contribution_by_research_content_and_discipline(
  const research_content_id_type& research_content_id,
  const discipline_id_type& discipline_id) const
{
    fc::optional<expertise_contribution_object_api_obj> result;
    const auto& idx = _db.get_index<expertise_contribution_index>()
      .indices()
      .get<by_research_content_and_discipline>();
      
    auto itr = idx.find(std::make_tuple(research_content_id, discipline_id));
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

fc::optional<review_api_obj> database_api::get_review_by_id(const review_id_type& review_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_review_by_id(review_id); });
}

fc::optional<review_api_obj> database_api_impl::get_review_by_id(const review_id_type& review_id) const
{
    const auto& idx = _db.get_index<review_index>().indices().get<by_id>();
    auto itr = idx.find(review_id);
    if (itr != idx.end()) {
        vector<discipline_api_obj> disciplines;

        for (const auto discipline_id : itr->disciplines) {
            auto discipline_ao = get_discipline(discipline_id);
            disciplines.push_back(*discipline_ao);
        }

        return review_api_obj(*itr, disciplines);
    }

    return {};
}

vector<review_api_obj> database_api::get_reviews_by_research(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<review_api_obj> results;
        chain::dbs_research_content& research_content_service = my->_db.obtain_service<chain::dbs_research_content>();


        auto contents = research_content_service.get_by_research_id(research_id);

        for (const chain::research_content_object& content : contents) {
            auto reviews = get_reviews_by_content(content.id);
            results.insert(results.end(), reviews.begin(), reviews.end());
        }
        return results;
    });
}

vector<review_api_obj> database_api::get_reviews_by_content(const research_content_id_type& research_content_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<review_api_obj> results;
        chain::dbs_review& review_service = my->_db.obtain_service<chain::dbs_review>();

        auto reviews = review_service.get_reviews_by_research_content(research_content_id);

        for (const chain::review_object& review : reviews)
        {
            vector<discipline_api_obj> disciplines;

            for (const auto discipline_id : review.disciplines) {
                auto discipline_ao = get_discipline(discipline_id);
                disciplines.push_back(*discipline_ao);
            }

            review_api_obj api_obj = review_api_obj(review, disciplines);
            results.push_back(api_obj);
        }

        return results;
    });
}

vector<review_api_obj> database_api::get_reviews_by_author(const account_name_type& author) const
{
    return my->_db.with_read_lock([&]() {
        vector<review_api_obj> results;
        chain::dbs_review& review_service = my->_db.obtain_service<chain::dbs_review>();

        auto reviews = review_service.get_author_reviews(author);

        for (const chain::review_object& review : reviews)
        {
            vector<discipline_api_obj> disciplines;

            for (const auto discipline_id : review.disciplines) {
                auto discipline_ao = get_discipline(discipline_id);
                disciplines.push_back(*discipline_ao);
            }

            review_api_obj api_obj = review_api_obj(review, disciplines);
            results.push_back(api_obj);
        }

        return results;
    });
}

vector<grant_application_review_api_obj> database_api::get_reviews_by_grant_application(const grant_application_id_type& grant_application_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<grant_application_review_api_obj> results;
        chain::dbs_grant_application_review& grant_application_review_service = my->_db.obtain_service<chain::dbs_grant_application_review>();
        auto reviews = grant_application_review_service.get_grant_application_reviews(grant_application_id);

        for (const chain::grant_application_review_object& review : reviews)
        {
            vector<discipline_api_obj> disciplines;

            for (const auto discipline_id : review.disciplines)
            {
                auto discipline_ao = get_discipline(discipline_id);
                disciplines.push_back(*discipline_ao);
            }

            grant_application_review_api_obj api_obj = grant_application_review_api_obj(review, disciplines);
            results.push_back(api_obj);
        }

        return results;
    });
}

fc::optional<research_token_api_obj> database_api::get_research_token_by_id(const research_token_id_type& research_token_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_token_by_id(research_token_id); });
}

fc::optional<research_token_api_obj> database_api_impl::get_research_token_by_id(const research_token_id_type& research_token_id) const
{
    const auto& idx = _db.get_index<research_token_index>().indices().get<by_id>();
    auto itr = idx.find(research_token_id);
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<research_token_api_obj> database_api::get_research_tokens_by_account_name(const account_name_type &account_name) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_token_api_obj> results;
        chain::dbs_research_token& research_token_service
                = my->_db.obtain_service<chain::dbs_research_token>();

        auto research_tokens = research_token_service.get_by_owner(account_name);

        for (const chain::research_token_object& research_token : research_tokens)
            results.push_back(research_token);

        return results;
    });
}

vector<research_token_api_obj> database_api::get_research_tokens_by_research_id(const research_id_type &research_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_token_api_obj> results;
        chain::dbs_research_token& research_token_service
                = my->_db.obtain_service<chain::dbs_research_token>();

        auto research_tokens = research_token_service.get_by_research(research_id);

        for (const chain::research_token_object& research_token : research_tokens)
            results.push_back(research_token);

        return results;
    });
}

fc::optional<research_token_api_obj> database_api::get_research_token_by_account_name_and_research_id(const account_name_type &account_name,
                                                                                                      const research_id_type &research_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_token_by_account_name_and_research_id(account_name, research_id); });
}

fc::optional<research_token_api_obj> database_api_impl::get_research_token_by_account_name_and_research_id(const account_name_type &account_name,
                                                                                                           const research_id_type &research_id) const
{
    const auto& idx = _db.get_index<research_token_index>().indices().get<by_account_name_and_research_id>();
    auto itr = idx.find(std::make_tuple(account_name, research_id));
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<review_vote_api_obj> database_api::get_review_votes_by_voter(const account_name_type &voter) const
{
    return my->_db.with_read_lock([&]() {
        vector<review_vote_api_obj> results;
        chain::dbs_review_vote& review_votes_service
                = my->_db.obtain_service<chain::dbs_review_vote>();

        auto review_votes = review_votes_service.get_review_votes_by_voter(voter);

        for (const chain::review_vote_object& review_vote : review_votes)
            results.push_back(review_vote);

        return results;
    });
}

vector<review_vote_api_obj> database_api::get_review_votes_by_review_id(const review_id_type &review_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<review_vote_api_obj> results;
        chain::dbs_review_vote& review_votes_service
                = my->_db.obtain_service<chain::dbs_review_vote>();

        auto review_votes = review_votes_service.get_review_votes(review_id);

        for (const chain::review_vote_object& review_vote : review_votes)
            results.push_back(review_vote);

        return results;
    });
}

fc::optional<expertise_allocation_proposal_api_obj> database_api::get_expertise_allocation_proposal_by_id(const expertise_allocation_proposal_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_allocation_proposal_by_id(id); });
}

fc::optional<expertise_allocation_proposal_api_obj> database_api_impl::get_expertise_allocation_proposal_by_id(const expertise_allocation_proposal_id_type& id) const
{
    const auto& idx = _db.get_index<expertise_allocation_proposal_index>().indices().get<by_id>();
    auto itr = idx.find(id);
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<expertise_allocation_proposal_api_obj> database_api::get_expertise_allocation_proposals_by_claimer(const account_name_type &claimer) const
{
    return my->_db.with_read_lock([&]() {
        vector<expertise_allocation_proposal_api_obj> results;
        chain::dbs_expertise_allocation_proposal& expertise_allocation_proposal_service
                = my->_db.obtain_service<chain::dbs_expertise_allocation_proposal>();

        auto proposals = expertise_allocation_proposal_service.get_by_claimer(claimer);

        for (const chain::expertise_allocation_proposal_object& proposal : proposals)
            results.push_back(proposal);

        return results;
    });
}

fc::optional<expertise_allocation_proposal_api_obj> database_api::get_expertise_allocation_proposals_by_claimer_and_discipline(const account_name_type& claimer,
                                                                                                                               const discipline_id_type& discipline_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_allocation_proposals_by_claimer_and_discipline(claimer, discipline_id); });
}

fc::optional<expertise_allocation_proposal_api_obj> database_api_impl::get_expertise_allocation_proposals_by_claimer_and_discipline(const account_name_type& claimer,
                                                                                                                                    const discipline_id_type& discipline_id) const
{
    const auto& idx = _db.get_index<expertise_allocation_proposal_index>().indices().get<by_claimer_and_discipline>();
    auto itr = idx.find(std::make_tuple(claimer, discipline_id));
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<expertise_allocation_proposal_api_obj> database_api::get_expertise_allocation_proposals_by_discipline(const discipline_id_type& discipline_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<expertise_allocation_proposal_api_obj> results;
        chain::dbs_expertise_allocation_proposal& expertise_allocation_proposal_service
                = my->_db.obtain_service<chain::dbs_expertise_allocation_proposal>();

        auto proposals = expertise_allocation_proposal_service.get_by_discipline_id(discipline_id);

        for (const chain::expertise_allocation_proposal_object& proposal : proposals)
            results.push_back(proposal);

        return results;
    });
}

fc::optional<expertise_allocation_proposal_vote_api_obj> database_api::get_expertise_allocation_proposal_vote_by_id(const expertise_allocation_proposal_vote_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_allocation_proposal_vote_by_id(id); });
}

fc::optional<expertise_allocation_proposal_vote_api_obj> database_api_impl::get_expertise_allocation_proposal_vote_by_id(const expertise_allocation_proposal_vote_id_type& id) const
{
    const auto& idx = _db.get_index<expertise_allocation_proposal_vote_index>().indices().get<by_id>();
    auto itr = idx.find(id);
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<expertise_allocation_proposal_vote_api_obj> database_api::get_expertise_allocation_proposal_votes_by_expertise_allocation_proposal_id
                                                                                        (const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<expertise_allocation_proposal_vote_api_obj> results;
        chain::dbs_expertise_allocation_proposal& expertise_allocation_proposal_vote_service
                = my->_db.obtain_service<chain::dbs_expertise_allocation_proposal>();

        auto proposal_votes = expertise_allocation_proposal_vote_service.get_votes_by_expertise_allocation_proposal_id(expertise_allocation_proposal_id);

        for (const chain::expertise_allocation_proposal_vote_object& proposal_vote : proposal_votes)
            results.push_back(proposal_vote);

        return results;
    });
}

fc::optional<expertise_allocation_proposal_vote_api_obj>
database_api::get_expertise_allocation_proposal_vote_by_voter_and_expertise_allocation_proposal_id(const account_name_type& voter,
                                                                                                   const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_allocation_proposal_vote_by_voter_and_expertise_allocation_proposal_id(voter, expertise_allocation_proposal_id); });
}

fc::optional<expertise_allocation_proposal_vote_api_obj>
database_api_impl::get_expertise_allocation_proposal_vote_by_voter_and_expertise_allocation_proposal_id(const account_name_type& voter,
                                                                                                        const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id) const
{
    const auto& idx = _db.get_index<expertise_allocation_proposal_vote_index>().indices().get<by_voter_and_expertise_allocation_proposal_id>();
    auto itr = idx.find(std::make_tuple(voter, expertise_allocation_proposal_id));
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<expertise_allocation_proposal_vote_api_obj> database_api::get_expertise_allocation_proposal_votes_by_voter_and_discipline_id(const account_name_type& voter,
                                                                                                                                   const discipline_id_type& discipline_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<expertise_allocation_proposal_vote_api_obj> results;
        chain::dbs_expertise_allocation_proposal& expertise_allocation_proposal_vote_service
                = my->_db.obtain_service<chain::dbs_expertise_allocation_proposal>();

        auto proposal_votes = expertise_allocation_proposal_vote_service.get_votes_by_voter_and_discipline_id(voter, discipline_id);

        for (const chain::expertise_allocation_proposal_vote_object& proposal_vote : proposal_votes)
            results.push_back(proposal_vote);

        return results;
    });
}

vector<expertise_allocation_proposal_vote_api_obj> database_api::get_expertise_allocation_proposal_votes_by_voter(const account_name_type& voter) const
{
    return my->_db.with_read_lock([&]() {
        vector<expertise_allocation_proposal_vote_api_obj> results;
        chain::dbs_expertise_allocation_proposal& expertise_allocation_proposal_vote_service
                = my->_db.obtain_service<chain::dbs_expertise_allocation_proposal>();

        auto proposal_votes = expertise_allocation_proposal_vote_service.get_votes_by_voter(voter);

        for (const chain::expertise_allocation_proposal_vote_object& proposal_vote : proposal_votes)
            results.push_back(proposal_vote);

        return results;
    });
}

fc::optional<vesting_balance_api_obj> database_api::get_vesting_balance_by_id(const vesting_balance_id_type& vesting_balance_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_vesting_balance_by_id(vesting_balance_id); });
}

fc::optional<vesting_balance_api_obj> database_api_impl::get_vesting_balance_by_id(const vesting_balance_id_type& vesting_balance_id) const
{
    const auto& idx = _db.get_index<vesting_balance_index>().indices().get<by_id>();
    auto itr = idx.find(vesting_balance_id);
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<vesting_balance_api_obj>
database_api::get_vesting_balance_by_owner(const account_name_type &owner) const
{
    return my->_db.with_read_lock([&]() {
        vector<vesting_balance_api_obj> results;
        chain::dbs_vesting_balance& vesting_balance_service
            = my->_db.obtain_service<chain::dbs_vesting_balance>();

        auto total_vesting_balance = vesting_balance_service.get_by_owner(owner);

        for (const chain::vesting_balance_object& vesting_balance : total_vesting_balance)
        {
            results.push_back(vesting_balance);
        }

        return results;
    });
}

fc::optional<offer_research_tokens_api_obj> database_api::get_offer(const offer_research_tokens_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_offer(id); });
}

fc::optional<offer_research_tokens_api_obj> database_api_impl::get_offer(const offer_research_tokens_id_type& id) const
{
    const auto& idx = _db.get_index<offer_research_tokens_index>().indices().get<by_id>();
    auto itr = idx.find(id);
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<offer_research_tokens_api_obj> database_api::get_offers_by_receiver(const account_name_type& receiver) const
{
    return my->_db.with_read_lock([&]() {
        vector<offer_research_tokens_api_obj> results;
        chain::dbs_offer_research_tokens& offer_service
                = my->_db.obtain_service<chain::dbs_offer_research_tokens>();

        auto offers = offer_service.get_offers_by_receiver(receiver);

        for (const chain::offer_research_tokens_object& offer : offers)
        {
            results.push_back(offer);
        }

        return results;
    });
}

fc::optional<offer_research_tokens_api_obj> database_api::get_offer_by_receiver_and_research_id(const account_name_type& receiver,
                                                                                                const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_offer_by_receiver_and_research_id(receiver, research_id); });
}

fc::optional<offer_research_tokens_api_obj> database_api_impl::get_offer_by_receiver_and_research_id(const account_name_type& receiver,
                                                                                                     const research_id_type& research_id) const
{
    const auto& idx = _db.get_index<offer_research_tokens_index>().indices().get<by_receiver_and_research_id>();
    auto itr = idx.find(std::make_tuple(receiver, research_id));
    if (itr != idx.end())
        return fc::optional<offer_research_tokens_api_obj>(*itr);

    return {};
}

vector<offer_research_tokens_api_obj> database_api::get_offers_by_research_id(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<offer_research_tokens_api_obj> results;
        chain::dbs_offer_research_tokens& offer_service
                = my->_db.obtain_service<chain::dbs_offer_research_tokens>();

        auto offers = offer_service.get_offers_by_research_id(research_id);

        for (const chain::offer_research_tokens_object& offer : offers)
        {
            results.push_back(offer);
        }

        return results;
    });
}

fc::optional<grant_api_obj> database_api::get_grant_with_announced_application_window(const grant_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_grant_with_announced_application_window(id); });
}

fc::optional<grant_api_obj> database_api_impl::get_grant_with_announced_application_window(const grant_id_type& id) const
{
    fc::optional<grant_api_obj> result;
    chain::dbs_grant& grant_service = _db.obtain_service<chain::dbs_grant>();
    const auto& opt = grant_service.get_grant_with_announced_application_window_if_exists(id);

    if (opt.valid())
    {
        result = grant_api_obj(*opt);
    }

    return result;
}

vector<grant_api_obj> database_api::get_grants_with_announced_application_window_by_grantor(const string& grantor) const
{
    return my->_db.with_read_lock([&]() { return my->get_grants_with_announced_application_window_by_grantor(grantor); });
}

vector<grant_api_obj> database_api_impl::get_grants_with_announced_application_window_by_grantor(const string& grantor) const
{
    vector<grant_api_obj> results;
    chain::dbs_grant& grant_service = _db.obtain_service<chain::dbs_grant>();

    auto grants = grant_service.get_grants_with_announced_application_window_by_grantor(grantor);
    for (const chain::grant_object& grant: grants)
    {
        results.push_back(grant_api_obj(grant));
    }

    return results;
}

fc::optional<grant_application_api_obj> database_api::get_grant_application(const grant_application_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_grant_application(id); });
}

fc::optional<grant_application_api_obj> database_api_impl::get_grant_application(const grant_application_id_type& id) const
{
    const auto& idx = _db.get_index<grant_application_index>().indices().get<by_id>();
    auto itr = idx.find(id);
    if (itr != idx.end())
        return *itr;
    else
        return {};
}

vector<grant_application_api_obj> database_api::get_grant_applications_by_grant(const grant_id_type& grant_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<grant_application_api_obj> results;
        chain::dbs_grant_application& grant_application_service = my->_db.obtain_service<chain::dbs_grant_application>();
        auto grant_applications = grant_application_service.get_grant_applications_by_grant(grant_id);

        for (const chain::grant_application_object& grant_application : grant_applications)
            results.push_back(grant_application);

        return results;
    });
}

vector<grant_application_api_obj>
database_api::get_grant_applications_by_research_id(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<grant_application_api_obj> results;
        chain::dbs_grant_application& grant_application_service = my->_db.obtain_service<chain::dbs_grant_application>();

        auto grant_applications = grant_application_service.get_grant_applications_by_research_id(research_id);

        for (const chain::grant_application_object& grant_application : grant_applications)
            results.push_back(grant_application);

        return results;
    });
}

fc::optional<funding_opportunity_api_obj> database_api::get_funding_opportunity_announcement(const funding_opportunity_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_funding_opportunity_announcement(id); });
}

fc::optional<funding_opportunity_api_obj> database_api_impl::get_funding_opportunity_announcement(const funding_opportunity_id_type& id) const
{
    fc::optional<funding_opportunity_api_obj> result;

    chain::dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<chain::dbs_funding_opportunity>();
    const auto& opt = funding_opportunity_service.get_funding_opportunity_announcement_if_exists(id);

    if (opt.valid())
    {
        result = funding_opportunity_api_obj(*opt);
    }

    return result;
}

fc::optional<funding_opportunity_api_obj> database_api::get_funding_opportunity_announcement_by_number(const string& funding_opportunity_number) const
{
    return my->_db.with_read_lock([&]() { return my->get_funding_opportunity_announcement_by_number(funding_opportunity_number); });
}

fc::optional<funding_opportunity_api_obj> database_api_impl::get_funding_opportunity_announcement_by_number(const string& funding_opportunity_number) const
{
    fc::optional<funding_opportunity_api_obj> result;
    chain::dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<chain::dbs_funding_opportunity>();
    const auto& opt = funding_opportunity_service.get_funding_opportunity_announcement_if_exists(funding_opportunity_number);
    
    if (opt.valid())
    {
        result = funding_opportunity_api_obj(*opt);
    }

    return result;
}

vector<funding_opportunity_api_obj> database_api::get_funding_opportunity_announcements_by_organization(const research_group_id_type& research_group_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_funding_opportunity_announcements_by_organization(research_group_id); });
}

vector<funding_opportunity_api_obj> database_api_impl::get_funding_opportunity_announcements_by_organization(const research_group_id_type& research_group_id) const
{
    vector<funding_opportunity_api_obj> results;
    chain::dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<chain::dbs_funding_opportunity>();
    auto funding_opportunities = funding_opportunity_service.get_funding_opportunity_announcements_by_organization(research_group_id);

    for (const chain::funding_opportunity_object& funding_opportunity : funding_opportunities)
    {
        results.push_back(funding_opportunity);
    }
    return results;
}


// TODO: add pagination
vector<funding_opportunity_api_obj> database_api::get_funding_opportunity_announcements_listing(const uint16_t& page = 1, const uint16_t& limit = 100) const
{
    return my->_db.with_read_lock([&]() { return my->get_funding_opportunity_announcements_listing(page, limit); });
}

vector<funding_opportunity_api_obj> database_api_impl::get_funding_opportunity_announcements_listing(const uint16_t& page, const uint16_t& limit) const
{
    vector<funding_opportunity_api_obj> results;
    chain::dbs_funding_opportunity& funding_opportunity_service = _db.obtain_service<chain::dbs_funding_opportunity>();
    
    auto funding_opportunities = funding_opportunity_service.get_funding_opportunity_announcements_listing(page, limit);
    for (auto& funding_opportunity : funding_opportunities) 
    {
        results.push_back(funding_opportunity_api_obj(funding_opportunity));
    }
    return results;
}

std::map<discipline_id_type, share_type> database_api::calculate_research_eci(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_research& research_service = my->_db.obtain_service<chain::dbs_research>();
        return research_service.get_eci_evaluation(research_id);
    });
}

std::map<discipline_id_type, share_type> database_api::calculate_research_content_eci(const research_content_id_type& research_content_id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_research_content& research_content_service = my->_db.obtain_service<chain::dbs_research_content>();
        return research_content_service.get_eci_evaluation(research_content_id);
    });
}

std::map<discipline_id_type, share_type> database_api::calculate_review_weight(const review_id_type& review_id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_review& review_service = my->_db.obtain_service<chain::dbs_review>();
        return review_service.get_eci_weight(review_id);
    });
}

fc::optional<asset_api_obj> database_api::get_asset(const asset_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_asset(id); });
}

fc::optional<asset_api_obj> database_api_impl::get_asset(const asset_id_type& id) const
{
    const auto& idx = _db.get_index<asset_index>().indices().get<by_id>();
    auto itr = idx.find(id);
    if (itr != idx.end())
        return *itr;

    return {};
}

fc::optional<asset_api_obj> database_api::get_asset_by_string_symbol(const std::string& string_symbol) const
{
    return my->_db.with_read_lock([&]() { return my->get_asset_by_string_symbol(string_symbol); });
}

fc::optional<asset_api_obj> database_api_impl::get_asset_by_string_symbol(const std::string& string_symbol) const
{
    const auto& idx = _db.get_index<asset_index>().indices().get<by_string_symbol>();
    auto itr = idx.find(string_symbol, fc::strcmp_less());
    if (itr != idx.end())
        return *itr;

    return {};
}

fc::optional<account_balance_api_obj> database_api::get_account_balance(const account_balance_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_account_balance(id); });
}

fc::optional<account_balance_api_obj> database_api_impl::get_account_balance(const account_balance_id_type& id) const
{
    const auto& idx = _db.get_index<account_balance_index>().indicies().get<by_id>();
    auto itr = idx.find(id);
    if (itr != idx.end())
        return *itr;

    return {};
}

vector<account_balance_api_obj> database_api::get_account_balances_by_owner(const account_name_type& owner) const
{
    return my->_db.with_read_lock([&]() {
        vector<account_balance_api_obj> results;
        chain::dbs_account_balance& account_balance_service
                = my->_db.obtain_service<chain::dbs_account_balance>();

        auto account_balances = account_balance_service.get_by_owner(owner);

        for (const chain::account_balance_object& account_balance : account_balances)
            results.push_back(account_balance);

        return results;
    });
}

fc::optional<account_balance_api_obj> database_api::get_account_balance_by_owner_and_asset_symbol(const account_name_type& owner, const string& symbol) const
{
    return my->_db.with_read_lock([&]() { return my->get_account_balance_by_owner_and_asset_symbol(owner, symbol); });
}

fc::optional<account_balance_api_obj> database_api_impl::get_account_balance_by_owner_and_asset_symbol(const account_name_type& owner, const string& symbol) const
{
    const auto& idx = _db.get_index<account_balance_index>().indicies().get<by_owner_and_asset_string_symbol>();
    auto itr = idx.find(std::make_tuple(owner, symbol));
    if (itr != idx.end())
        return *itr;

    return {};
}

fc::optional<research_group_organization_contract_api_obj> database_api::get_organizational_contract(const research_group_organization_contract_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_organizational_contract(id); });
}

fc::optional<research_group_organization_contract_api_obj> database_api_impl::get_organizational_contract(const research_group_organization_contract_id_type& id) const
{
    fc::optional<research_group_organization_contract_api_obj> result;

    const auto& idx = _db
      .get_index<research_group_organization_contract_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = research_group_organization_contract_api_obj(*itr);
    }

    return result;
}


vector<research_group_organization_contract_api_obj> database_api::get_organizational_contracts_by_organization(const research_group_id_type& organization_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_organizational_contracts_by_organization(organization_id); });
}

vector<research_group_organization_contract_api_obj> database_api_impl::get_organizational_contracts_by_organization(const research_group_id_type& organization_id) const
{
    vector<research_group_organization_contract_api_obj> results;

    const auto& idx = _db
      .get_index<research_group_organization_contract_index>()
      .indicies()
      .get<contracts_by_organization>();

    auto itr = idx.find(organization_id);
    const auto itr_end = idx.end();
    while (itr != itr_end)
    {
        results.push_back(research_group_organization_contract_api_obj(*itr));
        ++itr;
    }

    return results;
}


vector<research_group_organization_contract_api_obj> database_api::get_organizational_contracts_by_research_group(const research_group_id_type& research_group_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_organizational_contracts_by_research_group(research_group_id); });
}

vector<research_group_organization_contract_api_obj> database_api_impl::get_organizational_contracts_by_research_group(const research_group_id_type& research_group_id) const
{
    vector<research_group_organization_contract_api_obj> results;

    const auto& itr_pair = _db
      .get_index<research_group_organization_contract_index>()
      .indicies()
      .get<contracts_by_research_group>()
      .equal_range(research_group_id);

    auto itr = itr_pair.first;
    const auto itr_end = itr_pair.second;

    while (itr != itr_end)
    {
        results.push_back(research_group_organization_contract_api_obj(*itr));
        ++itr;
    }

    return results;
}

fc::optional<research_group_organization_contract_api_obj> database_api::get_organizational_contract_by_organization_and_research_group_and_type(const research_group_id_type& organization_id, const research_group_id_type& research_group_id, const uint16_t& type) const
{
    return my->_db.with_read_lock([&]() {
        return my->get_organizational_contract_by_organization_and_research_group_and_type(
          organization_id,
          research_group_id, 
          type);
    });
}

fc::optional<research_group_organization_contract_api_obj> database_api_impl::get_organizational_contract_by_organization_and_research_group_and_type(const research_group_id_type& organization_id, const research_group_id_type& research_group_id, const uint16_t& type) const
{
    fc::optional<research_group_organization_contract_api_obj> result;

    const auto& idx = _db
      .get_index<research_group_organization_contract_index>()
      .indicies()
      .get<contract_by_organization_and_research_group_and_type>();

    auto itr = idx.find(std::make_tuple(organization_id, research_group_id, type));
    if (itr != idx.end())
    {
        result = research_group_organization_contract_api_obj(*itr);
    }

    return result;
}

fc::optional<discipline_supply_api_obj> database_api::get_discipline_supply(const discipline_supply_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_discipline_supply(id); });
}

fc::optional<discipline_supply_api_obj> database_api_impl::get_discipline_supply(const discipline_supply_id_type& id) const
{
    fc::optional<discipline_supply_api_obj> result;

    const auto& idx = _db
      .get_index<discipline_supply_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = discipline_supply_api_obj(*itr);
    }

    return result;
}

fc::optional<award_api_obj> database_api::get_award(const string& award_number) const
{
    return my->_db.with_read_lock([&]() { return my->get_award(award_number); });
}

fc::optional<award_api_obj> database_api_impl::get_award(const string& award_number) const
{
    chain::dbs_award& awards_service = _db.obtain_service<chain::dbs_award>();
    
    fc::optional<award_api_obj> result;
    const auto& opt = awards_service.get_award_if_exists(award_number);

    if (opt.valid())
    {   
        vector<award_recipient_api_obj> awardees_list;
        auto awardees = awards_service.get_award_recipients_by_award(award_number);
        for (auto& wrap : awardees)
        {
            const auto& awardee = wrap.get();
            awardees_list.push_back(awardee);
        }
        result = award_api_obj(*opt, awardees_list);
    }

    return result;
}

vector<award_api_obj> database_api::get_awards_by_funding_opportunity(const string& funding_opportunity_number) const
{
    return my->_db.with_read_lock([&]() { return my->get_awards_by_funding_opportunity(funding_opportunity_number); });
}

vector<award_api_obj> database_api_impl::get_awards_by_funding_opportunity(const string& funding_opportunity_number) const
{
    chain::dbs_award& awards_service = _db.obtain_service<chain::dbs_award>();
    
    vector<award_api_obj> results;
    auto awards = awards_service.get_awards_by_funding_opportunity(funding_opportunity_number);
    for (auto& wrap : awards)
    {
        const auto& award = wrap.get();
        vector<award_recipient_api_obj> awardees_list;
        auto awardees = awards_service.get_award_recipients_by_award(fc::to_string(award.award_number));
        for (auto& awardee_wrap : awardees)
        {
            const auto& awardee = awardee_wrap.get();
            awardees_list.push_back(awardee);
        }
        results.push_back(award_api_obj(award, awardees_list));
    }

    return results;
}

fc::optional<award_recipient_api_obj> database_api::get_award_recipient(const award_recipient_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_award_recipient(id); });
}

fc::optional<award_recipient_api_obj> database_api_impl::get_award_recipient(const award_recipient_id_type& id) const
{
    chain::dbs_award& awards_service = _db.obtain_service<chain::dbs_award>();

    fc::optional<award_recipient_api_obj> result;
    const auto& opt = awards_service.get_award_recipient_if_exists(id);

    if (opt.valid())
    {
        result = award_recipient_api_obj(*opt);
    }

    return result;
}

vector<award_recipient_api_obj> database_api::get_award_recipients_by_award(const string& award_number) const
{
    return my->_db.with_read_lock([&]() { return my->get_award_recipients_by_award(award_number); });
}

vector<award_recipient_api_obj> database_api_impl::get_award_recipients_by_award(const string& award_number) const
{
    chain::dbs_award& awards_service = _db.obtain_service<chain::dbs_award>();

    vector<award_recipient_api_obj> results;
    auto awardees = awards_service.get_award_recipients_by_award(award_number);
    for (auto& wrap : awardees)
    {
        const auto& awardee = wrap.get();
        results.push_back(awardee);
    }

    return results;
}

vector<award_recipient_api_obj> database_api::get_award_recipients_by_account(const account_name_type& account) const
{
    return my->_db.with_read_lock([&]() { return my->get_award_recipients_by_account(account); });
}

vector<award_recipient_api_obj> database_api_impl::get_award_recipients_by_account(const account_name_type& account) const
{
    chain::dbs_award& awards_service = _db.obtain_service<chain::dbs_award>();

    vector<award_recipient_api_obj> results;
    auto awardees = awards_service.get_award_recipients_by_account(account);
    for (auto& wrap : awardees)
    {
        const auto& awardee = wrap.get();
        results.push_back(awardee);
    }

    return results;
}

vector<award_recipient_api_obj> database_api::get_award_recipients_by_funding_opportunity(const string& number) const
{
    return my->_db.with_read_lock([&]() { return my->get_award_recipients_by_funding_opportunity(number); });
}

vector<award_recipient_api_obj> database_api_impl::get_award_recipients_by_funding_opportunity(const string& number) const
{
    chain::dbs_award& awards_service = _db.obtain_service<chain::dbs_award>();
    vector<award_recipient_api_obj> results;

    auto awardees = awards_service.get_award_recipients_by_funding_opportunity(number);
    for (auto& wrap : awardees)
    {
        const auto& awardee = wrap.get();
        results.push_back(awardee);
    }
    return results;
}

fc::optional<award_withdrawal_request_api_obj> database_api::get_award_withdrawal_request(const string& award_number, const string& payment_number) const
{
    return my->_db.with_read_lock([&]() {
        return my->get_award_withdrawal_request(award_number, payment_number);
    });
}

fc::optional<award_withdrawal_request_api_obj> database_api_impl::get_award_withdrawal_request(const string& award_number, const string& payment_number) const
{
    chain::dbs_award& awards_service = _db.obtain_service<chain::dbs_award>();

    fc::optional<award_withdrawal_request_api_obj> result;
    const auto& opt = awards_service.get_award_withdrawal_request_if_exists(award_number, payment_number);

    if (opt.valid())
    {
        result = award_withdrawal_request_api_obj(*opt);
    }

    return result;
}

vector<award_withdrawal_request_api_obj> database_api::get_award_withdrawal_requests_by_award(const string& award_number) const
{
    return my->_db.with_read_lock([&]() {
        return my->get_award_withdrawal_requests_by_award(award_number);
    });
}

vector<award_withdrawal_request_api_obj> database_api_impl::get_award_withdrawal_requests_by_award(const string& award_number) const
{
    chain::dbs_award& awards_service = _db.obtain_service<chain::dbs_award>();

    vector<award_withdrawal_request_api_obj> results;
    auto withdrawal_requests = awards_service.get_award_withdrawal_requests_by_award(award_number);
    for (auto& wrap : withdrawal_requests)
    {
        const auto& withdrawal_request = wrap.get();
        results.push_back(withdrawal_request);
    }

    return results;
}

vector<award_withdrawal_request_api_obj> database_api::get_award_withdrawal_requests_by_award_and_subaward(const string& award_number, const string& subaward_number) const
{
    return my->_db.with_read_lock([&]() {
        return my->get_award_withdrawal_requests_by_award_and_subaward(award_number, subaward_number);
    });
}

vector<award_withdrawal_request_api_obj> database_api_impl::get_award_withdrawal_requests_by_award_and_subaward(const string& award_number, const string& subaward_number) const
{
    chain::dbs_award& awards_service = _db.obtain_service<chain::dbs_award>();

    vector<award_withdrawal_request_api_obj> results;
    auto withdrawal_requests = awards_service.get_award_withdrawal_requests_by_award_and_subaward(award_number, subaward_number);
    for (auto& wrap : withdrawal_requests)
    {
        const auto& withdrawal_request = wrap.get();
        results.push_back(withdrawal_request);
    }

    return results;
}

vector<award_withdrawal_request_api_obj> database_api::get_award_withdrawal_requests_by_award_and_status(const string& award_number, const uint16_t& status) const
{
    return my->_db.with_read_lock([&]() {
        const award_withdrawal_request_status st = static_cast<award_withdrawal_request_status>(status);
        return my->get_award_withdrawal_requests_by_award_and_status(award_number, st);
    });
}

vector<award_withdrawal_request_api_obj> database_api_impl::get_award_withdrawal_requests_by_award_and_status(const string& award_number, const award_withdrawal_request_status& status) const
{
    chain::dbs_award& awards_service = _db.obtain_service<chain::dbs_award>();

    vector<award_withdrawal_request_api_obj> results;
    auto withdrawal_requests = awards_service.get_award_withdrawal_requests_by_award_and_status(award_number, status);
    for (auto& wrap : withdrawal_requests)
    {
        const auto& withdrawal_request = wrap.get();
        results.push_back(withdrawal_request);
    }

    return results;
}

} // namespace app
} // namespace deip
