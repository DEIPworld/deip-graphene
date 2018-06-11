#pragma once
#include <deip/app/applied_operation.hpp>
#include <deip/app/state.hpp>

#include <deip/chain/database.hpp>
#include <deip/chain/deip_objects.hpp>
#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/history_object.hpp>
#include <deip/chain/dbs_proposal.hpp>

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
     * @brief Retrieve a block header
     * @param block_num Height of the block whose header should be returned
     * @return header of the referenced block, or null if no matching block was found
     */
    optional<block_header> get_block_header(uint32_t block_num) const;

    /**
     * @brief Retrieve a full, signed block
     * @param block_num Height of the block to be returned
     * @return the referenced block, or null if no matching block was found
     */
    optional<signed_block_api_obj> get_block(uint32_t block_num) const;

    /**
     *  @brief Get sequence of operations included/generated within a particular block
     *  @param block_num Height of the block whose generated virtual operations should be returned
     *  @param only_virtual Whether to only include virtual operations in returned results (default: true)
     *  @return sequence of operations included/generated within the block
     */
    vector<applied_operation> get_ops_in_block(uint32_t block_num, bool only_virtual = true) const;

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
    reward_fund_api_obj get_reward_fund(const string& name) const;

    //////////
    // Keys //
    //////////

    vector<set<string>> get_key_references(vector<public_key_type> key) const;

    //////////////
    // Accounts //
    //////////////

    vector<extended_account> get_accounts(const vector<string>& names) const;

    /**
     *  @return all accounts that referr to the key or account id in their owner or active authorities.
     */
    vector<account_id_type> get_account_references(account_id_type account_id) const;

    /**
     * @brief Get a list of accounts by name
     * @param account_names Names of the accounts to retrieve
     * @return The accounts holding the provided names
     *
     * This function has semantics identical to @ref get_objects
     */
    vector<optional<account_api_obj>> lookup_account_names(const vector<string>& account_names) const;

    /**
     * @brief Get names and IDs for registered accounts
     * @param lower_bound_name Lower bound of the first name to return
     * @param limit Maximum number of results to return -- must not exceed 1000
     * @return Map of account names to corresponding IDs
     */
    set<string> lookup_accounts(const string& lower_bound_name, uint32_t limit) const;

    /**
     * @brief Get the total number of accounts registered with the blockchain
     */
    uint64_t get_account_count() const;

    vector<account_api_obj> get_all_accounts() const;

    vector<grant_api_obj> get_grants(const set<string>& account_names) const;

    set<string> lookup_grant_owners(const string& lower_bound_name, uint32_t limit) const;

    vector<owner_authority_history_api_obj> get_owner_history(string account) const;

    optional<account_recovery_request_api_obj> get_recovery_request(string account) const;

    vector<withdraw_route> get_withdraw_routes(string account, withdraw_route_type type = outgoing) const;

    optional<account_bandwidth_api_obj> get_account_bandwidth(string account, witness::bandwidth_type type) const;

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
    annotated_signed_transaction get_transaction(transaction_id_type trx_id) const;

    /**
     *  This API will take a partially signed transaction and a set of public keys that the owner has the ability to
     * sign for
     *  and return the minimal subset of public keys that should add signatures to the transaction.
     */
    set<public_key_type> get_required_signatures(
        const signed_transaction& trx, const flat_set<public_key_type>& available_keys) const;

    /**
     *  This method will return the set of all public keys that could possibly sign for a given transaction.  This call
     * can
     *  be used by wallets to filter their set of public keys to just the relevant subset prior to calling @ref
     * get_required_signatures
     *  to get the minimum subset.
     */
    set<public_key_type> get_potential_signatures(const signed_transaction& trx) const;

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

    /**
     *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
     *  returns operations in the range [from-limit, from]
     *
     *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
     *  @param limit - the maximum number of items that can be queried (0 to 1000], must be less than from
     */
    map<uint32_t, applied_operation> get_account_history(string account, uint64_t from, uint32_t limit) const;

    /////////////////
    // Disciplines //
    /////////////////
    vector<discipline_api_obj> get_all_disciplines() const;
    discipline_api_obj get_discipline(const discipline_id_type id) const;
    discipline_api_obj get_discipline_by_name(const discipline_name_type name) const;
    vector<discipline_api_obj> get_disciplines_by_parent_id(const discipline_id_type parent_id) const;

    ////////////////
    // Researches //
    ////////////////
    research_api_obj get_research_by_id(const research_id_type& id) const;
    research_api_obj get_research_by_permlink(const research_group_id_type& research_group_id, const string& permlink) const;
    research_api_obj get_research_by_absolute_permlink(const string& research_group_permlink, const string& research_permlink) const;
    vector<research_api_obj> get_researches_by_discipline_id(const uint64_t from, const uint32_t limit, const discipline_id_type& discipline_id) const;
    vector<research_api_obj> get_researches_by_research_group_id(const research_group_id_type& research_group_id) const;
    bool check_research_existence_by_permlink(const research_group_id_type& research_group_id, const string& permlink) const;

    //////////////////////
    // Research Content //
    //////////////////////
    research_content_api_obj get_research_content_by_id(const research_content_id_type& id) const;
    research_content_api_obj get_research_content_by_permlink(const research_id_type& research_id, const string& permlink) const;
    research_content_api_obj get_research_content_by_absolute_permlink(const string& research_group_permlink, const string& research_permlink, const string& research_content_permlink) const;
    vector<research_content_api_obj> get_all_research_content(const research_id_type& research_id) const;
    vector<research_content_api_obj> get_research_content_by_type(const research_id_type& research_id, const research_content_type& type) const;

    ///////////////////
    // Expert Tokens //
    ///////////////////
    expert_token_api_obj get_expert_token(const expert_token_id_type id) const;
    vector<expert_token_api_obj> get_expert_tokens_by_account_name(const account_name_type account_name) const;
    vector<expert_token_api_obj> get_expert_tokens_by_discipline_id(const discipline_id_type discipline_id) const;

    ////////////////////
    // Proposal       //
    ////////////////////
    vector<proposal_api_obj> get_proposals_by_research_group_id(const research_group_id_type research_group_id) const;
    proposal_api_obj get_proposal(const proposal_id_type id) const;

    ////////////////////
    // Research group //
    ////////////////////
    research_group_api_obj get_research_group_by_id(const research_group_id_type research_group_id) const;
    research_group_api_obj get_research_group_by_permlink(const string& permlink) const;
    bool check_research_group_existence_by_permlink(const string& permlink) const;

    /////////////////////////////////
    // Research group tokens       //
    /////////////////////////////////
    vector<research_group_token_api_obj> get_research_group_tokens_by_account(const account_name_type account) const;
    vector<research_group_token_api_obj> get_research_group_tokens_by_research_group(const research_group_id_type& research_group_id) const;
    research_group_token_api_obj
    get_research_group_token_by_account_and_research_group_id(const account_name_type account,
                                                              const research_group_id_type& research_group_id) const;

    /////////////////////////
    // Research token sale //
    /////////////////////////
    research_token_sale_api_obj get_research_token_sale_by_id(const research_token_sale_id_type research_token_sale_id) const;
    bool check_research_token_sale_existence_by_research_id(const research_id_type& research_id) const;
    research_token_sale_api_obj get_research_token_sale_by_research_id(const research_id_type& research_id) const;
    vector<research_token_sale_api_obj> get_research_token_sale_by_end_time(const time_point_sec end_time) const;
    research_token_sale_contribution_api_obj get_research_token_sale_contribution_by_id(const research_token_sale_contribution_id_type research_token_sale_contribution_id) const;
    vector<research_token_sale_contribution_api_obj> get_research_token_sale_contributions_by_research_token_sale_id(const research_token_sale_id_type research_token_sale_id) const;
    research_token_sale_contribution_api_obj get_research_token_sale_contribution_by_account_name_and_research_token_sale_id(const account_name_type owner,
                                                                                                                             const research_token_sale_id_type research_token_sale_id) const;

    ///////////////////////////////////
    // Research discipline relation  //
    ///////////////////////////////////

    vector<discipline_api_obj> get_disciplines_by_research(const research_id_type& research_id) const;

    ///////////////////////////////////
    // Research group invite         //
    ///////////////////////////////////

    research_group_invite_api_obj get_research_group_invite_by_id(const research_group_invite_id_type& research_group_invite_id) const;
    research_group_invite_api_obj get_research_group_invite_by_account_name_and_research_group_id(const account_name_type& account_name, const research_group_id_type& research_group_id) const;
    vector<research_group_invite_api_obj> get_research_group_invites_by_account_name(const account_name_type& account_name) const;
    vector<research_group_invite_api_obj> get_research_group_invites_by_research_group_id(const research_group_id_type& research_group_id) const;

    ///////////////////////////////////
    // Research listing              //
    ///////////////////////////////////

    vector<research_listing_api_obj> get_research_listing(const discipline_id_type& discipline_id, const uint64_t& from, const uint32_t& limit) const;
    vector<research_listing_api_obj> get_all_researches_listing(const discipline_id_type& discipline_id, const uint32_t& limit) const;

    ///////////////////////////////////
    // Total votes                   //
    ///////////////////////////////////
    vector<total_votes_api_obj> get_total_votes_by_research(const research_id_type& research_id) const;
    vector<total_votes_api_obj> get_total_votes_by_research_and_discipline(const research_id_type& research_id,
                                                                   const discipline_id_type& discipline_id) const;

    ///////////////////////////////
    // Reviews                   //
    ///////////////////////////////
    vector<review_api_obj> get_reviews_by_research(const research_id_type& research_id) const;
    vector<review_api_obj> get_reviews_by_content(const research_content_id_type& research_content_id) const;

    /////////////////////////////////
    // Research group join request //
    ////////////////////////////////
    research_group_join_request_api_obj get_research_group_join_request_by_id(const research_group_join_request_id_type& research_group_join_request_id) const;
    research_group_join_request_api_obj get_research_group_join_request_by_account_name_and_research_group_id(const account_name_type& account_name, const research_group_id_type& research_group_id) const;
    vector<research_group_join_request_api_obj> get_research_group_join_requests_by_account_name(const account_name_type& account_name) const;
    vector<research_group_join_request_api_obj> get_research_group_join_requests_by_research_group_id(const research_group_id_type& research_group_id) const;

    /////////////////////
    // Research token ///
    /////////////////////

    research_token_api_obj get_research_token_by_id(const research_token_id_type& research_token_id) const;
    vector<research_token_api_obj> get_research_tokens_by_account_name(const account_name_type &account_name) const;
    vector<research_token_api_obj> get_research_tokens_by_research_id(const research_id_type &research_id) const;
    research_token_api_obj get_research_token_by_account_name_and_research_id(const account_name_type &account_name,
                                                                              const research_id_type &research_id) const;

    //////////////////
    // Vote object ///
    //////////////////

    vector<vote_api_obj> get_votes_by_voter(const account_name_type &voter) const;
    vector<vote_api_obj> get_votes_by_research_id(const research_id_type &research_id) const;
    vector<vote_api_obj> get_votes_by_research_content_id(const research_content_id_type &research_content_id) const;


    ////////////////////////////
    // Handlers - not exposed //
    ////////////////////////////
    void on_api_startup();

private:
    std::shared_ptr<database_api_impl> my;
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

   // Blocks and transactions
   (get_block_header)
   (get_block)
   (get_ops_in_block)
   (get_state)

   // Globals
   (get_config)
   (get_dynamic_global_properties)
   (get_chain_properties)
   (get_witness_schedule)
   (get_hardfork_version)
   (get_next_scheduled_hardfork)
   (get_reward_fund)

   // Keys
   (get_key_references)

   // Accounts
   (get_accounts)
   (get_account_references)
   (lookup_account_names)
   (lookup_accounts)
   (get_account_count)
   (get_all_accounts)
   (get_account_history)
   (get_owner_history)
   (get_recovery_request)
   (get_withdraw_routes)
   (get_account_bandwidth)

   // Authority / validation
   (get_transaction_hex)
   (get_transaction)
   (get_required_signatures)
   (get_potential_signatures)
   (verify_authority)
   (verify_account_authority)

   // Witnesses
   (get_witnesses)
   (get_witness_by_account)
   (get_witnesses_by_vote)
   (lookup_witness_accounts)
   (get_witness_count)
   (get_active_witnesses)

    // Grant
   (get_grants)
   (lookup_grant_owners)

   // Disciplines
   (get_all_disciplines)
   (get_discipline)
   (get_discipline_by_name)
   (get_disciplines_by_parent_id)

   // Research
   (get_research_by_id)
   (get_research_by_permlink)
   (get_researches_by_discipline_id)
   (get_researches_by_research_group_id)
   (get_research_by_absolute_permlink)
   (check_research_existence_by_permlink)


   // Research Content
   (get_research_content_by_id)
   (get_all_research_content)
   (get_research_content_by_type)
   (get_research_content_by_permlink)
   (get_research_content_by_absolute_permlink)

   // Expert Tokens
   (get_expert_token)
   (get_expert_tokens_by_account_name)
   (get_expert_tokens_by_discipline_id)

   // Proposal
   (get_proposals_by_research_group_id)
   (get_proposal)

   // Research group
   (get_research_group_by_id)
   (get_research_group_by_permlink)
   (check_research_group_existence_by_permlink)

   // Research group tokens
   (get_research_group_tokens_by_account)
   (get_research_group_tokens_by_research_group)
   (get_research_group_token_by_account_and_research_group_id)

   // Research Token Sale
   (get_research_token_sale_by_id)
   (get_research_token_sale_by_research_id)
   (get_research_token_sale_by_end_time)
   (get_research_token_sale_contribution_by_id)
   (get_research_token_sale_contributions_by_research_token_sale_id)
   (get_research_token_sale_contribution_by_account_name_and_research_token_sale_id)
   (check_research_token_sale_existence_by_research_id)

   // Research discipline relation
   (get_disciplines_by_research)

   // Research group invite
    (get_research_group_invites_by_account_name) 
    (get_research_group_invites_by_research_group_id)

   // Research listing
    (get_research_listing)
    (get_all_researches_listing)

   // Total votes
   (get_total_votes_by_research)
   (get_total_votes_by_research_and_discipline)

   // Reviews
   (get_reviews_by_research)
   (get_reviews_by_content)

   // Research group join request
   (get_research_group_join_request_by_id)
   (get_research_group_join_request_by_account_name_and_research_group_id)
   (get_research_group_join_requests_by_account_name)
   (get_research_group_join_requests_by_research_group_id)

   // Research token
   (get_research_token_by_id)
   (get_research_tokens_by_account_name)
   (get_research_tokens_by_research_id)
   (get_research_token_by_account_name_and_research_id)

   // Votes
   (get_votes_by_voter)
   (get_votes_by_research_id)
   (get_votes_by_research_content_id)

)

// clang-format on
