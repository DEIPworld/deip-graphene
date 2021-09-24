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
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/services/dbs_expertise_contribution.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_expertise_allocation_proposal.hpp>
#include <deip/chain/services/dbs_vesting_balance.hpp>
#include <deip/chain/services/dbs_grant_application.hpp>
#include <deip/chain/services/dbs_funding_opportunity.hpp>
#include <deip/chain/services/dbs_nda_contract.hpp>
#include <deip/chain/services/dbs_nda_contract_requests.hpp>
#include <deip/chain/services/dbs_research_license.hpp>
#include <deip/chain/services/dbs_contract_agreement.hpp>

#define GET_REQUIRED_FEES_MAX_RECURSION 4

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

    // Accounts
    vector<optional<account_api_obj>> get_accounts(const set<string>& names) const;
    vector<account_api_obj> lookup_accounts(const string& lower_bound_name, uint32_t limit) const;
    vector<account_api_obj> get_accounts_by_expert_discipline(const discipline_id_type& discipline_id, uint32_t from, uint32_t limit) const;
    uint64_t get_account_count() const;

    // discipline_supplies
    vector<discipline_supply_api_obj> get_discipline_supplies(const set<string>& names) const;
    set<string> lookup_discipline_supply_grantors(const string& lower_bound_name, uint32_t limit) const;

    // Witnesses
    vector<optional<witness_api_obj>> get_witnesses(const vector<witness_id_type>& witness_ids) const;
    fc::optional<witness_api_obj> get_witness_by_account(const string& account_name) const;
    set<account_name_type> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit) const;
    uint64_t get_witness_count() const;

    // Disciplines
    vector<discipline_api_obj> lookup_disciplines(const discipline_id_type& lower_bound, uint32_t limit) const;
    fc::optional<discipline_api_obj> get_discipline(const external_id_type& external_id) const;
    vector<discipline_api_obj> get_disciplines_by_parent(const external_id_type& parent_external_id) const;

    // Researches
    fc::optional<research_api_obj> get_research(const external_id_type& id) const;
    vector<research_api_obj> get_researches(const set<external_id_type>& ids) const;
    vector<research_api_obj> get_researches_by_research_group(const external_id_type& external_id) const;
    vector<research_api_obj> lookup_researches(const research_id_type& lower_bound, uint32_t limit) const;

    // Research contents
    fc::optional<research_content_api_obj> get_research_content(const external_id_type& id) const;
    vector<research_content_api_obj> get_research_contents(const set<external_id_type>& ids) const;
    fc::optional<research_content_api_obj> get_research_content_by_id(const research_content_id_type& internal_id) const;
    vector<research_content_api_obj> get_research_contents_by_research(const external_id_type& external_id) const;
    vector<research_content_api_obj> get_research_content_by_type(const research_id_type& research_id, const research_content_type& type) const;
    vector<research_content_api_obj> lookup_research_contents(const research_content_id_type& lower_bound, uint32_t limit) const;

    // Research license
    fc::optional<research_license_api_obj> get_research_license(const external_id_type& external_id) const;
    vector<research_license_api_obj> get_research_licenses(const set<external_id_type>& ids) const;
    vector<research_license_api_obj> get_research_licenses_by_licensee(const account_name_type& licensee) const;
    vector<research_license_api_obj> get_research_licenses_by_licenser(const account_name_type& licenser) const;
    vector<research_license_api_obj> get_research_licenses_by_research(const external_id_type& research_external_id) const;
    vector<research_license_api_obj> get_research_licenses_by_licensee_and_research(const account_name_type& licensee, const external_id_type& research_external_id) const;
    vector<research_license_api_obj> get_research_licenses_by_licensee_and_licenser(const account_name_type& licensee, const account_name_type& licenser) const;

    // Expert tokens
    fc::optional<expert_token_api_obj> get_expert_token(const expert_token_id_type id) const;
    vector<expert_token_api_obj> get_expert_tokens_by_account_name(const account_name_type account) const;
    vector<expert_token_api_obj> get_expert_tokens_by_discipline(const external_id_type& discipline_external_id) const;
    fc::optional<expert_token_api_obj> get_common_token_by_account_name(const account_name_type account_name) const;

    // Proposals
    fc::optional<proposal_api_obj> get_proposal(const external_id_type& external_id) const;
    vector<proposal_api_obj> get_proposals_by_creator(const account_name_type& creator) const;

    // Research token sales
    fc::optional<research_token_sale_api_obj> get_research_token_sale(const external_id_type& external_id) const;
    fc::optional<research_token_sale_api_obj> get_research_token_sale_by_id(const research_token_sale_id_type& token_sale_id) const;
    vector<research_token_sale_api_obj> get_research_token_sales_by_research_id(const research_id_type& research_id) const;
    vector<research_token_sale_api_obj> get_research_token_sales_by_research(const external_id_type& research_external_id) const;
    vector<research_token_sale_api_obj> get_research_token_sales(const uint32_t& from, uint32_t limit) const;
    vector<research_token_sale_contribution_api_obj> get_research_token_sale_contributions_by_research_token_sale(const external_id_type& token_sale_external_id) const;
    vector<research_token_sale_contribution_api_obj> get_research_token_sale_contributions_by_research_token_sale_id(const research_token_sale_id_type& token_sale_id) const;
    vector<research_token_sale_contribution_api_obj> get_research_token_sale_contributions_by_contributor(const account_name_type& owner) const;

    // Total votes
    fc::optional<expertise_contribution_object_api_obj> get_expertise_contribution_by_research_content_and_discipline(const research_content_id_type& research_content_id, const discipline_id_type& discipline_id) const;
    vector<expertise_contribution_object_api_obj> get_expertise_contributions_by_research(const research_id_type& research_id) const;
    vector<expertise_contribution_object_api_obj> get_expertise_contributions_by_research_and_discipline(const research_id_type& research_id, const discipline_id_type& discipline_id) const;
    vector<expertise_contribution_object_api_obj> get_expertise_contributions_by_research_content(const research_content_id_type& research_content_id) const;

    // Reviews
    fc::optional<review_api_obj> get_review(const external_id_type& external_id) const;
    vector<review_api_obj> get_reviews(const set<external_id_type>& ids) const;
    fc::optional<review_api_obj> get_review_by_id(const review_id_type& review_id) const;
    vector<review_api_obj> get_reviews_by_research(const external_id_type& research_external_id) const;
    vector<review_api_obj> get_reviews_by_research_content(const external_id_type& research_content_external_id) const;
    vector<review_api_obj> get_reviews_by_author(const account_name_type& author) const;

    // Grant application reviews
    vector<grant_application_review_api_obj> get_reviews_by_grant_application(const grant_application_id_type& grant_application_id) const;

    // Review vote object
    vector<review_vote_api_obj> get_review_votes_by_voter(const account_name_type& voter) const;
    vector<review_vote_api_obj> get_review_votes_by_review_id(const review_id_type& review_id) const;
    vector<review_vote_api_obj> get_review_votes_by_review(const external_id_type& review_external_id) const;

    // Expertise allocation proposals
    fc::optional<expertise_allocation_proposal_api_obj> get_expertise_allocation_proposal_by_id(const expertise_allocation_proposal_id_type& id) const;
    vector<expertise_allocation_proposal_api_obj> get_expertise_allocation_proposals_by_claimer(const account_name_type& claimer) const;
    fc::optional<expertise_allocation_proposal_api_obj> get_expertise_allocation_proposals_by_claimer_and_discipline(const account_name_type& claimer, const discipline_id_type& discipline_id) const;
    vector<expertise_allocation_proposal_api_obj> get_expertise_allocation_proposals_by_discipline(const discipline_id_type& discipline_id) const;

    // Expertise allocation votes
    fc::optional<expertise_allocation_proposal_vote_api_obj> get_expertise_allocation_proposal_vote_by_id(const expertise_allocation_proposal_vote_id_type& id) const;
    vector<expertise_allocation_proposal_vote_api_obj> get_expertise_allocation_proposal_votes_by_expertise_allocation_proposal_id(const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id) const;
    fc::optional<expertise_allocation_proposal_vote_api_obj> get_expertise_allocation_proposal_vote_by_voter_and_expertise_allocation_proposal_id(const account_name_type& voter, const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id) const;
    vector<expertise_allocation_proposal_vote_api_obj> get_expertise_allocation_proposal_votes_by_voter_and_discipline_id(const account_name_type& voter, const discipline_id_type& discipline_id) const;
    vector<expertise_allocation_proposal_vote_api_obj> get_expertise_allocation_proposal_votes_by_voter(const account_name_type& voter) const;

    // Vesting balances
    fc::optional<vesting_balance_api_obj> get_vesting_balance_by_id(const vesting_balance_id_type& vesting_balance_id) const;
    vector<vesting_balance_api_obj> get_vesting_balance_by_owner(const account_name_type& owner) const;

    // Grant applications
    fc::optional<grant_application_api_obj> get_grant_application(const grant_application_id_type& id) const;
    vector<grant_application_api_obj> get_grant_applications_by_funding_opportunity_number(const std::string& funding_opportunity_number) const;
    vector<grant_application_api_obj> get_grant_applications_by_research_id(const research_id_type& research_id) const;

    // Funding opportunities
    fc::optional<funding_opportunity_api_obj> get_funding_opportunity_announcement(const funding_opportunity_id_type& id) const;
    fc::optional<funding_opportunity_api_obj> get_funding_opportunity_announcement_by_number(const string& number) const;
    vector<funding_opportunity_api_obj> get_funding_opportunity_announcements_by_organization(const account_id_type& research_group_id) const;
    vector<funding_opportunity_api_obj> get_funding_opportunity_announcements_listing(const uint16_t&, const uint16_t& limit) const;

    // Assets
    fc::optional<asset_api_obj> get_asset(const asset_id_type& id) const;
    fc::optional<asset_api_obj> get_asset_by_symbol(const string& symbol) const;
    vector<asset_api_obj> get_assets_by_type(const uint8_t& type) const;
    vector<asset_api_obj> get_assets_by_issuer(const account_name_type& issuer) const;
    vector<asset_api_obj> lookup_assets(const string& lower_bound_symbol, uint32_t limit) const;

    // Account balances
    fc::optional<account_balance_api_obj> get_account_asset_balance(const account_name_type& owner, const string& symbol) const;
    vector<account_balance_api_obj> get_account_assets_balances(const account_name_type& owner) const;
    vector<account_balance_api_obj> get_accounts_asset_balances_by_asset(const string& symbol) const;

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

    // NDA Contracts
    fc::optional<nda_contract_api_obj> get_research_nda(const external_id_type& external_id) const;
    vector<nda_contract_api_obj> get_research_nda_by_creator(const account_name_type& creator) const;
    vector<nda_contract_api_obj> get_research_nda_by_hash(const string& hash) const;
    vector<nda_contract_api_obj> get_research_nda_by_research(const external_id_type& external_id) const;

    // NDA Contracts Requests
    fc::optional<nda_contract_file_access_api_obj> get_nda_contract_content_access_request(const external_id_type& external_id) const;
    vector<nda_contract_file_access_api_obj> get_nda_contract_content_access_requests_by_nda(const external_id_type& nda_external_id) const;
    vector<nda_contract_file_access_api_obj> get_nda_contract_content_access_requests_by_requester(const account_name_type& requester) const;

    // Contract agreements
    fc::optional<contract_agreement_api_obj> get_contract_agreement(const external_id_type& id) const;
    vector<contract_agreement_api_obj> get_contract_agreement_by_creator(const account_name_type& creator) const;

    // Authority / validation
    std::string get_transaction_hex(const signed_transaction& trx) const;
    bool verify_authority(const signed_transaction& trx) const;

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
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<account_api_obj>> database_api::get_accounts(const set<string>& names) const
{
    FC_ASSERT(names.size() <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->get_accounts(names); });
}

vector<optional<account_api_obj>> database_api_impl::get_accounts(const set<string>& names) const
{
    const auto& accounts_service = _db.obtain_service<chain::dbs_account>();
    const auto& account_balances_service = _db.obtain_service<chain::dbs_account_balance>();

    vector<optional<account_api_obj>> results;
    results.reserve(names.size());

    for (auto name : names)
    {
        optional<account_api_obj> result;
        const auto& account_opt = accounts_service.get_account_if_exists(name);
        
        if (account_opt.valid())
        {
            const auto& account = (*account_opt).get();
            const auto& auth = accounts_service.get_account_authority(name);
            const auto& account_balances = account_balances_service.get_account_balances_by_owner(name);

            result = account_api_obj(account, auth, account_balances);
        }

        results.push_back(result);
    }

    return results;
}

vector<account_api_obj> database_api::lookup_accounts(const string& lower_bound_name, uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->lookup_accounts(lower_bound_name, limit); });
}

vector<account_api_obj> database_api_impl::lookup_accounts(const string& lower_bound_name, uint32_t limit) const
{
    const auto& accounts_service = _db.obtain_service<chain::dbs_account>();
    const auto& account_balances_service = _db.obtain_service<chain::dbs_account_balance>();

    vector<account_api_obj> result;

    const auto& accounts = accounts_service.lookup_accounts(lower_bound_name, limit);

    for (const account_object& account : accounts)
    {
        const auto& auth = accounts_service.get_account_authority(account.name);
        const auto& account_balances = account_balances_service.get_account_balances_by_owner(account.name);
        result.push_back(account_api_obj(account, auth, account_balances));
    }

    return result;
}

uint64_t database_api::get_account_count() const
{
    return my->_db.with_read_lock([&]() { return my->get_account_count(); });
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

optional<account_bandwidth_api_obj> database_api::get_account_bandwidth(string account, witness::bandwidth_type type) const
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
    FC_ASSERT(limit <= DEIP_API_BULK_FETCH_LIMIT);
    FC_ASSERT(discipline_id > 0, "Cannot use root discipline.");
    FC_ASSERT(from >= 0, "From must be >= 0");

    return my->_db.with_read_lock([&]() { return my->get_accounts_by_expert_discipline(discipline_id, from, limit); });
}

vector<account_api_obj> database_api_impl::get_accounts_by_expert_discipline(const discipline_id_type& discipline_id, uint32_t from, uint32_t limit) const
{
    vector<account_api_obj> result;
    result.reserve(limit);

    const auto& accounts_service = _db.obtain_service<chain::dbs_account>();
    const auto& account_balances_service = _db.obtain_service<chain::dbs_account_balance>();

    auto accounts_by_expert_discipline = accounts_service.get_accounts_by_expert_discipline(discipline_id);

    if (from >= accounts_by_expert_discipline.size())
        return result;

    if (from + limit >= accounts_by_expert_discipline.size())
        limit = accounts_by_expert_discipline.size() - from;

    for (auto i = from; i < from + limit; i++)
    {
        const auto& account = accounts_by_expert_discipline[i].get();
        const auto& auth = accounts_service.get_account_authority(account.name);
        const auto account_balances = account_balances_service.get_account_balances_by_owner(account.name);
        result.push_back(account_api_obj(account, auth, account_balances));
    }

    return result;
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
        FC_ASSERT(limit <= DEIP_API_BULK_FETCH_LIMIT);

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
    FC_ASSERT(limit <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->lookup_witness_accounts(lower_bound_name, limit); });
}

set<account_name_type> database_api_impl::lookup_witness_accounts(const string& lower_bound_name, uint32_t limit) const
{
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

bool database_api::verify_authority(const signed_transaction& trx) const
{
    return my->_db.with_read_lock([&]() { return my->verify_authority(trx); });
}

bool database_api_impl::verify_authority(const signed_transaction& trx) const
{
    auto get_active = [&](const string& account_name) {
        return authority(_db.get<account_authority_object, by_account>(account_name).active);
    };

    auto get_owner = [&](const string& account_name) {
        return authority(_db.get<account_authority_object, by_account>(account_name).owner);
    };

    auto get_active_overrides = [&](const string& account_name, const uint16_t& op_tag) {
        fc::optional<authority> result;
        const auto& auth = _db.get<account_authority_object, by_account>(account_name);
        if (auth.active_overrides.find(op_tag) != auth.active_overrides.end())
        {
            result = auth.active_overrides.at(op_tag);
        }
        return result;
    };

    trx.verify_authority(get_chain_id(), get_active, get_owner, get_active_overrides);
    return true;
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
    FC_ASSERT(names.size() <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->get_discipline_supplies(names); });
}

vector<discipline_supply_api_obj> database_api_impl::get_discipline_supplies(const set<string>& names) const
{
    vector<discipline_supply_api_obj> results;

    chain::dbs_discipline_supply& discipline_supply_service = _db.obtain_service<chain::dbs_discipline_supply>();
    for (const auto& name : names)
    {
        auto discipline_supplies = discipline_supply_service.get_discipline_supplies_by_grantor(name);
        for (const chain::discipline_supply_object& discipline_supply : discipline_supplies)
        {
            results.push_back(discipline_supply_api_obj(discipline_supply));
        }
    }

    return results;
}

set<string> database_api::lookup_discipline_supply_grantors(const string& lower_bound_name, uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->lookup_discipline_supply_grantors(lower_bound_name, limit); });
}

set<string> database_api_impl::lookup_discipline_supply_grantors(const string& lower_bound_name, uint32_t limit) const
{
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

        const auto& account_balances_service = my->_db.obtain_service<chain::dbs_account_balance>();
        const auto& accounts_service = my->_db.obtain_service<chain::dbs_account>();

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

                const auto& account = accounts_service.get_account(acnt);
                const auto& auth = accounts_service.get_account_authority(acnt);
                const auto balances = account_balances_service.get_account_balances_by_owner(acnt);

                _state.accounts[acnt] = account_api_obj(account, auth, balances);

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
                    //     case operation::tag<create_account_operation>::value:
                    //     case operation::tag<update_account_operation>::value:
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
                const auto& account = accounts_service.get_account(a);
                const auto& auth = accounts_service.get_account_authority(a);
                const auto balances = account_balances_service.get_account_balances_by_owner(a);
                _state.accounts[a] = account_api_obj(account, auth, balances);
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

vector<discipline_api_obj> database_api::lookup_disciplines(const discipline_id_type& lower_bound, uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->lookup_disciplines(lower_bound, limit); });
}

vector<discipline_api_obj> database_api_impl::lookup_disciplines(const discipline_id_type& lower_bound, uint32_t limit) const
{
    vector<discipline_api_obj> results;
    const auto& discipline_service = _db.obtain_service<chain::dbs_discipline>();

    const auto& disciplines = discipline_service.lookup_disciplines(lower_bound, limit);
    for (const chain::discipline_object& discipline : disciplines)
    {
        results.push_back(discipline);
    }

    return results;
}

fc::optional<discipline_api_obj> database_api::get_discipline(const external_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_discipline(id); });
}

fc::optional<discipline_api_obj> database_api_impl::get_discipline(const external_id_type& id) const
{
    fc::optional<discipline_api_obj> result;
    const auto& discipline_service = _db.obtain_service<chain::dbs_discipline>();

    const auto& discipline_opt = discipline_service.get_discipline_if_exists(id);
    if (discipline_opt.valid())
    {
        const auto& discipline = (*discipline_opt).get();
        result = discipline_api_obj(discipline);
    }
    return result;
}

vector<discipline_api_obj> database_api::get_disciplines_by_parent(const external_id_type& parent_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_disciplines_by_parent(parent_id); });
}

vector<discipline_api_obj> database_api_impl::get_disciplines_by_parent(const external_id_type& parent_id) const
{
    vector<discipline_api_obj> results;

    const auto& discipline_service = _db.obtain_service<chain::dbs_discipline>();
    const auto& disciplines = discipline_service.get_disciplines_by_parent(parent_id);

    for (const chain::discipline_object &discipline : disciplines) 
    {
        results.push_back(discipline_api_obj(discipline));
    }

    return results;
}

optional<research_api_obj> database_api::get_research(const external_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research(id); });
}

fc::optional<research_api_obj> database_api_impl::get_research(const external_id_type& id) const
{
    optional<research_api_obj> result;
    const auto& research_service = _db.obtain_service<chain::dbs_research>();
    const auto& account_service = _db.obtain_service<chain::dbs_account>();
    const auto& discipline_service = _db.obtain_service<chain::dbs_discipline>();
    const auto& research_discipline_relation_service = _db.obtain_service<chain::dbs_research_discipline_relation>();

    const auto& research_opt = research_service.get_research_if_exists(id);

    if (research_opt.valid())
    {
        const auto& research = (*research_opt).get();
        vector<discipline_api_obj> disciplines;
        const auto& discipline_relations
            = research_discipline_relation_service.get_research_discipline_relations_by_research(research.id);

        for (const chain::research_discipline_relation_object& discipline_relation : discipline_relations)
        {
            disciplines.push_back(discipline_service.get_discipline(discipline_relation.discipline_external_id));
        }

        const auto& research_group = account_service.get_account(research.research_group);
        result = research_api_obj(research, disciplines, research_group_api_obj(research_group));
    }

    return result;
}

vector<research_api_obj> database_api::get_researches(const set<external_id_type>& ids) const
{
    FC_ASSERT(ids.size() <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->get_researches(ids); });
}

vector<research_api_obj> database_api_impl::get_researches(const set<external_id_type>& ids) const
{
    const auto& research_service = _db.obtain_service<chain::dbs_research>();
    const auto& discipline_service = _db.obtain_service<chain::dbs_discipline>();
    const auto& research_discipline_relation_service = _db.obtain_service<chain::dbs_research_discipline_relation>();
    const auto& account_service = _db.obtain_service<chain::dbs_account>();

    vector<research_api_obj> result;
    for (const auto& external_id : ids)
    {
        const auto& research_opt = research_service.get_research_if_exists(external_id);

        if (research_opt.valid())
        {
            const auto& research = (*research_opt).get();
            vector<discipline_api_obj> disciplines;

            const auto& discipline_relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research.id);
            for (const chain::research_discipline_relation_object& discipline_relation : discipline_relations)
            {
                disciplines.push_back(discipline_service.get_discipline(discipline_relation.discipline_external_id));
            }

            const auto& research_group = account_service.get_account(research.research_group);
            result.push_back(research_api_obj(research, disciplines, research_group_api_obj(research_group)));
        }
    }

    return result;
}


vector<research_api_obj> database_api::get_researches_by_research_group(const external_id_type& external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_researches_by_research_group(external_id); });
}

vector<research_api_obj> database_api_impl::get_researches_by_research_group(const external_id_type& external_id) const
{
    vector<research_api_obj> result;

    const auto& research_service = _db.obtain_service<chain::dbs_research>();
    const auto& account_service = _db.obtain_service<chain::dbs_account>();
    const auto& disciplines_service = _db.obtain_service<chain::dbs_discipline>();
    const auto& research_discipline_relation_service = _db.obtain_service<chain::dbs_research_discipline_relation>();

    const auto& research_group_opt = account_service.get_account_if_exists(external_id);
    if (!research_group_opt.valid())
    {
        return result;
    }

    const account_object& research_group = *research_group_opt;
    const auto& researches = research_service.get_researches_by_research_group(research_group.name);
    for (const research_object& research : researches)
    {
        vector<discipline_api_obj> disciplines;
        const auto& research_discipline_relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research.id);
        for (const research_discipline_relation_object& research_discipline_relation : research_discipline_relations)
        {
            disciplines.push_back(disciplines_service.get_discipline(research_discipline_relation.discipline_external_id));
        }

        result.push_back(research_api_obj(research, disciplines, research_group_api_obj(research_group)));
    }

    return result;
}

vector<research_api_obj> database_api::lookup_researches(const research_id_type& lower_bound, uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->lookup_researches(lower_bound, limit); });
}

vector<research_api_obj> database_api_impl::lookup_researches(const research_id_type& lower_bound,
                                                              uint32_t limit) const
{
    const auto& research_service = _db.obtain_service<chain::dbs_research>();
    const auto& account_service = _db.obtain_service<chain::dbs_account>();
    const auto& research_discipline_relation_service = _db.obtain_service<chain::dbs_research_discipline_relation>();
    const auto& disciplines_service = _db.obtain_service<chain::dbs_discipline>();

    vector<research_api_obj> result;

    const auto& researches = research_service.lookup_researches(lower_bound, limit);
    for (const research_object& research : researches)
    {
        vector<discipline_api_obj> disciplines;
        const auto& research_discipline_relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research.id);
        for (const research_discipline_relation_object& research_discipline_relation : research_discipline_relations)
        {
            disciplines.push_back(disciplines_service.get_discipline(research_discipline_relation.discipline_external_id));
        }

        const auto& research_group = account_service.get_account(research.research_group);
        result.push_back(research_api_obj(research, disciplines, research_group_api_obj(research_group)));
    }

    return result;
}


fc::optional<research_license_api_obj> database_api::get_research_license(const external_id_type& external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_license(external_id); });
}

fc::optional<research_license_api_obj> database_api_impl::get_research_license(const external_id_type& external_id) const
{
    fc::optional<research_license_api_obj> result;
    const auto& research_license_service = _db.obtain_service<chain::dbs_research_license>();

    const auto& research_license_opt = research_license_service.get_research_license_if_exists(external_id);
    if (research_license_opt.valid())
    {
        const research_license_object& research_license = *research_license_opt;
        result = research_license_api_obj(research_license);
    }
    return result;
}


vector<research_license_api_obj> database_api::get_research_licenses(const set<external_id_type>& ids) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_licenses(ids); });
}

vector<research_license_api_obj> database_api_impl::get_research_licenses(const set<external_id_type>& ids) const
{
    vector<research_license_api_obj> results;
    const auto& research_license_service = _db.obtain_service<chain::dbs_research_license>();

    for (const auto& external_id : ids)
    {
        const auto& research_license_opt = research_license_service.get_research_license_if_exists(external_id);
        if (research_license_opt.valid())
        {
            const research_license_object& research_license = *research_license_opt;
            results.push_back(research_license_api_obj(research_license));
        }
    }

    return results;
}


vector<research_license_api_obj> database_api::get_research_licenses_by_licensee(const account_name_type& licensee) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_licenses_by_licensee(licensee); });
}

vector<research_license_api_obj> database_api_impl::get_research_licenses_by_licensee(const account_name_type& licensee) const
{
    vector<research_license_api_obj> results;
    const auto& research_license_service = _db.obtain_service<chain::dbs_research_license>();

    const auto& research_licenses = research_license_service.get_research_licenses_by_licensee(licensee);
    for (const auto& research_license : research_licenses)
    {
        results.push_back(research_license_api_obj(research_license));
    }

    return results;
}


vector<research_license_api_obj> database_api::get_research_licenses_by_licenser(const account_name_type& licenser) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_licenses_by_licenser(licenser); });
}

vector<research_license_api_obj> database_api_impl::get_research_licenses_by_licenser(const account_name_type& licenser) const
{
    vector<research_license_api_obj> results;
    const auto& research_license_service = _db.obtain_service<chain::dbs_research_license>();

    const auto& research_licenses = research_license_service.get_research_licenses_by_licenser(licenser);
    for (const auto& research_license : research_licenses)
    {
        results.push_back(research_license_api_obj(research_license));
    }

    return results;
}


vector<research_license_api_obj> database_api::get_research_licenses_by_research(const external_id_type& research_external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_licenses_by_research(research_external_id); });
}

vector<research_license_api_obj> database_api_impl::get_research_licenses_by_research(const external_id_type& research_external_id) const
{
    vector<research_license_api_obj> results;
    const auto& research_license_service = _db.obtain_service<chain::dbs_research_license>();

    const auto& research_licenses = research_license_service.get_research_licenses_by_research(research_external_id);
    for (const auto& research_license : research_licenses)
    {
        results.push_back(research_license_api_obj(research_license));
    }

    return results;
}


vector<research_license_api_obj> database_api::get_research_licenses_by_licensee_and_research(const account_name_type& licensee, const external_id_type& research_external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_licenses_by_licensee_and_research(licensee, research_external_id); });
}

vector<research_license_api_obj> database_api_impl::get_research_licenses_by_licensee_and_research(const account_name_type& licensee, const external_id_type& research_external_id) const
{
    vector<research_license_api_obj> results;
    const auto& research_license_service = _db.obtain_service<chain::dbs_research_license>();

    const auto& research_licenses = research_license_service.get_research_licenses_by_licensee_and_research(licensee, research_external_id);
    for (const auto& research_license : research_licenses)
    {
        results.push_back(research_license_api_obj(research_license));
    }

    return results;
}


vector<research_license_api_obj> database_api::get_research_licenses_by_licensee_and_licenser(const account_name_type& licensee, const account_name_type& licenser) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_licenses_by_licensee_and_licenser(licensee, licenser); });
}

vector<research_license_api_obj> database_api_impl::get_research_licenses_by_licensee_and_licenser(const account_name_type& licensee, const account_name_type& licenser) const
{
    vector<research_license_api_obj> results;
    const auto& research_license_service = _db.obtain_service<chain::dbs_research_license>();

    const auto& research_licenses = research_license_service.get_research_licenses_by_licensee_and_licenser(licensee, licenser);
    for (const auto& research_license : research_licenses)
    {
        results.push_back(research_license_api_obj(research_license));
    }

    return results;
}


fc::optional<research_content_api_obj> database_api::get_research_content(const external_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_content(id); });
}

fc::optional<research_content_api_obj> database_api_impl::get_research_content(const external_id_type& id) const
{
    fc::optional<research_content_api_obj> result;
    const auto& research_content_service = _db.obtain_service<chain::dbs_research_content>();

    const auto& research_content_opt = research_content_service.get_research_content_if_exists(id);
    if (research_content_opt.valid())
    {
        const auto& research_content = (*research_content_opt).get();
        result = research_content_api_obj(research_content);
    }
    return result;
}

vector<research_content_api_obj> database_api::get_research_contents(const set<external_id_type>& ids) const
{
    FC_ASSERT(ids.size() <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->get_research_contents(ids); });
}

vector<research_content_api_obj> database_api_impl::get_research_contents(const set<external_id_type>& ids) const
{
    vector<research_content_api_obj> result;
    const auto& research_content_service = _db.obtain_service<chain::dbs_research_content>();

    for (const auto& external_id : ids)
    {
        const auto& research_content_opt = research_content_service.get_research_content_if_exists(external_id);
        if (research_content_opt.valid())
        {
            const auto& research_content = (*research_content_opt).get();
            result.push_back(research_content_api_obj(research_content));
        }
    }

    return result;
}

fc::optional<research_content_api_obj> database_api::get_research_content_by_id(const research_content_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_content_by_id(id); });
}

fc::optional<research_content_api_obj> database_api_impl::get_research_content_by_id(const research_content_id_type& id) const
{
    fc::optional<research_content_api_obj> result;
    const auto& research_content_service = _db.obtain_service<chain::dbs_research_content>();

    const auto& research_content_opt = research_content_service.get_research_content_if_exists(id);
    if (research_content_opt.valid())
    {
        const auto& research_content = (*research_content_opt).get();
        result = research_content_api_obj(research_content);
    }
    return result;
}

vector<research_content_api_obj> database_api::get_research_contents_by_research(const external_id_type& external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_contents_by_research(external_id); });
}

vector<research_content_api_obj> database_api_impl::get_research_contents_by_research(const external_id_type& external_id) const
{
    const auto& research_service = _db.obtain_service<chain::dbs_research>();

    vector<research_content_api_obj> results;
    const auto& research_opt = research_service.get_research_if_exists(external_id);

    if (!research_opt.valid())
    {
        return results;
    }

    const auto& research = (*research_opt).get();

    const auto& research_content_service = _db.obtain_service<chain::dbs_research_content>();
    const auto& contents = research_content_service.get_research_content_by_research_id(research.id);

    for (const auto& content : contents) 
    {
        results.push_back(research_content_api_obj(content));
    }

    return results;
}

vector<research_content_api_obj> database_api::get_research_content_by_type(const research_id_type& research_id, const research_content_type& type) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_content_by_type(research_id, type); });
}

vector<research_content_api_obj> database_api_impl::get_research_content_by_type(const research_id_type& research_id, const research_content_type& type) const
{
    vector<research_content_api_obj> results;
    const auto& research_content_service = _db.obtain_service<chain::dbs_research_content>();
    auto contents = research_content_service.get_by_research_and_type(research_id, type);

    for (const chain::research_content_object &content : contents) {
        results.push_back(research_content_api_obj(content));
    }
    return results;
}

vector<research_content_api_obj> database_api::lookup_research_contents(const research_content_id_type& lower_bound, uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->lookup_research_contents(lower_bound, limit); });
}

vector<research_content_api_obj> database_api_impl::lookup_research_contents(const research_content_id_type& lower_bound, uint32_t limit) const
{
    vector<research_content_api_obj> results;
    const auto& research_content_service = _db.obtain_service<chain::dbs_research_content>();
    const auto& contents = research_content_service.lookup_research_contents(lower_bound, limit);

    for (const chain::research_content_object &content : contents) 
    {
        results.push_back(research_content_api_obj(content));
    }

    return results;
}

fc::optional<expert_token_api_obj> database_api::get_expert_token(const expert_token_id_type id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expert_token(id); });
}

fc::optional<expert_token_api_obj> database_api_impl::get_expert_token(const expert_token_id_type id) const
{
    fc::optional<expert_token_api_obj> result;
    const auto& expert_token_service = _db.obtain_service<chain::dbs_expert_token>();

    const auto& expert_token_opt = expert_token_service.get_expert_token_if_exists(id);
    if (expert_token_opt.valid())
    {
        fc::optional<discipline_api_obj> discipline_opt = get_discipline((*expert_token_opt).get().discipline_external_id);
        if (discipline_opt.valid())
        {
            const auto& expert_token = (*expert_token_opt).get();
            result = expert_token_api_obj(expert_token, discipline_opt->name);
        }
    }
    return result;
}

vector<expert_token_api_obj> database_api::get_expert_tokens_by_account_name(const account_name_type account_name) const
{
    return my->_db.with_read_lock([&]() { return my->get_expert_tokens_by_account_name(account_name); });
}

vector<expert_token_api_obj> database_api_impl::get_expert_tokens_by_account_name(const account_name_type account_name) const
{
    vector<expert_token_api_obj> results;
    const auto& expert_token_service = _db.obtain_service<chain::dbs_expert_token>();
    const auto& discipline_service = _db.obtain_service<chain::dbs_discipline>();
    const auto& expert_tokens = expert_token_service.get_expert_tokens_by_account_name(account_name);

    for (const chain::expert_token_object &expert_token : expert_tokens) {
        auto& discipline = discipline_service.get_discipline(expert_token.discipline_external_id);
        if (expert_token.discipline_id != 0)
            results.push_back(expert_token_api_obj(expert_token, fc::to_string(discipline.name)));
    }

    return results;
}

vector<expert_token_api_obj> database_api::get_expert_tokens_by_discipline(const external_id_type& discipline_external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expert_tokens_by_discipline(discipline_external_id); });
}

vector<expert_token_api_obj> database_api_impl::get_expert_tokens_by_discipline(const external_id_type& discipline_external_id) const
{
    vector<expert_token_api_obj> results;

    const auto& expert_token_service = _db.obtain_service<chain::dbs_expert_token>();
    const auto& discipline_service = _db.obtain_service<chain::dbs_discipline>();
    auto expert_tokens = expert_token_service.get_expert_tokens_by_discipline(discipline_external_id);

    for (const chain::expert_token_object &expert_token : expert_tokens) 
    {
        auto& discipline = discipline_service.get_discipline(expert_token.discipline_external_id);
        results.push_back(expert_token_api_obj(expert_token, fc::to_string(discipline.name)));
    }

    return results;
}

fc::optional<expert_token_api_obj> database_api::get_common_token_by_account_name(const account_name_type account_name) const
{
    return my->_db.with_read_lock([&]() { return my->get_common_token_by_account_name(account_name); });
}

fc::optional<expert_token_api_obj> database_api_impl::get_common_token_by_account_name(const account_name_type account_name) const
{
    fc::optional<expert_token_api_obj> result;
    const auto& expert_token_service = _db.obtain_service<chain::dbs_expert_token>();

    const auto& expert_token_opt = expert_token_service.get_expert_token_by_account_and_discipline_if_exists(account_name, 0);
    if (expert_token_opt.valid())
    {
        fc::optional<discipline_api_obj> discipline_opt = get_discipline((*expert_token_opt).get().discipline_external_id);
        if (discipline_opt.valid())
        {
            const auto& expert_token = (*expert_token_opt).get();
            result = expert_token_api_obj(expert_token, discipline_opt->name);
        }
    }
    return result;
}

fc::optional<proposal_api_obj> database_api::get_proposal(const external_id_type& external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_proposal(external_id); });
}

fc::optional<proposal_api_obj> database_api_impl::get_proposal(const external_id_type& external_id) const
{
    fc::optional<proposal_api_obj> result;

    const auto& proposals_service = _db.obtain_service<chain::dbs_proposal>();
    const auto& proposal_opt = proposals_service.get_proposal_if_exists(external_id);

    if (proposal_opt.valid())
    {
        result = proposal_api_obj((*proposal_opt).get());
    }

    return result;
}

vector<proposal_api_obj> database_api::get_proposals_by_creator(const account_name_type& creator) const
{
    return my->_db.with_read_lock([&]() { return my->get_proposals_by_creator(creator); });
}

vector<proposal_api_obj> database_api_impl::get_proposals_by_creator(const account_name_type& creator) const
{
    vector<proposal_api_obj> results;

    const auto& proposal_service = _db.obtain_service<chain::dbs_proposal>();

    const auto& proposals = proposal_service.get_proposals_by_creator(creator);
    for (const chain::proposal_object& proposal : proposals)
    {
        results.push_back(proposal_api_obj(proposal));
    }

    return results;
}


fc::optional<research_token_sale_api_obj>
database_api::get_research_token_sale(const external_id_type& external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_token_sale(external_id); });
}

fc::optional<research_token_sale_api_obj>
database_api_impl::get_research_token_sale(const external_id_type& external_id) const
{
    fc::optional<research_token_sale_api_obj> result;

    const auto& research_token_sale_service = _db.obtain_service<chain::dbs_research_token_sale>();
    const auto& research_token_sale_opt = research_token_sale_service.get_research_token_sale_if_exists(external_id);

    if (research_token_sale_opt.valid())
    {
        result = research_token_sale_api_obj((*research_token_sale_opt).get());
    }

    return result;
}

vector<research_token_sale_api_obj>
database_api::get_research_token_sales_by_research(const external_id_type& research_external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_token_sales_by_research(research_external_id); });
}

vector<research_token_sale_api_obj> database_api_impl::get_research_token_sales_by_research(const external_id_type& research_external_id) const
{
    vector<research_token_sale_api_obj> results;
    const auto& research_token_sale_service = _db.obtain_service<chain::dbs_research_token_sale>();

    const auto& research_token_sales = research_token_sale_service.get_research_token_sales_by_research(research_external_id);
    for (const research_token_sale_object& research_token_sale : research_token_sales)
    {
        results.push_back(research_token_sale);
    }

    return results;
}

fc::optional<research_token_sale_api_obj>
database_api::get_research_token_sale_by_id(const research_token_sale_id_type& token_sale_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_token_sale_by_id(token_sale_id); });
}

fc::optional<research_token_sale_api_obj>
database_api_impl::get_research_token_sale_by_id(const research_token_sale_id_type& token_sale_id) const
{
    fc::optional<research_token_sale_api_obj> result;

    const auto& research_token_sale_service = _db.obtain_service<chain::dbs_research_token_sale>();
    const auto& research_token_sale_opt = research_token_sale_service.get_research_token_sale_if_exists(token_sale_id);

    if (research_token_sale_opt.valid())
    {
        result = research_token_sale_api_obj((*research_token_sale_opt).get());
    }

    return result;
}

vector<research_token_sale_api_obj> database_api::get_research_token_sales(const uint32_t& from = 0, uint32_t limit = 100) const
{
    FC_ASSERT(limit <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->get_research_token_sales(from, limit); });
}

vector<research_token_sale_api_obj> database_api_impl::get_research_token_sales(const uint32_t& from = 0, uint32_t limit = 100) const
{
    const auto& research_token_sale_by_id = _db.get_index<research_token_sale_index>().indices().get<by_id>();
    vector<research_token_sale_api_obj> result;
    result.reserve(limit);

    for (auto itr = research_token_sale_by_id.lower_bound(from); limit-- && itr != research_token_sale_by_id.end(); ++itr)
    {
        result.push_back(research_token_sale_api_obj(*itr));
    }

    return result;
}

vector<research_token_sale_api_obj>
database_api::get_research_token_sales_by_research_id(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_token_sales_by_research_id(research_id); });
}

vector<research_token_sale_api_obj>
database_api_impl::get_research_token_sales_by_research_id(const research_id_type& research_id) const
{
    vector<research_token_sale_api_obj> results;
    const auto& research_token_sale_service = _db.obtain_service<chain::dbs_research_token_sale>();

    const auto& research_token_sales = research_token_sale_service.get_by_research_id(research_id);

    for (const chain::research_token_sale_object& research_token_sale : research_token_sales)
    {
        results.push_back(research_token_sale);
    }

    return results;
}

vector<research_token_sale_contribution_api_obj>
database_api::get_research_token_sale_contributions_by_research_token_sale(const external_id_type& token_sale_external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_token_sale_contributions_by_research_token_sale(token_sale_external_id); });
}

vector<research_token_sale_contribution_api_obj>
database_api_impl::get_research_token_sale_contributions_by_research_token_sale(const external_id_type& token_sale_external_id) const
{
    vector<research_token_sale_contribution_api_obj> results;
    const auto& research_token_sale_service = _db.obtain_service<chain::dbs_research_token_sale>();

    const auto& research_token_sale_contributions = research_token_sale_service.get_research_token_sale_contributions_by_research_token_sale(token_sale_external_id);
    for (const research_token_sale_contribution_object& research_token_sale_contribution : research_token_sale_contributions)
    {
        results.push_back(research_token_sale_contribution);
    }

    return results;
}


vector<research_token_sale_contribution_api_obj>
database_api::get_research_token_sale_contributions_by_research_token_sale_id(const research_token_sale_id_type& token_sale_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_token_sale_contributions_by_research_token_sale_id(token_sale_id); });
}

vector<research_token_sale_contribution_api_obj>
database_api_impl::get_research_token_sale_contributions_by_research_token_sale_id(const research_token_sale_id_type& token_sale_id) const
{
    vector<research_token_sale_contribution_api_obj> results;
    const auto& research_token_sale_service = _db.obtain_service<chain::dbs_research_token_sale>();

    const auto& research_token_sale_contributions = research_token_sale_service.get_research_token_sale_contributions_by_research_token_sale_id(token_sale_id);

    for (const chain::research_token_sale_contribution_object& research_token_sale_contribution : research_token_sale_contributions)
    {
        results.push_back(research_token_sale_contribution);
    }

    return results;
}


vector<research_token_sale_contribution_api_obj>
database_api::get_research_token_sale_contributions_by_contributor(const account_name_type& owner) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_token_sale_contributions_by_contributor(owner); });
}

vector<research_token_sale_contribution_api_obj>
database_api_impl::get_research_token_sale_contributions_by_contributor(const account_name_type& owner) const
{
    vector<research_token_sale_contribution_api_obj> results;
    const auto& research_token_sale_service = _db.obtain_service<chain::dbs_research_token_sale>();
    const auto& research_token_sale_contributions = research_token_sale_service.get_research_token_sale_contributions_by_contributor(owner);

    for (const chain::research_token_sale_contribution_object& research_token_sale_contribution : research_token_sale_contributions)
    {
        results.push_back(research_token_sale_contribution);
    }
    return results;
}

vector<discipline_api_obj> database_api::get_disciplines_by_research(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<discipline_api_obj> results;
        const auto& research_discipline_relation_service = my->_db.obtain_service<chain::dbs_research_discipline_relation>();

        const auto& research_discipline_relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research_id);

        for (const chain::research_discipline_relation_object& research_discipline_relation : research_discipline_relations)
        {
            results.push_back(*get_discipline(research_discipline_relation.discipline_external_id));
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
    const auto& expertise_contributions_service = _db.obtain_service<chain::dbs_expertise_contribution>();

    const auto& expertise_contributions = expertise_contributions_service.get_expertise_contributions_by_research(research_id);
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
    const auto& expertise_contributions_service = _db.obtain_service<chain::dbs_expertise_contribution>();

    const auto& expertise_contributions = expertise_contributions_service.get_expertise_contributions_by_research_and_discipline(research_id, discipline_id);
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
    const auto& expertise_contributions_service = _db.obtain_service<chain::dbs_expertise_contribution>();

    const auto& expertise_contributions = expertise_contributions_service.get_expertise_contributions_by_research_content(research_content_id);

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

    const auto& expertise_contribution_service = _db.obtain_service<chain::dbs_expertise_contribution>();
    const auto& expertise_contribution_opt = expertise_contribution_service.get_expertise_contribution_by_research_content_and_discipline_if_exists(research_content_id, discipline_id);

    if (expertise_contribution_opt.valid())
    {
        result = expertise_contribution_object_api_obj((*expertise_contribution_opt).get());
    }

    return result;
}

fc::optional<review_api_obj> database_api::get_review(const external_id_type& external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_review(external_id); });
}

fc::optional<review_api_obj> database_api_impl::get_review(const external_id_type& external_id) const
{
    fc::optional<review_api_obj> result;

    const auto& review_service = _db.obtain_service<chain::dbs_review>();
    const auto& review_opt = review_service.get_review_if_exists(external_id);

    if (review_opt.valid())
    {
        vector<discipline_api_obj> disciplines;
        for (const auto& discipline_id : (*review_opt).get().disciplines_external_ids)
        {
            const auto& discipline = get_discipline(discipline_id);
            disciplines.push_back(*discipline);
        }

        result = review_api_obj(*review_opt, disciplines);
    }

    return result;
}

vector<review_api_obj> database_api::get_reviews(const set<external_id_type>& ids) const
{
    FC_ASSERT(ids.size() <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->get_reviews(ids); });
}

vector<review_api_obj> database_api_impl::get_reviews(const set<external_id_type>& ids) const
{
    vector<review_api_obj> result;
    const auto& review_service = _db.obtain_service<chain::dbs_review>();

    for (const auto& external_id : ids)
    {
        const auto& review_opt = review_service.get_review_if_exists(external_id);
        if (review_opt.valid())
        {
            const review_object& review = *review_opt;
            vector<discipline_api_obj> disciplines;
            for (const auto& discipline_id : review.disciplines_external_ids)
            {
                const auto& discipline = get_discipline(discipline_id);
                disciplines.push_back(*discipline);
            }

            result.push_back(review_api_obj(review, disciplines));
        }
    }

    return result;
}

fc::optional<review_api_obj> database_api::get_review_by_id(const review_id_type& review_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_review_by_id(review_id); });
}

fc::optional<review_api_obj> database_api_impl::get_review_by_id(const review_id_type& review_id) const
{
    fc::optional<review_api_obj> result;

    const auto& review_service = _db.obtain_service<chain::dbs_review>();
    const auto& review_opt = review_service.get_review_if_exists(review_id);

    if (review_opt.valid())
    {
        vector<discipline_api_obj> disciplines;

        for (const auto discipline_id : (*review_opt).get().disciplines_external_ids) 
        {
            auto discipline_ao = get_discipline(discipline_id);
            disciplines.push_back(*discipline_ao);
        }

        result = review_api_obj(*review_opt, disciplines);
    }

    return result;
}

vector<review_api_obj> database_api::get_reviews_by_research(const external_id_type& research_external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_reviews_by_research(research_external_id); });
}

vector<review_api_obj> database_api_impl::get_reviews_by_research(const external_id_type& research_external_id) const
{
    vector<review_api_obj> results;
    const auto& research_content_service = _db.obtain_service<chain::dbs_research_content>();
    const auto& research_service = _db.obtain_service<chain::dbs_research>();

    const auto& research_opt = research_service.get_research_if_exists(research_external_id);

    if (research_opt.valid())
    {
        const auto& research = (*research_opt).get();
        const auto& contents = research_content_service.get_research_content_by_research_id(research.id);

        for (const chain::research_content_object& research_content : contents)
        {
            auto reviews = get_reviews_by_research_content(research_content.external_id);
            results.insert(results.end(), reviews.begin(), reviews.end());
        }
    }
    return results;
}

vector<review_api_obj> database_api::get_reviews_by_research_content(const external_id_type& research_content_external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_reviews_by_research_content(research_content_external_id); });
}

vector<review_api_obj> database_api_impl::get_reviews_by_research_content(const external_id_type& research_content_external_id) const
{
    vector<review_api_obj> results;
    const auto& review_service = _db.obtain_service<chain::dbs_review>();

    const auto& reviews = review_service.get_reviews_by_research_content(research_content_external_id);

    for (const chain::review_object& review : reviews)
    {
        vector<discipline_api_obj> disciplines;

        for (const auto discipline_id : review.disciplines_external_ids)
        {
            auto discipline = get_discipline(discipline_id);
            disciplines.push_back(*discipline);
        }

        review_api_obj api_obj = review_api_obj(review, disciplines);
        results.push_back(api_obj);
    }

    return results;
}

vector<review_api_obj> database_api::get_reviews_by_author(const account_name_type& author) const
{
    return my->_db.with_read_lock([&]() { return my->get_reviews_by_author(author); });
}

vector<review_api_obj> database_api_impl::get_reviews_by_author(const account_name_type& author) const
{
    vector<review_api_obj> results;
    const auto& review_service = _db.obtain_service<chain::dbs_review>();

    const auto& reviews = review_service.get_author_reviews(author);

    for (const chain::review_object& review : reviews)
    {
        vector<discipline_api_obj> disciplines;

        for (const auto discipline_id : review.disciplines_external_ids)
        {
            auto discipline_ao = get_discipline(discipline_id);
            disciplines.push_back(*discipline_ao);
        }

        review_api_obj api_obj = review_api_obj(review, disciplines);
        results.push_back(api_obj);
    }

    return results;
}

vector<grant_application_review_api_obj> database_api::get_reviews_by_grant_application(const grant_application_id_type& grant_application_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_reviews_by_grant_application(grant_application_id); });
}

vector<grant_application_review_api_obj> database_api_impl::get_reviews_by_grant_application(const grant_application_id_type& grant_application_id) const
{
    vector<grant_application_review_api_obj> results;
    const auto& grant_application_service = _db.obtain_service<chain::dbs_grant_application>();
    const auto& reviews = grant_application_service.get_grant_application_reviews(grant_application_id);

    for (const chain::grant_application_review_object& review : reviews)
    {
        vector<discipline_api_obj> disciplines;

        grant_application_review_api_obj api_obj = grant_application_review_api_obj(review, disciplines);
        results.push_back(api_obj);
    }

    return results;
}

vector<review_vote_api_obj> database_api::get_review_votes_by_voter(const account_name_type &voter) const
{
    return my->_db.with_read_lock([&]() { return my->get_review_votes_by_voter(voter); });
}

vector<review_vote_api_obj> database_api_impl::get_review_votes_by_voter(const account_name_type &voter) const
{
    vector<review_vote_api_obj> results;
    const auto& review_votes_service = _db.obtain_service<chain::dbs_review_vote>();

    const auto& review_votes = review_votes_service.get_review_votes_by_voter(voter);

    for (const chain::review_vote_object& review_vote : review_votes)
        results.push_back(review_vote);

    return results;
}

vector<review_vote_api_obj> database_api::get_review_votes_by_review(const external_id_type& review_external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_review_votes_by_review(review_external_id); });
}

vector<review_vote_api_obj> database_api_impl::get_review_votes_by_review(const external_id_type& review_external_id) const
{
    vector<review_vote_api_obj> results;
    const auto& review_votes_service = _db.obtain_service<chain::dbs_review_vote>();

    const auto& review_votes = review_votes_service.get_review_votes(review_external_id);

    for (const chain::review_vote_object& review_vote : review_votes)
        results.push_back(review_vote);

    return results;
}

vector<review_vote_api_obj> database_api::get_review_votes_by_review_id(const review_id_type &review_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_review_votes_by_review_id(review_id); });
}

vector<review_vote_api_obj> database_api_impl::get_review_votes_by_review_id(const review_id_type &review_id) const
{
    vector<review_vote_api_obj> results;
    const auto& review_votes_service = _db.obtain_service<chain::dbs_review_vote>();

    const auto& review_votes = review_votes_service.get_review_votes(review_id);

    for (const chain::review_vote_object& review_vote : review_votes)
        results.push_back(review_vote);

    return results;
}

fc::optional<expertise_allocation_proposal_api_obj> database_api::get_expertise_allocation_proposal_by_id(const expertise_allocation_proposal_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_allocation_proposal_by_id(id); });
}

fc::optional<expertise_allocation_proposal_api_obj> database_api_impl::get_expertise_allocation_proposal_by_id(const expertise_allocation_proposal_id_type& id) const
{
    fc::optional<expertise_allocation_proposal_api_obj> result;

    const auto& expertise_allocation_proposal_service = _db.obtain_service<chain::dbs_expertise_allocation_proposal>();
    const auto& expertise_allocation_proposal_opt = expertise_allocation_proposal_service.get_expertise_allocation_proposal_if_exists(id);

    if (expertise_allocation_proposal_opt.valid())
    {
        result = expertise_allocation_proposal_api_obj(*expertise_allocation_proposal_opt);
    }

    return result;
}

vector<expertise_allocation_proposal_api_obj> database_api::get_expertise_allocation_proposals_by_claimer(const account_name_type &claimer) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_allocation_proposals_by_claimer(claimer); });
}

vector<expertise_allocation_proposal_api_obj> database_api_impl::get_expertise_allocation_proposals_by_claimer(const account_name_type &claimer) const
{
    vector<expertise_allocation_proposal_api_obj> results;
    const auto& expertise_allocation_proposal_service = _db.obtain_service<chain::dbs_expertise_allocation_proposal>();

    const auto& proposals = expertise_allocation_proposal_service.get_by_claimer(claimer);

    for (const chain::expertise_allocation_proposal_object& proposal : proposals)
        results.push_back(proposal);

    return results;
}

fc::optional<expertise_allocation_proposal_api_obj> database_api::get_expertise_allocation_proposals_by_claimer_and_discipline(const account_name_type& claimer,
                                                                                                                               const discipline_id_type& discipline_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_allocation_proposals_by_claimer_and_discipline(claimer, discipline_id); });
}

fc::optional<expertise_allocation_proposal_api_obj> database_api_impl::get_expertise_allocation_proposals_by_claimer_and_discipline(const account_name_type& claimer,
                                                                                                                                    const discipline_id_type& discipline_id) const
{
    fc::optional<expertise_allocation_proposal_api_obj> result;

    const auto& expertise_allocation_proposal_service = _db.obtain_service<chain::dbs_expertise_allocation_proposal>();
    const auto& expertise_allocation_proposal_opt = expertise_allocation_proposal_service.get_expertise_allocation_proposal_by_claimer_and_discipline_if_exists(claimer, discipline_id);

    if (expertise_allocation_proposal_opt.valid())
    {
        result = expertise_allocation_proposal_api_obj(*expertise_allocation_proposal_opt);
    }

    return result;
}

vector<expertise_allocation_proposal_api_obj> database_api::get_expertise_allocation_proposals_by_discipline(const discipline_id_type& discipline_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_allocation_proposals_by_discipline(discipline_id); });
}

vector<expertise_allocation_proposal_api_obj> database_api_impl::get_expertise_allocation_proposals_by_discipline(const discipline_id_type& discipline_id) const
{
    vector<expertise_allocation_proposal_api_obj> results;
    const auto& expertise_allocation_proposal_service = _db.obtain_service<chain::dbs_expertise_allocation_proposal>();

    const auto& proposals = expertise_allocation_proposal_service.get_by_discipline_id(discipline_id);

    for (const chain::expertise_allocation_proposal_object& proposal : proposals)
        results.push_back(proposal);

    return results;
}

fc::optional<expertise_allocation_proposal_vote_api_obj> database_api::get_expertise_allocation_proposal_vote_by_id(const expertise_allocation_proposal_vote_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_allocation_proposal_vote_by_id(id); });
}

fc::optional<expertise_allocation_proposal_vote_api_obj> database_api_impl::get_expertise_allocation_proposal_vote_by_id(const expertise_allocation_proposal_vote_id_type& id) const
{
    fc::optional<expertise_allocation_proposal_vote_api_obj> result;

    const auto& expertise_allocation_proposal_service = _db.obtain_service<chain::dbs_expertise_allocation_proposal>();
    const auto& expertise_allocation_proposal_vote_opt = expertise_allocation_proposal_service.get_expertise_allocation_proposal_vote_if_exists(id);

    if (expertise_allocation_proposal_vote_opt.valid())
    {
        result = expertise_allocation_proposal_vote_api_obj(*expertise_allocation_proposal_vote_opt);
    }

    return result;
}

vector<expertise_allocation_proposal_vote_api_obj> database_api::get_expertise_allocation_proposal_votes_by_expertise_allocation_proposal_id
                                                                                        (const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_allocation_proposal_votes_by_expertise_allocation_proposal_id(expertise_allocation_proposal_id); });
}

vector<expertise_allocation_proposal_vote_api_obj> database_api_impl::get_expertise_allocation_proposal_votes_by_expertise_allocation_proposal_id
                                                                                        (const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id) const
{
    vector<expertise_allocation_proposal_vote_api_obj> results;
    const auto& expertise_allocation_proposal_vote_service = _db.obtain_service<chain::dbs_expertise_allocation_proposal>();

    const auto& proposal_votes = expertise_allocation_proposal_vote_service.get_votes_by_expertise_allocation_proposal_id(expertise_allocation_proposal_id);

    for (const chain::expertise_allocation_proposal_vote_object& proposal_vote : proposal_votes)
        results.push_back(proposal_vote);

    return results;
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
    fc::optional<expertise_allocation_proposal_vote_api_obj> result;

    const auto& expertise_allocation_proposal_service = _db.obtain_service<chain::dbs_expertise_allocation_proposal>();
    const auto& expertise_allocation_proposal_vote_opt =
            expertise_allocation_proposal_service.get_expertise_allocation_proposal_vote_by_voter_and_expertise_allocation_proposal_id_if_exists(voter, expertise_allocation_proposal_id);

    if (expertise_allocation_proposal_vote_opt.valid())
    {
        result = expertise_allocation_proposal_vote_api_obj(*expertise_allocation_proposal_vote_opt);
    }

    return result;
}

vector<expertise_allocation_proposal_vote_api_obj> database_api::get_expertise_allocation_proposal_votes_by_voter_and_discipline_id(const account_name_type& voter,
                                                                                                                                    const discipline_id_type& discipline_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_allocation_proposal_votes_by_voter_and_discipline_id(voter, discipline_id); });
}

vector<expertise_allocation_proposal_vote_api_obj> database_api_impl::get_expertise_allocation_proposal_votes_by_voter_and_discipline_id(const account_name_type& voter,
                                                                                                                                   const discipline_id_type& discipline_id) const
{
    vector<expertise_allocation_proposal_vote_api_obj> results;
    const auto& expertise_allocation_proposal_vote_service = _db.obtain_service<chain::dbs_expertise_allocation_proposal>();

    const auto& proposal_votes
        = expertise_allocation_proposal_vote_service.get_votes_by_voter_and_discipline_id(voter, discipline_id);

    for (const chain::expertise_allocation_proposal_vote_object& proposal_vote : proposal_votes)
        results.push_back(proposal_vote);

    return results;
}

vector<expertise_allocation_proposal_vote_api_obj> database_api::get_expertise_allocation_proposal_votes_by_voter(const account_name_type& voter) const
{
    return my->_db.with_read_lock([&]() { return my->get_expertise_allocation_proposal_votes_by_voter(voter); });
}

vector<expertise_allocation_proposal_vote_api_obj> database_api_impl::get_expertise_allocation_proposal_votes_by_voter(const account_name_type& voter) const
{
    vector<expertise_allocation_proposal_vote_api_obj> results;
    const auto& expertise_allocation_proposal_vote_service = _db.obtain_service<chain::dbs_expertise_allocation_proposal>();

    const auto& proposal_votes = expertise_allocation_proposal_vote_service.get_votes_by_voter(voter);

    for (const chain::expertise_allocation_proposal_vote_object& proposal_vote : proposal_votes)
        results.push_back(proposal_vote);

    return results;
}

fc::optional<vesting_balance_api_obj> database_api::get_vesting_balance_by_id(const vesting_balance_id_type& vesting_balance_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_vesting_balance_by_id(vesting_balance_id); });
}

fc::optional<vesting_balance_api_obj> database_api_impl::get_vesting_balance_by_id(const vesting_balance_id_type& vesting_balance_id) const
{
    fc::optional<vesting_balance_api_obj> result;

    const auto& vesting_balance_service = _db.obtain_service<chain::dbs_vesting_balance>();
    const auto& vesting_balance_opt = vesting_balance_service.get_vesting_balance_if_exists(vesting_balance_id);

    if (vesting_balance_opt.valid())
    {
        result = vesting_balance_api_obj(*vesting_balance_opt);
    }

    return result;
}

vector<vesting_balance_api_obj>
database_api::get_vesting_balance_by_owner(const account_name_type &owner) const
{
    return my->_db.with_read_lock([&]() { return my->get_vesting_balance_by_owner(owner); });
}

vector<vesting_balance_api_obj>
database_api_impl::get_vesting_balance_by_owner(const account_name_type &owner) const
{
    vector<vesting_balance_api_obj> results;
    const auto& vesting_balance_service = _db.obtain_service<chain::dbs_vesting_balance>();

    const auto& total_vesting_balance = vesting_balance_service.get_vesting_balance_by_owner(owner);

    for (const chain::vesting_balance_object& vesting_balance : total_vesting_balance)
    {
        results.push_back(vesting_balance);
    }

    return results;
}

fc::optional<grant_application_api_obj> database_api::get_grant_application(const grant_application_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_grant_application(id); });
}

fc::optional<grant_application_api_obj> database_api_impl::get_grant_application(const grant_application_id_type& id) const
{
    fc::optional<grant_application_api_obj> result;

    const auto& grant_application_service = _db.obtain_service<chain::dbs_grant_application>();
    const auto& opt = grant_application_service.get_grant_application_if_exists(id);

    if (opt.valid())
    {
        result = grant_application_api_obj(*opt);
    }

    return result;
}

vector<grant_application_api_obj> database_api::get_grant_applications_by_funding_opportunity_number(const std::string& funding_opportunity_number) const
{
    return my->_db.with_read_lock([&]() { return my->get_grant_applications_by_funding_opportunity_number(funding_opportunity_number); });
}

vector<grant_application_api_obj> database_api_impl::get_grant_applications_by_funding_opportunity_number(const std::string& funding_opportunity_number) const
{
    vector<grant_application_api_obj> results;
    const auto& grant_application_service = _db.obtain_service<chain::dbs_grant_application>();
    const auto& grant_applications = grant_application_service.get_grant_applications_by_funding_opportunity_number(funding_opportunity_number);

    for (const chain::grant_application_object& grant_application : grant_applications)
        results.push_back(grant_application);

    return results;
}

vector<grant_application_api_obj> database_api::get_grant_applications_by_research_id(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_grant_applications_by_research_id(research_id); });
}

vector<grant_application_api_obj>
database_api_impl::get_grant_applications_by_research_id(const research_id_type& research_id) const
{
    vector<grant_application_api_obj> results;
    const auto& grant_application_service = _db.obtain_service<chain::dbs_grant_application>();
    const auto& grant_applications = grant_application_service.get_grant_applications_by_research_id(research_id);

    for (const chain::grant_application_object& grant_application : grant_applications)
        results.push_back(grant_application);

    return results;
}

fc::optional<funding_opportunity_api_obj> database_api::get_funding_opportunity_announcement(const funding_opportunity_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_funding_opportunity_announcement(id); });
}

fc::optional<funding_opportunity_api_obj> database_api_impl::get_funding_opportunity_announcement(const funding_opportunity_id_type& id) const
{
    fc::optional<funding_opportunity_api_obj> result;

    const auto& funding_opportunity_service = _db.obtain_service<chain::dbs_funding_opportunity>();
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
    const auto& funding_opportunity_service = _db.obtain_service<chain::dbs_funding_opportunity>();
    const auto& opt = funding_opportunity_service.get_funding_opportunity_announcement_if_exists(funding_opportunity_number);
    
    if (opt.valid())
    {
        result = funding_opportunity_api_obj(*opt);
    }

    return result;
}

vector<funding_opportunity_api_obj> database_api::get_funding_opportunity_announcements_by_organization(const account_id_type& research_group_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_funding_opportunity_announcements_by_organization(research_group_id); });
}

vector<funding_opportunity_api_obj> database_api_impl::get_funding_opportunity_announcements_by_organization(const account_id_type& research_group_id) const
{
    vector<funding_opportunity_api_obj> results;
    const auto& funding_opportunity_service = _db.obtain_service<chain::dbs_funding_opportunity>();
    const auto& funding_opportunities = funding_opportunity_service.get_funding_opportunity_announcements_by_organization(research_group_id);

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
    const auto& funding_opportunity_service = _db.obtain_service<chain::dbs_funding_opportunity>();
    
    const auto& funding_opportunities = funding_opportunity_service.get_funding_opportunity_announcements_listing(page, limit);
    for (auto& funding_opportunity : funding_opportunities) 
    {
        results.push_back(funding_opportunity_api_obj(funding_opportunity));
    }
    return results;
}

std::map<discipline_id_type, share_type> database_api::calculate_research_eci(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() {
        const auto& research_service = my->_db.obtain_service<chain::dbs_research>();
        return research_service.get_eci_evaluation(research_id);
    });
}

std::map<discipline_id_type, share_type> database_api::calculate_research_content_eci(const research_content_id_type& research_content_id) const
{
    return my->_db.with_read_lock([&]() {
        const auto& research_content_service = my->_db.obtain_service<chain::dbs_research_content>();
        return research_content_service.get_eci_evaluation(research_content_id);
    });
}

std::map<discipline_id_type, share_type> database_api::calculate_review_weight(const review_id_type& review_id) const
{
    return my->_db.with_read_lock([&]() {
        const auto& review_service = my->_db.obtain_service<chain::dbs_review>();
        return review_service.get_eci_weight(review_id);
    });
}

fc::optional<asset_api_obj> database_api::get_asset(const asset_id_type& id) const
{
    return my->_db.with_read_lock([&]() { return my->get_asset(id); });
}

fc::optional<asset_api_obj> database_api_impl::get_asset(const asset_id_type& id) const
{
    fc::optional<asset_api_obj> result;

    const auto& asset_service = _db.obtain_service<chain::dbs_asset>();
    const auto& opt = asset_service.get_asset_if_exists(id);

    if (opt.valid())
    {
        result = asset_api_obj(*opt);
    }

    return result;
}

fc::optional<asset_api_obj> database_api::get_asset_by_symbol(const std::string& symbol) const
{
    return my->_db.with_read_lock([&]() { return my->get_asset_by_symbol(symbol); });
}

fc::optional<asset_api_obj> database_api_impl::get_asset_by_symbol(const std::string& symbol) const
{
    fc::optional<asset_api_obj> result;

    const auto& asset_service = _db.obtain_service<chain::dbs_asset>();
    const auto& opt = asset_service.get_asset_by_string_symbol_if_exists(symbol);

    if (opt.valid())
    {
        result = asset_api_obj(*opt);
    }

    return result;
}

vector<asset_api_obj> database_api::get_assets_by_issuer(const account_name_type& issuer) const
{
    return my->_db.with_read_lock([&]() { return my->get_assets_by_issuer(issuer); });
}

vector<asset_api_obj> database_api_impl::get_assets_by_issuer(const account_name_type& issuer) const
{
    vector<asset_api_obj> results;

    const auto& asset_service = _db.obtain_service<chain::dbs_asset>();
    const auto& assets = asset_service.get_assets_by_issuer(issuer);

    for (auto& asset_o : assets)
    {
        results.push_back(asset_api_obj(asset_o));
    }

    return results;
}

vector<asset_api_obj> database_api::lookup_assets(const string& lower_bound_symbol, uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_API_BULK_FETCH_LIMIT);
    return my->_db.with_read_lock([&]() { return my->lookup_assets(lower_bound_symbol, limit); });
}

vector<asset_api_obj> database_api_impl::lookup_assets(const string& lower_bound_symbol, uint32_t limit) const
{
    const auto& assets_service = _db.obtain_service<chain::dbs_asset>();

    vector<asset_api_obj> result;
    const auto& assets = assets_service.lookup_assets(lower_bound_symbol, limit);

    for (const asset_object& asset : assets)
    {
        result.push_back(asset_api_obj(asset));
    }

    return result;
}

vector<asset_api_obj> database_api::get_assets_by_type(const uint8_t& type) const
{
    return my->_db.with_read_lock([&]() { return my->get_assets_by_type(type); });
}

vector<asset_api_obj> database_api_impl::get_assets_by_type(const uint8_t& type) const
{
    const auto& assets_service = _db.obtain_service<chain::dbs_asset>();

    const asset_type enum_val = static_cast<asset_type>(type);

    vector<asset_api_obj> result;
    const auto& assets = assets_service.get_assets_by_type(enum_val);

    for (const asset_object& asset : assets)
    {
        result.push_back(asset_api_obj(asset));
    }

    return result;
}

vector<account_balance_api_obj> database_api::get_account_assets_balances(const account_name_type& owner) const
{
    return my->_db.with_read_lock([&]() { return my->get_account_assets_balances(owner); });
}

vector<account_balance_api_obj> database_api_impl::get_account_assets_balances(const account_name_type& owner) const
{
    vector<account_balance_api_obj> results;
    const auto& account_balance_service = _db.obtain_service<chain::dbs_account_balance>();

    auto account_balances = account_balance_service.get_account_balances_by_owner(owner);

    for (const chain::account_balance_object& account_balance : account_balances)
        results.push_back(account_balance);

    return results;
}

vector<account_balance_api_obj> database_api::get_accounts_asset_balances_by_asset(const string& symbol) const
{
    return my->_db.with_read_lock([&]() { return my->get_accounts_asset_balances_by_asset(symbol); });
}

vector<account_balance_api_obj> database_api_impl::get_accounts_asset_balances_by_asset(const string& symbol) const
{
    vector<account_balance_api_obj> results;
    const auto& account_balance_service = _db.obtain_service<chain::dbs_account_balance>();

    auto account_balances = account_balance_service.get_accounts_balances_by_symbol(symbol);

    for (const chain::account_balance_object& account_balance : account_balances)
        results.push_back(account_balance);

    return results;
}


fc::optional<account_balance_api_obj> database_api::get_account_asset_balance(const account_name_type& owner, const string& symbol) const
{
    return my->_db.with_read_lock([&]() { return my->get_account_asset_balance(owner, symbol); });
}

fc::optional<account_balance_api_obj> database_api_impl::get_account_asset_balance(const account_name_type& owner, const string& symbol) const
{
    fc::optional<account_balance_api_obj> result;

    const auto& account_balance_service = _db.obtain_service<chain::dbs_account_balance>();
    const auto& opt = account_balance_service.get_account_balance_by_owner_and_asset_if_exists(owner, symbol);

    if (opt.valid())
    {
        result = account_balance_api_obj(*opt);
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

    const auto& disicpline_supply_service = _db.obtain_service<chain::dbs_discipline_supply>();
    const auto& opt = disicpline_supply_service.get_discipline_supply_if_exists(id);

    if (opt.valid())
    {
        result = discipline_supply_api_obj(*opt);
    }

    return result;
}

fc::optional<award_api_obj> database_api::get_award(const string& award_number) const
{
    return my->_db.with_read_lock([&]() { return my->get_award(award_number); });
}

fc::optional<award_api_obj> database_api_impl::get_award(const string& award_number) const
{
    const auto& awards_service = _db.obtain_service<chain::dbs_award>();

    fc::optional<award_api_obj> result;
    const auto& opt = awards_service.get_award_if_exists(award_number);

    if (opt.valid())
    {   
        vector<award_recipient_api_obj> awardees_list;
        const auto& awardees = awards_service.get_award_recipients_by_award(award_number);
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
    const auto& awards_service = _db.obtain_service<chain::dbs_award>();

    vector<award_api_obj> results;
    const auto& awards = awards_service.get_awards_by_funding_opportunity(funding_opportunity_number);
    for (auto& wrap : awards)
    {
        const auto& award = wrap.get();
        vector<award_recipient_api_obj> awardees_list;
        const auto& awardees = awards_service.get_award_recipients_by_award(award.award_number);
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
    const auto& awards_service = _db.obtain_service<chain::dbs_award>();

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
    const auto& awards_service = _db.obtain_service<chain::dbs_award>();

    vector<award_recipient_api_obj> results;
    const auto& awardees = awards_service.get_award_recipients_by_award(award_number);
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
    const auto& awards_service = _db.obtain_service<chain::dbs_award>();

    vector<award_recipient_api_obj> results;
    const auto& awardees = awards_service.get_award_recipients_by_account(account);
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
    const auto& awards_service = _db.obtain_service<chain::dbs_award>();
    vector<award_recipient_api_obj> results;

    const auto& awardees = awards_service.get_award_recipients_by_funding_opportunity(number);
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
    const auto& awards_service = _db.obtain_service<chain::dbs_award>();

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
    const auto& awards_service = _db.obtain_service<chain::dbs_award>();

    vector<award_withdrawal_request_api_obj> results;
    const auto& withdrawal_requests = awards_service.get_award_withdrawal_requests_by_award(award_number);
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
    const auto& awards_service = _db.obtain_service<chain::dbs_award>();

    vector<award_withdrawal_request_api_obj> results;
    const auto& withdrawal_requests = awards_service.get_award_withdrawal_requests_by_award_and_subaward(award_number, subaward_number);
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
    const auto& awards_service = _db.obtain_service<chain::dbs_award>();

    vector<award_withdrawal_request_api_obj> results;
    const auto& withdrawal_requests = awards_service.get_award_withdrawal_requests_by_award_and_status(award_number, status);
    for (auto& wrap : withdrawal_requests)
    {
        const auto& withdrawal_request = wrap.get();
        results.push_back(withdrawal_request);
    }

    return results;
}

fc::optional<nda_contract_api_obj> database_api::get_research_nda(const external_id_type& external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_nda(external_id); });
}

fc::optional<nda_contract_api_obj> database_api_impl::get_research_nda(const external_id_type& external_id) const
{
    const auto& nda_contract_service = _db.obtain_service<chain::dbs_nda_contract>();

    fc::optional<nda_contract_api_obj> result;
    const auto& opt = nda_contract_service.get_research_nda_if_exists(external_id);

    if (opt.valid())
    {
        result = nda_contract_api_obj(*opt);
    }

    return result;
}

vector<nda_contract_api_obj> database_api::get_research_nda_by_creator(const account_name_type& creator) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_nda_by_creator(creator); });
}

vector<nda_contract_api_obj> database_api_impl::get_research_nda_by_creator(const account_name_type& creator) const
{
    const auto& contract_service = _db.obtain_service<chain::dbs_nda_contract>();

    vector<nda_contract_api_obj> results;
    const auto& contracts = contract_service.get_research_nda_by_creator(creator);
    for (auto& wrap : contracts)
    {
        const auto& contract = wrap.get();
        results.push_back(contract);
    }

    return results;
}


vector<nda_contract_api_obj>
database_api::get_research_nda_by_hash(const std::string &hash) const
{
    return my->_db.with_read_lock([&]() {
        return my->get_research_nda_by_hash(hash);
    });
}

vector<nda_contract_api_obj>
database_api_impl::get_research_nda_by_hash(const std::string& hash) const
{
    const auto& contract_service = _db.obtain_service<chain::dbs_nda_contract>();

    vector<nda_contract_api_obj> results;
    const auto& contracts = contract_service.get_research_nda_by_hash(hash);
    for (auto& wrap : contracts)
    {
        const auto& contract = wrap.get();
        results.push_back(contract);
    }

    return results;
}


vector<nda_contract_api_obj> database_api::get_research_nda_by_research(const external_id_type& external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_research_nda_by_research(external_id); });
}

vector<nda_contract_api_obj> database_api_impl::get_research_nda_by_research(const external_id_type& external_id) const
{
    const auto& contract_service = _db.obtain_service<chain::dbs_nda_contract>();

    vector<nda_contract_api_obj> results;
    const auto& contracts = contract_service.get_research_nda_by_research(external_id);
    for (auto& wrap : contracts)
    {
        const auto& contract = wrap.get();
        results.push_back(contract);
    }

    return results;
}


fc::optional<nda_contract_file_access_api_obj>
database_api::get_nda_contract_content_access_request(const external_id_type& external_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_nda_contract_content_access_request(external_id); });
}

fc::optional<nda_contract_file_access_api_obj>
database_api_impl::get_nda_contract_content_access_request(const external_id_type& external_id) const
{
    const auto& contract_requests_service = _db.obtain_service<chain::dbs_nda_contract_requests>();

    fc::optional<nda_contract_file_access_api_obj> result;
    const auto& opt = contract_requests_service.get_content_access_request_if_exists(external_id);

    if (opt.valid())
    {
        result = nda_contract_file_access_api_obj(*opt);
    }

    return result;
}

vector<nda_contract_file_access_api_obj>
database_api::get_nda_contract_content_access_requests_by_nda(const external_id_type& nda_external_id) const
{
    return my->_db.with_read_lock([&]() {
        return my->get_nda_contract_content_access_requests_by_nda(nda_external_id);
    });
}

vector<nda_contract_file_access_api_obj>
database_api_impl::get_nda_contract_content_access_requests_by_nda(const external_id_type& nda_external_id) const
{
    const auto& contract_requests_service = _db.obtain_service<chain::dbs_nda_contract_requests>();

    vector<nda_contract_file_access_api_obj> results;
    const auto& requests = contract_requests_service.get_content_access_requests_by_nda(nda_external_id);
    for (auto& wrap : requests)
    {
        const auto& request = wrap.get();
        results.push_back(request);
    }

    return results;
}

vector<nda_contract_file_access_api_obj>
database_api::get_nda_contract_content_access_requests_by_requester(const account_name_type& requester) const
{
    return my->_db.with_read_lock([&]() {
        return my->get_nda_contract_content_access_requests_by_requester(requester);
    });
}

vector<nda_contract_file_access_api_obj>
database_api_impl::get_nda_contract_content_access_requests_by_requester(const account_name_type& requester) const
{
    const auto& contract_requests_service = _db.obtain_service<chain::dbs_nda_contract_requests>();

    vector<nda_contract_file_access_api_obj> results;
    const auto& requests = contract_requests_service.get_content_access_requests_by_requester(requester);
    for (auto& wrap : requests)
    {
        const auto& request = wrap.get();
        results.push_back(request);
    }

    return results;
}

fc::optional<contract_agreement_api_obj> database_api::get_contract_agreement(const external_id_type& id) const
{
    return my->_db.with_read_lock([&]() {
        return my->get_contract_agreement(id);
    });
}

fc::optional<contract_agreement_api_obj> database_api_impl::get_contract_agreement(const external_id_type& id) const
{
    const auto& service = _db.obtain_service<chain::dbs_contract_agreement>();

    fc::optional<contract_agreement_api_obj> result;
    const auto& opt = service.get_if_exists(id);

    if (opt.valid())
    {
        result = contract_agreement_api_obj(*opt);
    }

    return result;
}

vector<contract_agreement_api_obj> database_api::get_contract_agreement_by_creator(
        const account_name_type& creator) const
{
    return my->_db.with_read_lock([&]() {
        return my->get_contract_agreement_by_creator(creator);
    });
}

vector<contract_agreement_api_obj> database_api_impl::get_contract_agreement_by_creator(
        const account_name_type& creator) const
{
    const auto& service = _db.obtain_service<chain::dbs_contract_agreement>();

    vector<contract_agreement_api_obj> results;
    const auto& contracts = service.get_by_creator(creator);
    for (auto& wrap : contracts)
    {
        const auto& contract = wrap.get();
        results.push_back(contract);
    }

    return results;
}

} // namespace app
} // namespace deip
