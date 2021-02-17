#pragma once
#include <deip/app/state.hpp>

#include <deip/chain/database/database.hpp>
#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/schema/deip_object_types.hpp>
//#include <deip/chain/history_object.hpp>
#include <deip/chain/services/dbs_proposal.hpp>

#include <deip/witness/witness_plugin.hpp>

#include <fc/api.hpp>
#include <fc/optional.hpp>
#include <fc/variant_object.hpp>

#include <fc/network/ip.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace deip {
namespace app {

using namespace deip::chain;
using namespace deip::protocol;
using namespace std;

struct api_context;

struct scheduled_hardfork
{
    hardfork_version hf_version;
    fc::time_point_sec live_time;
};

struct withdraw_route
{
    string from_account;
    string to_account;
    uint16_t percent;
    bool auto_common_token;
};

enum withdraw_route_type
{
    incoming,
    outgoing,
    all
};

class database_api_impl;
/**
 * @brief The database_api class implements the RPC API for the chain database.
 *
 * This API exposes accessors on the database which query state tracked by a blockchain validating node. This API is
 * read-only; all modifications to the database must be performed via transactions. Transactions are broadcast via
 * the @ref network_broadcast_api.
 */
class database_api
{
public:
    database_api(const deip::app::api_context& ctx);
    ~database_api();

    ///////////////////
    // Subscriptions //
    ///////////////////

    void set_block_applied_callback(std::function<void(const variant& block_header)> cb);

    /**
     *  This API is a short-cut for returning all of the state required for a particular URL
     *  with a single query.
     */
    state get_state(string path) const;

    vector<account_name_type> get_active_witnesses() const;

    /////////////////////////////
    // Blocks and transactions //
    /////////////////////////////

    /**
     * @brief Retrieve a full, signed block
     * @param block_num Height of the block to be returned
     * @return the referenced block, or null if no matching block was found
     */
    optional<signed_block_api_obj> get_block(uint32_t block_num) const;

    /////////////
    // Globals //
    /////////////

    /**
     * @brief Retrieve compile-time constants
     */
    fc::variant_object get_config() const;

    /**
     * @brief Return a JSON description of object representations
     */
    std::string get_schema() const;

    /**
     * @brief Retrieve the current @ref dynamic_global_property_object
     */
    dynamic_global_property_api_obj get_dynamic_global_properties() const;
    chain_properties get_chain_properties() const;
    witness_schedule_api_obj get_witness_schedule() const;
    hardfork_version get_hardfork_version() const;
    scheduled_hardfork get_next_scheduled_hardfork() const;


    //////////////
    // Accounts //
    //////////////

    /**
     * @brief Get a list of accounts by name
     * @param names Names of the accounts to retrieve
     * @return The accounts holding the provided names
     *
     * This function has semantics identical to @ref get_objects
     */
    vector<optional<account_api_obj>> get_accounts(const set<string>& names) const;

    /**
     * @brief Get names and IDs for registered accounts
     * @param lower_bound_name Lower bound of the first name to return
     * @param limit Maximum number of results to return -- must not exceed 1000
     * @return Vector of accounts
     */
    vector<account_api_obj> lookup_accounts(const string& lower_bound_name, uint32_t limit) const;

    /**
     * @brief Get the total number of accounts registered with the blockchain
     */
    uint64_t get_account_count() const;

    vector<discipline_supply_api_obj> get_discipline_supplies(const set<string>& account_names) const;

    set<string> lookup_discipline_supply_grantors(const string& lower_bound_name, uint32_t limit) const;

    vector<owner_authority_history_api_obj> get_owner_history(string account) const;

    optional<account_recovery_request_api_obj> get_recovery_request(string account) const;

    vector<withdraw_route> get_withdraw_routes(string account, withdraw_route_type type = outgoing) const;

    optional<account_bandwidth_api_obj> get_account_bandwidth(string account, witness::bandwidth_type type) const;

    vector<account_api_obj> get_accounts_by_expert_discipline(const discipline_id_type& discipline_id, uint32_t from, uint32_t limit) const;

    ///////////////
    // Witnesses //
    ///////////////

    /**
     * @brief Get a list of witnesses by ID
     * @param witness_ids IDs of the witnesses to retrieve
     * @return The witnesses corresponding to the provided IDs
     *
     * This function has semantics identical to @ref get_objects
     */
    vector<optional<witness_api_obj>> get_witnesses(const vector<witness_id_type>& witness_ids) const;

    /**
     * @brief Get the witness owned by a given account
     * @param account The name of the account whose witness should be retrieved
     * @return The witness object, or null if the account does not have a witness
     */
    fc::optional<witness_api_obj> get_witness_by_account(string account_name) const;

    /**
     *  This method is used to fetch witnesses with pagination.
     *
     *  @return an array of `count` witnesses sorted by total votes after witness `from` with at most `limit' results.
     */
    vector<witness_api_obj> get_witnesses_by_vote(string from, uint32_t limit) const;

    /**
     * @brief Get names and IDs for registered witnesses
     * @param lower_bound_name Lower bound of the first name to return
     * @param limit Maximum number of results to return -- must not exceed 1000
     * @return Map of witness names to corresponding IDs
     */
    set<account_name_type> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit) const;

    /**
     * @brief Get the total number of witnesses registered with the blockchain
     */
    uint64_t get_witness_count() const;

    ////////////
    // Market //
    ////////////

    ////////////////////////////
    // Authority / validation //
    ////////////////////////////

    /// @brief Get a hexdump of the serialized binary form of a transaction
    std::string get_transaction_hex(const signed_transaction& trx) const;

    /**
     * @return true of the @ref trx has all of the required signatures, otherwise throws an exception
     */
    bool verify_authority(const signed_transaction& trx) const;

    /*
     * @return true if the signers have enough authority to authorize an account
     */
    bool verify_account_authority(const string& name_or_id, const flat_set<public_key_type>& signers) const;

    /**
     *  if permlink is "" then it will return all votes for author
     */
    vector<vote_state> get_active_votes(string author, string permlink) const;
    vector<account_vote> get_account_votes(string voter) const;

    ///@}

    /**
     *  For each of these filters:
     *     Get root content...
     *     Get any content...
     *     Get root content in category..
     *     Get any content in category...
     *

     *
     *  Return content (comments)
     *     Pending Payout Amount
     *     Pending Payout Time
     *     Creation Date
     *
     */
    ///@{

    /////////////////
    // Disciplines //
    /////////////////
    vector<discipline_api_obj> lookup_disciplines(const discipline_id_type& lower_bound, uint32_t limit) const;
    fc::optional<discipline_api_obj> get_discipline(const external_id_type& external_id) const;
    vector<discipline_api_obj> get_disciplines_by_parent(const external_id_type& parent_external_id) const;

    ////////////////////
    // Research group //
    ////////////////////
    fc::optional<research_group_api_obj> get_research_group(const account_name_type& account) const;
    vector<research_group_api_obj> get_research_groups(const set<external_id_type>& ids) const;
    vector<research_group_api_obj> get_research_groups_by_member(const account_name_type& member) const;
    /* [DEPRECATED] */ fc::optional<research_group_api_obj> get_research_group_by_id(const research_group_id_type research_group_id) const;
    /* [DEPRECATED] */ fc::optional<research_group_api_obj> get_research_group_by_permlink(const string& permlink) const;
    /* [DEPRECATED] */ bool check_research_group_existence_by_permlink(const string& name) const;
    vector<research_group_api_obj> lookup_research_groups(const research_group_id_type& lower_bound, uint32_t limit) const;

    ////////////////
    // Researches //
    ////////////////
    fc::optional<research_api_obj> get_research(const external_id_type& id) const;
    vector<research_api_obj> get_researches(const set<external_id_type>& ids) const;
    fc::optional<research_api_obj> get_research_by_id(const research_id_type& internal_id) const;
    /* [DEPRECATED] */ fc::optional<research_api_obj> get_research_by_permlink(const research_group_id_type& research_group_id, const string& permlink) const;
    /* [DEPRECATED] */ fc::optional<research_api_obj> get_research_by_absolute_permlink(const string& research_group_permlink, const string& research_permlink) const;
    /* [DEPRECATED] */ bool check_research_existence_by_permlink(const external_id_type& research_group_external_id, const string& title) const;
    vector<research_api_obj> get_researches_by_research_group(const external_id_type& external_id) const;
    vector<research_api_obj> get_researches_by_research_group_member(const account_name_type& member) const;
    vector<research_api_obj> lookup_researches(const research_id_type& lower_bound, uint32_t limit) const;

    //////////////////////
    // Research Content //
    //////////////////////
    fc::optional<research_content_api_obj> get_research_content(const external_id_type& id) const;
    vector<research_content_api_obj> get_research_contents(const set<external_id_type>& ids) const;
    fc::optional<research_content_api_obj> get_research_content_by_id(const research_content_id_type& internal_id) const;
    /* [DEPRECATED] */ fc::optional<research_content_api_obj> get_research_content_by_permlink(const research_id_type& research_id, const string& permlink) const;
    /* [DEPRECATED] */ fc::optional<research_content_api_obj> get_research_content_by_absolute_permlink(const string& research_group_permlink, const string& research_permlink, const string& research_content_permlink) const;
    /* [DEPRECATED] */ bool check_research_content_existence_by_permlink(const external_id_type& research_external_id, const string& title) const;
    vector<research_content_api_obj> get_research_contents_by_research(const external_id_type& external_id) const;
    vector<research_content_api_obj> get_research_content_by_type(const research_id_type& research_id, const research_content_type& type) const;
    vector<research_content_api_obj> lookup_research_contents(const research_content_id_type& lower_bound, uint32_t limit) const;

    ///////////////////////
    // Research licenses //
    ///////////////////////
    fc::optional<research_license_api_obj> get_research_license(const external_id_type& external_id) const;
    vector<research_license_api_obj> get_research_licenses(const set<external_id_type>& ids) const;
    vector<research_license_api_obj> get_research_licenses_by_licensee(const account_name_type& licensee) const;
    vector<research_license_api_obj> get_research_licenses_by_licenser(const account_name_type& licenser) const;
    vector<research_license_api_obj> get_research_licenses_by_research(const external_id_type& research_external_id) const;
    vector<research_license_api_obj> get_research_licenses_by_licensee_and_research(const account_name_type& licensee, const external_id_type& research_external_id) const;
    vector<research_license_api_obj> get_research_licenses_by_licensee_and_licenser(const account_name_type& licensee, const account_name_type& licenser) const;

    ///////////////////
    // Expert Tokens //
    ///////////////////
    fc::optional<expert_token_api_obj> get_expert_token(const expert_token_id_type id) const;
    vector<expert_token_api_obj> get_expert_tokens_by_account_name(const account_name_type account) const;
    vector<expert_token_api_obj> get_expert_tokens_by_discipline(const external_id_type& discipline_external_id) const;
    fc::optional<expert_token_api_obj> get_common_token_by_account_name(const account_name_type account_name) const;

    ////////////////////
    // Proposal       //
    ////////////////////
    fc::optional<proposal_api_obj> get_proposal(const external_id_type& external_id) const;
    vector<proposal_api_obj> get_proposals_by_creator(const account_name_type& creator) const;


    /////////////////////////////////
    // Research group tokens       //
    /////////////////////////////////
    vector<research_group_token_api_obj> get_research_group_tokens_by_account(const account_name_type& member) const;
    vector<research_group_token_api_obj> get_research_group_membership_tokens(const external_id_type& external_id) const;
    vector<research_group_token_api_obj> get_research_group_tokens_by_research_group(const research_group_id_type& internal_id) const;
    fc::optional<research_group_token_api_obj> get_research_group_token_by_account_and_research_group_id(const account_name_type& member, const research_group_id_type& internal_id) const;

    /////////////////////////
    // Research token sale //
    /////////////////////////
    fc::optional<research_token_sale_api_obj> get_research_token_sale(const external_id_type& external_id) const;
    fc::optional<research_token_sale_api_obj> get_research_token_sale_by_id(const research_token_sale_id_type& research_token_sale_id) const;
    vector<research_token_sale_api_obj> get_research_token_sales_by_research(const external_id_type& research_external_id) const;
    vector<research_token_sale_api_obj> get_research_token_sales_by_research_id(const research_id_type& research_id) const;
    vector<research_token_sale_api_obj> get_research_token_sales(const uint32_t& from, uint32_t limit) const;
    vector<research_token_sale_contribution_api_obj> get_research_token_sale_contributions_by_research_token_sale(const external_id_type& token_sale_external_id) const;
    vector<research_token_sale_contribution_api_obj> get_research_token_sale_contributions_by_research_token_sale_id(const research_token_sale_id_type& research_token_sale_id) const;
    vector<research_token_sale_contribution_api_obj> get_research_token_sale_contributions_by_contributor(const account_name_type& owner) const;

    ///////////////////////////////////
    // Research discipline relation  //
    ///////////////////////////////////
    vector<discipline_api_obj> get_disciplines_by_research(const research_id_type& research_id) const;

    /////////////////////////////
    // Expertise contributions //
    /////////////////////////////
    fc::optional<expertise_contribution_object_api_obj> get_expertise_contribution_by_research_content_and_discipline(const research_content_id_type& research_content_id, const discipline_id_type& discipline_id) const;
    vector<expertise_contribution_object_api_obj> get_expertise_contributions_by_research(const research_id_type& research_id) const;
    vector<expertise_contribution_object_api_obj> get_expertise_contributions_by_research_and_discipline(const research_id_type& research_id, const discipline_id_type& discipline_id) const;
    vector<expertise_contribution_object_api_obj> get_expertise_contributions_by_research_content(const research_content_id_type& research_content_id) const;

    ///////////////////////////////
    // Research Reviews          //
    ///////////////////////////////
    fc::optional<review_api_obj> get_review(const external_id_type& external_id) const;
    vector<review_api_obj> get_reviews(const set<external_id_type>& ids) const;
    fc::optional<review_api_obj> get_review_by_id(const review_id_type& review_id) const;
    vector<review_api_obj> get_reviews_by_research(const external_id_type& research_external_id) const;
    vector<review_api_obj> get_reviews_by_research_content(const external_id_type& research_content_external_id) const;
    vector<review_api_obj> get_reviews_by_author(const account_name_type& author) const;

    ///////////////////////////////
    // Grant Application Reviews //
    ///////////////////////////////
    vector<grant_application_review_api_obj> get_reviews_by_grant_application(const grant_application_id_type& grant_application_id) const;

    /////////////////////
    // Research token ///
    /////////////////////
    fc::optional<research_token_api_obj> get_research_token_by_id(const research_token_id_type& research_token_id) const;
    vector<research_token_api_obj> get_research_tokens_by_account_name(const account_name_type &account_name) const;
    vector<research_token_api_obj> get_research_tokens_by_research_id(const research_id_type &research_id) const;
    fc::optional<research_token_api_obj> get_research_token_by_account_name_and_research_id(const account_name_type &account_name, const research_id_type &research_id) const;


    /////////////////////////
    // Review vote object ///
    /////////////////////////

    vector<review_vote_api_obj> get_review_votes_by_voter(const account_name_type& voter) const;
    vector<review_vote_api_obj> get_review_votes_by_review_id(const review_id_type& review_id) const;
    vector<review_vote_api_obj> get_review_votes_by_review(const external_id_type& review_external_id) const;

    //////////////////////////////////////////
    // Expertise allocation proposal object///
    /////////////////////////////////////////

    fc::optional<expertise_allocation_proposal_api_obj> get_expertise_allocation_proposal_by_id(const expertise_allocation_proposal_id_type& id) const;
    vector<expertise_allocation_proposal_api_obj> get_expertise_allocation_proposals_by_claimer(const account_name_type& claimer) const;
    fc::optional<expertise_allocation_proposal_api_obj> get_expertise_allocation_proposals_by_claimer_and_discipline(const account_name_type& claimer,
                                                                                                       const discipline_id_type& discipline_id) const;
    vector<expertise_allocation_proposal_api_obj> get_expertise_allocation_proposals_by_discipline(const discipline_id_type& discipline_id) const;

    ////////////////////////////////////////////////
    // Expertise allocation proposal vote object///
    ///////////////////////////////////////////////

    fc::optional<expertise_allocation_proposal_vote_api_obj> get_expertise_allocation_proposal_vote_by_id(const expertise_allocation_proposal_vote_id_type& id) const;
    vector<expertise_allocation_proposal_vote_api_obj> get_expertise_allocation_proposal_votes_by_expertise_allocation_proposal_id
                                                                                          (const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id) const;
    fc::optional<expertise_allocation_proposal_vote_api_obj> get_expertise_allocation_proposal_vote_by_voter_and_expertise_allocation_proposal_id(const account_name_type& voter,
                                                                                                                                                  const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id) const;
    vector<expertise_allocation_proposal_vote_api_obj> get_expertise_allocation_proposal_votes_by_voter_and_discipline_id(const account_name_type& voter,
                                                                                                                         const discipline_id_type& discipline_id) const;
    vector<expertise_allocation_proposal_vote_api_obj> get_expertise_allocation_proposal_votes_by_voter(const account_name_type& voter) const;

    ///////////////////////
    // Vesting balance ///
    //////////////////////

    fc::optional<vesting_balance_api_obj> get_vesting_balance_by_id(const vesting_balance_id_type& vesting_balance_id) const;
    vector<vesting_balance_api_obj> get_vesting_balance_by_owner(const account_name_type& owner) const;


    ////////////////////////
    // Grant applications //
    ////////////////////////

    fc::optional<grant_application_api_obj> get_grant_application(const grant_application_id_type& id) const;
    vector<grant_application_api_obj> get_grant_applications_by_funding_opportunity_number(const std::string& funding_opportunity_number) const;
    vector<grant_application_api_obj> get_grant_applications_by_research_id(const research_id_type& research_id) const;


    ///////////////////////////
    // Funding opportunities //
    ///////////////////////////

    fc::optional<funding_opportunity_api_obj> get_funding_opportunity_announcement(const funding_opportunity_id_type& id) const;
    fc::optional<funding_opportunity_api_obj> get_funding_opportunity_announcement_by_number(const string& number) const;
    vector<funding_opportunity_api_obj> get_funding_opportunity_announcements_by_organization(const research_group_id_type& research_group_id) const;
    vector<funding_opportunity_api_obj> get_funding_opportunity_announcements_listing(const uint16_t& page, const uint16_t& limit) const;


    // These methods are just for ECI debugging purposes, for actual values use 'eci_per_discipline' field
    std::map<discipline_id_type, share_type> calculate_research_eci(const research_id_type& research_id) const;
    std::map<discipline_id_type, share_type> calculate_research_content_eci(const research_content_id_type& research_content_id) const;
    std::map<discipline_id_type, share_type> calculate_review_weight(const review_id_type& review_id) const;

    ////////////
    // Assets //
    ////////////

    fc::optional<asset_api_obj> get_asset(const asset_id_type& id) const;
    fc::optional<asset_api_obj> get_asset_by_symbol(const string& symbol) const;
    vector<asset_api_obj> get_assets_by_type(const uint8_t& type) const;
    vector<asset_api_obj> get_assets_by_issuer(const account_name_type& issuer) const;
    vector<asset_api_obj> lookup_assets(const string& lower_bound_symbol, uint32_t limit) const;

    //////////////////////
    // Account balances //
    //////////////////////

    fc::optional<account_balance_api_obj> get_account_asset_balance(const account_name_type& owner, const string& symbol) const;
    vector<account_balance_api_obj> get_account_assets_balances(const account_name_type& owner) const;
    vector<account_balance_api_obj> get_accounts_asset_balances_by_asset(const string& symbol) const;

    //////////////////////////
    // Discipline supplies //
    /////////////////////////

    fc::optional<discipline_supply_api_obj> get_discipline_supply(const discipline_supply_id_type& id) const;

    ////////////
    // Awards //
    ///////////

    fc::optional<award_api_obj> get_award(const string& award_number) const;
    vector<award_api_obj> get_awards_by_funding_opportunity(const string& funding_opportunity_number) const;

    fc::optional<award_recipient_api_obj> get_award_recipient(const award_recipient_id_type& id) const;
    vector<award_recipient_api_obj> get_award_recipients_by_award(const string& award_number) const;
    vector<award_recipient_api_obj> get_award_recipients_by_account(const account_name_type& awardee) const;
    vector<award_recipient_api_obj> get_award_recipients_by_funding_opportunity(const string& number) const;

    fc::optional<award_withdrawal_request_api_obj> get_award_withdrawal_request(const string& award_number, const string& payment_number) const;
    vector<award_withdrawal_request_api_obj> get_award_withdrawal_requests_by_award(const string& award_number) const;
    vector<award_withdrawal_request_api_obj> get_award_withdrawal_requests_by_award_and_subaward(const string& award_number, const string& subaward_number) const;
    vector<award_withdrawal_request_api_obj> get_award_withdrawal_requests_by_award_and_status(const string& award_number, const uint16_t& status) const;


    ///////////////////
    // NDA Contracts //
    ///////////////////

    fc::optional<nda_contract_api_obj> get_research_nda(const external_id_type& external_id) const;
    vector<nda_contract_api_obj> get_research_nda_by_creator(const account_name_type& creator) const;
    vector<nda_contract_api_obj> get_research_nda_by_hash(const string& hash) const;
    vector<nda_contract_api_obj> get_research_nda_by_research(const external_id_type& external_id) const;


    ///////////////////////////////////////////
    // NDA Contracts Content Access Requests //
    ///////////////////////////////////////////

    fc::optional<nda_contract_file_access_api_obj> get_nda_contract_content_access_request(const external_id_type& external_id) const;
    vector<nda_contract_file_access_api_obj> get_nda_contract_content_access_requests_by_nda(const external_id_type& nda_external_id) const;
    vector<nda_contract_file_access_api_obj> get_nda_contract_content_access_requests_by_requester(const account_name_type& requester) const;


    ////////////////////////////
    // Handlers - not exposed //
    ////////////////////////////
    void on_api_startup();

private:
    std::shared_ptr<database_api_impl> my;
    application& _app;

};
}
}

// clang-format off

FC_REFLECT( deip::app::scheduled_hardfork, (hf_version)(live_time) )
FC_REFLECT( deip::app::withdraw_route, (from_account)(to_account)(percent)(auto_common_token) )

FC_REFLECT_ENUM( deip::app::withdraw_route_type, (incoming)(outgoing)(all) )

FC_API(deip::app::database_api,
   // Subscriptions
   (set_block_applied_callback)

   //blocks
   (get_block)
   (get_state)

   // Globals
   (get_config)
   (get_dynamic_global_properties)
   (get_chain_properties)
   (get_witness_schedule)
   (get_hardfork_version)
   (get_next_scheduled_hardfork)

   // Accounts
   (get_accounts)
   (lookup_accounts)
   (get_account_count)
   (get_owner_history)
   (get_recovery_request)
   (get_withdraw_routes)
   (get_account_bandwidth)
   (get_accounts_by_expert_discipline)

   // Authority / validation
   (get_transaction_hex)
   (verify_authority)
   (verify_account_authority)

   // Witnesses
   (get_witnesses)
   (get_witness_by_account)
   (get_witnesses_by_vote)
   (lookup_witness_accounts)
   (get_witness_count)
   (get_active_witnesses)

    // Discipline supply
   (get_discipline_supplies)
   (lookup_discipline_supply_grantors)

   // Disciplines
   (lookup_disciplines)
   (get_discipline)
   (get_disciplines_by_parent)

   // Research group
   (get_research_group)
   (get_research_groups)
   (get_research_groups_by_member)
   (get_research_group_by_id)
   (get_research_group_by_permlink)
   (check_research_group_existence_by_permlink)
   (lookup_research_groups)

   // Research
   (get_research)
   (get_researches)
   (get_research_by_id)
   (get_research_by_permlink)
   (get_researches_by_research_group)
   (get_researches_by_research_group_member)
   (get_research_by_absolute_permlink)
   (check_research_existence_by_permlink)
   (lookup_researches)

   // Research Content
   (get_research_content)
   (get_research_contents)
   (get_research_content_by_id)
   (get_research_content_by_permlink)
   (get_research_content_by_absolute_permlink)
   (check_research_content_existence_by_permlink)
   (get_research_contents_by_research)
   (get_research_content_by_type)
   (lookup_research_contents)

   // Research license
   (get_research_license)
   (get_research_licenses)
   (get_research_licenses_by_licensee)
   (get_research_licenses_by_licenser)
   (get_research_licenses_by_research)
   (get_research_licenses_by_licensee_and_research)
   (get_research_licenses_by_licensee_and_licenser)

   // Expert Tokens
   (get_expert_token)
   (get_expert_tokens_by_account_name)
   (get_expert_tokens_by_discipline)
   (get_common_token_by_account_name)

   // Proposal
   (get_proposal)
   (get_proposals_by_creator)

   (get_research_group_tokens_by_account)
   (get_research_group_membership_tokens)
   (get_research_group_tokens_by_research_group)
   (get_research_group_token_by_account_and_research_group_id)

   // Research Token Sale
   (get_research_token_sale)
   (get_research_token_sale_by_id)
   (get_research_token_sales_by_research)
   (get_research_token_sales_by_research_id)
   (get_research_token_sales)
   (get_research_token_sale_contributions_by_research_token_sale)
   (get_research_token_sale_contributions_by_research_token_sale_id)
   (get_research_token_sale_contributions_by_contributor)

   // Research discipline relation
   (get_disciplines_by_research)

   // Total votes
   (get_expertise_contributions_by_research)
   (get_expertise_contributions_by_research_and_discipline)
   (get_expertise_contributions_by_research_content)
   (get_expertise_contribution_by_research_content_and_discipline)

   // Reviews
   (get_review)
   (get_reviews)
   (get_review_by_id)
   (get_reviews_by_research)
   (get_reviews_by_research_content)
   (get_reviews_by_author)

   // Grant Application Reviews
   (get_reviews_by_grant_application)

   // Research token
   (get_research_token_by_id)
   (get_research_tokens_by_account_name)
   (get_research_tokens_by_research_id)
   (get_research_token_by_account_name_and_research_id)

   // Review votes
   (get_review_votes_by_voter)
   (get_review_votes_by_review_id)
   (get_review_votes_by_review)

   // Expertise allocation proposal
   (get_expertise_allocation_proposal_by_id)
   (get_expertise_allocation_proposals_by_claimer)
   (get_expertise_allocation_proposals_by_claimer_and_discipline)
   (get_expertise_allocation_proposals_by_discipline)

   // Expertise allocation proposal vote
   (get_expertise_allocation_proposal_vote_by_id)
   (get_expertise_allocation_proposal_votes_by_expertise_allocation_proposal_id)
   (get_expertise_allocation_proposal_vote_by_voter_and_expertise_allocation_proposal_id)
   (get_expertise_allocation_proposal_votes_by_voter_and_discipline_id)
   (get_expertise_allocation_proposal_votes_by_voter)

   // Vesting balance
   (get_vesting_balance_by_id)
   (get_vesting_balance_by_owner)

   // Grant applications
   (get_grant_application)
   (get_grant_applications_by_funding_opportunity_number)
   (get_grant_applications_by_research_id)

   // FOA
   (get_funding_opportunity_announcement)
   (get_funding_opportunity_announcement_by_number)
   (get_funding_opportunity_announcements_by_organization)
   (get_funding_opportunity_announcements_listing)

   
   (calculate_research_eci)
   (calculate_research_content_eci)
   (calculate_review_weight)

   (get_asset)
   (get_asset_by_symbol)
   (get_assets_by_type)
   (get_assets_by_issuer)
   (lookup_assets)

   (get_account_asset_balance)
   (get_account_assets_balances)
   (get_accounts_asset_balances_by_asset)

   (get_discipline_supply)

   // Awards
   (get_award)
   (get_awards_by_funding_opportunity)

   (get_award_recipient)
   (get_award_recipients_by_award)
   (get_award_recipients_by_account)
   (get_award_recipients_by_funding_opportunity)

   (get_award_withdrawal_request)
   (get_award_withdrawal_requests_by_award)
   (get_award_withdrawal_requests_by_award_and_subaward)
   (get_award_withdrawal_requests_by_award_and_status)

   // Contracts
   (get_research_nda)
   (get_research_nda_by_creator)
   (get_research_nda_by_hash)
   (get_research_nda_by_research)

   (get_nda_contract_content_access_request)
   (get_nda_contract_content_access_requests_by_nda)
   (get_nda_contract_content_access_requests_by_requester)

)

// clang-format on
