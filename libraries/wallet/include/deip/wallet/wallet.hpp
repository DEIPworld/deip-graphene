#pragma once

#include <deip/app/api.hpp>
#include <deip/app/deip_api_objects.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/real128.hpp>
#include <fc/crypto/base58.hpp>

#include <deip/blockchain_history/applied_operation.hpp>

using namespace deip::app;
using namespace deip::chain;
using namespace graphene::utilities;
using namespace std;

namespace deip {
namespace wallet {

using deip::blockchain_history::applied_operation;
using deip::blockchain_history::applied_operation_type;
using transaction_handle_type = uint16_t;

struct memo_data
{

    static optional<memo_data> from_string(const std::string& str)
    {
        try
        {
            if (str.size() > sizeof(memo_data) && str[0] == '#')
            {
                auto data = fc::from_base58(str.substr(1));
                auto m = fc::raw::unpack<memo_data>(data);
                FC_ASSERT(string(m) == str);
                return m;
            }
        }
        catch (...)
        {
        }
        return optional<memo_data>();
    }

    public_key_type from;
    public_key_type to;
    uint64_t nonce = 0;
    uint32_t check = 0;
    vector<char> encrypted;

    operator string() const
    {
        auto data = fc::raw::pack(*this);
        auto base58 = fc::to_base58(data);
        return '#' + base58;
    }
};

struct brain_key_info
{
    string brain_priv_key;
    public_key_type pub_key;
    string wif_priv_key;
};

struct wallet_data
{
    vector<char> cipher_keys; /** encrypted keys */

    string ws_server = "ws://localhost:8090";
    string ws_user;
    string ws_password;

    chain_id_type chain_id;
};

enum authority_type
{
    owner,
    active
};

namespace detail {
class wallet_api_impl;
}

namespace utils {
fc::ecc::private_key derive_private_key(const std::string& prefix_string, int sequence_number);
brain_key_info suggest_brain_key();
} // namespace utils

/**
 * This wallet assumes it is connected to the database server with a high-bandwidth, low-latency connection and
 * performs minimal caching. This API could be provided locally to be used by a web interface.
 */
class wallet_api
{
public:
    wallet_api(const wallet_data& initial_data, fc::api<login_api> rapi);
    virtual ~wallet_api();

    bool copy_wallet_file(const std::string& destination_filename);

    /** Returns a list of all commands supported by the wallet API.
     *
     * This lists each command, along with its arguments and return types.
     * For more detailed help on a single command, use \c get_help()
     *
     * @returns a multi-line string suitable for displaying on a terminal
     */
    string help() const;

    /**
     * Returns info about the current state of the blockchain
     */
    variant info();

    /** Returns info such as client version, git version of graphene/fc, version of boost, openssl.
     * @returns compile time info and client and dependencies versions
     */
    variant_object about() const;

    /** Returns the information about a block header
     *
     * @param num Block num
     *
     * @returns Header block data on the blockchain
     */
    optional<block_header> get_block_header(uint32_t num) const;

    /** Returns the information about a block
     *
     * @param num Block num
     *
     * @returns Public block data on the blockchain
     */
    optional<signed_block_api_obj> get_block(uint32_t num) const;

    /** Returns information about the block headers in range [from-limit, from]
     *
     * @param num Block num, -1 means most recent, limit is the number of blocks before from.
     * @param limit the maximum number of items that can be queried (0 to 500], must be less than from
     *
     */
    std::map<uint32_t, block_header> get_block_headers_history(uint32_t num, uint32_t limit) const;

    /** Returns information about the blocks in range [from-limit, from]
     *
     * @param num Block num, -1 means most recent, limit is the number of blocks before from.
     * @param limit the maximum number of items that can be queried (0 to 500], must be less than from
     *
     */
    std::map<uint32_t, signed_block_api_obj> get_blocks_history(uint32_t num, uint32_t limit) const;

    /** Returns sequence of operations included/generated in a specified block
     *
     * @param block_num Block height of specified block
     * @param opt Operations type (all = 0, not_virt = 1, virt = 2, market = 3)
     */
    std::map<uint32_t, applied_operation> get_ops_in_block(uint32_t block_num, applied_operation_type opt) const;

     /**
     *  This method returns all operations in ids range [from-limit, from]
     *
     *  @param from_op - the operation number, -1 means most recent, limit is the number of operations before from.
     *  @param limit - the maximum number of items that can be queried (0 to 100], must be less than from
     *  @param opt Operations type (all = 0, not_virt = 1, virt = 2, market = 3)
     */
    std::map<uint32_t, applied_operation>
    get_ops_history(uint32_t from_op, uint32_t limit, applied_operation_type opt) const;

    /**
     * Returns the list of witnesses producing blocks in the current round (21 Blocks)
     */
    vector<account_name_type> get_active_witnesses() const;

    /**
     * Returns the state info associated with the URL
     */
    app::state get_state(const std::string& url);

    /**
     * Returns vesting withdraw routes for an account.
     *
     * @param account Account to query routes
     * @param type Withdraw type type [incoming, outgoing, all]
     */
    vector<withdraw_route> get_withdraw_routes(const std::string& account, withdraw_route_type type = all) const;

    /**
     *  Gets the account information for all accounts for which this wallet has a private key
     */
    vector<account_api_obj> list_my_accounts();

    /** Lists all accounts registered in the blockchain.
     * This returns a list of all account names and their account ids, sorted by account name.
     *
     * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all accounts,
     * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
     * the last account name returned as the \c lowerbound for the next \c list_accounts() call.
     *
     * @param lowerbound the name of the first account to return.  If the named account does not exist,
     *                   the list will start at the account that comes after \c lowerbound
     * @param limit the maximum number of accounts to return (max: 1000)
     * @returns a list of accounts mapping account names to account ids
     */
    vector<account_api_obj> list_accounts(const string& lowerbound, uint32_t limit);

    /** Returns the block chain's rapidly-changing properties.
     * The returned object contains information that changes every block interval
     * such as the head block number, the next witness, etc.
     * @see \c get_global_properties() for less-frequently changing properties
     * @returns the dynamic global properties
     */
    dynamic_global_property_api_obj get_dynamic_global_properties() const;

    /** Returns information about the given account.
     *
     * @param account_name the name of the account to provide information about
     * @returns the public account data stored in the blockchain
     */
    account_api_obj get_account(const std::string& account_name) const;

    /** Returns the current wallet filename.
     *
     * This is the filename that will be used when automatically saving the wallet.
     *
     * @see set_wallet_filename()
     * @return the wallet filename
     */
    string get_wallet_filename() const;

    /**
     * Get the WIF private key corresponding to a public key.  The
     * private key must already be in the wallet.
     */
    string get_private_key(const public_key_type& pubkey) const;

    /**
     *  @param account
     *  @param role - active | owner | memo
     *  @param password
     */
    pair<public_key_type, string> get_private_key_from_password(const std::string& account,
                                                                const std::string& role,
                                                                const std::string& password) const;

    /**
     * Returns transaction by ID.
     */
    annotated_signed_transaction get_transaction(transaction_id_type trx_id) const;

    /** Checks whether the wallet has just been created and has not yet had a password set.
     *
     * Calling \c set_password will transition the wallet to the locked state.
     * @return true if the wallet is new
     * @ingroup Wallet Management
     */
    bool is_new() const;

    /** Checks whether the wallet is locked (is unable to use its private keys).
     *
     * This state can be changed by calling \c lock() or \c unlock().
     * @return true if the wallet is locked
     * @ingroup Wallet Management
     */
    bool is_locked() const;

    /** Locks the wallet immediately.
     * @ingroup Wallet Management
     */
    void lock();

    /** Unlocks the wallet.
     *
     * The wallet remain unlocked until the \c lock is called
     * or the program exits.
     * @param password the password previously set with \c set_password()
     * @ingroup Wallet Management
     */
    void unlock(const std::string& password);

    /** Sets a new password on the wallet.
     *
     * The wallet must be either 'new' or 'unlocked' to
     * execute this command.
     * @ingroup Wallet Management
     */
    void set_password(const std::string& password);

    /** Dumps all private keys owned by the wallet.
     *
     * The keys are printed in WIF format.  You can import these keys into another wallet
     * using \c import_key()
     * @returns a map containing the private keys, indexed by their public key
     */
    map<public_key_type, string> list_keys();

    /** Returns detailed help on a single API command.
     * @param method the name of the API command you want help with
     * @returns a multi-line string suitable for displaying on a terminal
     */
    string gethelp(const std::string& method) const;

    /** Loads a specified Graphene wallet.
     *
     * The current wallet is closed before the new wallet is loaded.
     *
     * @warning This does not change the filename that will be used for future
     * wallet writes, so this may cause you to overwrite your original
     * wallet unless you also call \c set_wallet_filename()
     *
     * @param wallet_filename the filename of the wallet JSON file to load.
     *                        If \c wallet_filename is empty, it reloads the
     *                        existing wallet file
     * @returns true if the specified wallet is loaded
     */
    bool load_wallet_file(const std::string& wallet_filename = "");

    /** Saves the current wallet to the given filename.
     *
     * @warning This does not change the wallet filename that will be used for future
     * writes, so think of this function as 'Save a Copy As...' instead of
     * 'Save As...'.  Use \c set_wallet_filename() to make the filename
     * persist.
     * @param wallet_filename the filename of the new wallet JSON file to create
     *                        or overwrite.  If \c wallet_filename is empty,
     *                        save to the current filename.
     */
    void save_wallet_file(const std::string& wallet_filename = "");

    /** Sets the wallet filename used for future writes.
     *
     * This does not trigger a save, it only changes the default filename
     * that will be used the next time a save is triggered.
     *
     * @param wallet_filename the new filename to use for future saves
     */
    void set_wallet_filename(const std::string& wallet_filename);

    /** Suggests a safe brain key to use for creating your account.
     * \c create_account_with_brain_key() requires you to specify a 'brain key',
     * a long passphrase that provides enough entropy to generate cyrptographic
     * keys.  This function will suggest a suitably random string that should
     * be easy to write down (and, with effort, memorize).
     * @returns a suggested brain_key
     */
    brain_key_info suggest_brain_key() const;

    /** Converts a signed_transaction in JSON form to its binary representation.
     *
     * TODO: I don't see a broadcast_transaction() function, do we need one?
     *
     * @param tx the transaction to serialize
     * @returns the binary form of the transaction.  It will not be hex encoded,
     *          this returns a raw string that may have null characters embedded
     *          in it
     */
    string serialize_transaction(const signed_transaction& tx) const;

    /** Imports a WIF Private Key into the wallet to be used to sign transactions by an account.
     *
     * example: import_key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
     *
     * @param wif_key the WIF Private Key to import
     */
    bool import_key(const std::string& wif_key);

    /** Transforms a brain key to reduce the chance of errors when re-entering the key from memory.
     *
     * This takes a user-supplied brain key and normalizes it into the form used
     * for generating private keys.  In particular, this upper-cases all ASCII characters
     * and collapses multiple spaces into one.
     * @param s the brain key as supplied by the user
     * @returns the brain key in its normalized form
     */
    string normalize_brain_key(const std::string& s) const;

    /**
     *  This method will genrate new owner, active, and memo keys for the new account which
     *  will be controlable by this wallet. There is a fee associated with account creation
     *  that is paid by the creator. The current account creation fee can be found with the
     *  'info' wallet command.
     *
     *  @param creator The account creating the new account
     *  @param newname The name of the new account
     *  @param json_meta JSON Metadata associated with the new account
     *  @param fee The fee to be paid for account creation. It is converted to Common tokens for new account
     *  @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_account(const std::string& creator,
                                                const std::string& newname,
                                                const std::string& json_meta,
                                                const share_type& fee,
                                                bool broadcast);

    /**
     * This method is used by faucets to create new accounts for other users which must
     * provide their desired keys. The resulting account may not be controllable by this
     * wallet. There is a fee associated with account creation that is paid by the creator.
     * The current account creation fee can be found with the 'info' wallet command.
     *
     * @param creator The account creating the new account
     * @param newname The name of the new account
     * @param json_meta JSON Metadata associated with the new account
     * @param owner public owner key of the new account
     * @param active public active key of the new account
     * @param memo public memo key of the new account
     * @param fee The fee to paid for account creation. It is converted to Common tokens for new account
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction create_account_with_keys(const std::string& creator,
                                                          const std::string& newname,
                                                          const std::string& json_meta,
                                                          const public_key_type& owner,
                                                          const public_key_type& active,
                                                          const public_key_type& memo,
                                                          const share_type& fee,
                                                          bool broadcast) const;

    /**
     * This method updates the keys of an existing account.
     *
     * @param accountname The name of the account
     * @param json_meta New JSON Metadata to be associated with the account
     * @param owner New public owner key for the account
     * @param active New public active key for the account
     * @param memo New public memo key for the account
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction update_account(const std::string& accountname,
                                                const std::string& json_meta,
                                                const public_key_type& owner,
                                                const public_key_type& active,
                                                const public_key_type& memo,
                                                bool broadcast) const;

    /**
     * This method updates the key of an authority for an exisiting account.
     * Warning: You can create impossible authorities using this method. The method
     * will fail if you create an impossible owner authority, but will allow impossible
     * active authorities.
     *
     * @param account_name The name of the account whose authority you wish to update
     * @param type The authority type. e.g. owner or active
     * @param key The public key to add to the authority
     * @param weight The weight the key should have in the authority. A weight of 0 indicates the removal of the key.
     * @param broadcast true if you wish to broadcast the transaction.
     */
    annotated_signed_transaction update_account_auth_key(const std::string& account_name,
                                                         const authority_type& type,
                                                         const public_key_type& key,
                                                         weight_type weight,
                                                         bool broadcast);

    /**
     * This method updates the account of an authority for an exisiting account.
     * Warning: You can create impossible authorities using this method. The method
     * will fail if you create an impossible owner authority, but will allow impossible
     * active authorities.
     *
     * @param account_name The name of the account whose authority you wish to update
     * @param type The authority type. e.g. owner or active
     * @param auth_account The account to add the the authority
     * @param weight The weight the account should have in the authority. A weight of 0 indicates the removal of the
     * account.
     * @param broadcast true if you wish to broadcast the transaction.
     */
    annotated_signed_transaction update_account_auth_account(const std::string& account_name,
                                                             authority_type type,
                                                             const std::string& auth_account,
                                                             weight_type weight,
                                                             bool broadcast);

    /**
     * This method updates the weight threshold of an authority for an account.
     * Warning: You can create impossible authorities using this method as well
     * as implicitly met authorities. The method will fail if you create an implicitly
     * true authority and if you create an impossible owner authoroty, but will allow
     * impossible active authorities.
     *
     * @param account_name The name of the account whose authority you wish to update
     * @param type The authority type. e.g. owner or active
     * @param threshold The weight threshold required for the authority to be met
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction update_account_auth_threshold(const std::string& account_name,
                                                               authority_type type,
                                                               uint32_t threshold,
                                                               bool broadcast);

    /**
     * This method updates the account JSON metadata
     *
     * @param account_name The name of the account you wish to update
     * @param json_meta The new JSON metadata for the account. This overrides existing metadata
     * @param broadcast ture if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    update_account_meta(const std::string& account_name, const std::string& json_meta, bool broadcast);

    /**
     * This method updates the memo key of an account
     *
     * @param account_name The name of the account you wish to update
     * @param key The new memo public key
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    update_account_memo_key(const std::string& account_name, const public_key_type& key, bool broadcast);

    /**
     *  This method is used to convert a JSON transaction to its transaction ID.
     */
    transaction_id_type get_transaction_id(const signed_transaction& trx) const
    {
        return trx.id();
    }

    /** Lists all witnesses registered in the blockchain.
     * This returns a list of all account names that own witnesses, and the associated witness id,
     * sorted by name.  This lists witnesses whether they are currently voted in or not.
     *
     * Use the \c lowerbound and limit parameters to page through the list.  To retrieve all witnesss,
     * start by setting \c lowerbound to the empty string \c "", and then each iteration, pass
     * the last witness name returned as the \c lowerbound for the next \c list_witnesss() call.
     *
     * @param lowerbound the name of the first witness to return.  If the named witness does not exist,
     *                   the list will start at the witness that comes after \c lowerbound
     * @param limit the maximum number of witnesss to return (max: 1000)
     * @returns a list of witnesss mapping witness names to witness ids
     */
    set<account_name_type> list_witnesses(const string& lowerbound, uint32_t limit);

    /** Returns information about the given witness.
     * @param owner_account the name or id of the witness account owner, or the id of the witness
     * @returns the information about the witness stored in the block chain
     */
    optional<witness_api_obj> get_witness(const std::string& owner_account);

    /**
     * Update a witness object owned by the given account.
     *
     * @param witness_name The name of the witness account.
     * @param url A URL containing some information about the witness.  The empty string makes it remain the same.
     * @param block_signing_key The new block signing public key.  The empty string disables block production.
     * @param props The chain properties the witness is voting on.
     * @param broadcast true if you wish to broadcast the transaction.
     */
    annotated_signed_transaction update_witness(const std::string& witness_name,
                                                const std::string& url,
                                                const public_key_type& block_signing_key,
                                                const chain_properties& props,
                                                bool broadcast = false);

    /** Set the voting proxy for an account.
     *
     * If a user does not wish to take an active part in voting, they can choose
     * to allow another account to vote their stake.
     *
     * Setting a vote proxy does not remove your previous votes from the blockchain,
     * they remain there but are ignored.  If you later null out your vote proxy,
     * your previous votes will take effect again.
     *
     * This setting can be changed at any time.
     *
     * @param account_to_modify the name or id of the account to update
     * @param proxy the name of account that should proxy to, or empty string to have no proxy
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    set_voting_proxy(const std::string& account_to_modify, const std::string& proxy, bool broadcast = false);

    /**
     * Vote for a witness to become a block producer. By default an account has not voted
     * positively or negatively for a witness. The account can either vote for with positively
     * votes or against with negative votes. The vote will remain until updated with another
     * vote. Vote strength is determined by the accounts vesting shares.
     *
     * @param account_to_vote_with The account voting for a witness
     * @param witness_to_vote_for The witness that is being voted for
     * @param approve true if the account is voting for the account to be able to be a block produce
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction vote_for_witness(const std::string& account_to_vote_with,
                                                  const std::string& witness_to_vote_for,
                                                  bool approve = true,
                                                  bool broadcast = false);

    /**
     * Transfer funds from one account to another. DEIP and SBD can be transferred.
     *
     * @param from The account the funds are coming from
     * @param to The account the funds are going to
     * @param amount The funds being transferred. i.e. "100.000 DEIP"
     * @param memo A memo for the transactionm, encrypted with the to account's public memo key
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction transfer(const std::string& from,
                                          const std::string& to,
                                          const asset& amount,
                                          const std::string& memo,
                                          bool broadcast = false);

    /**
     * Transfer DEIP into a vesting fund represented by vesting shares (VESTS). VESTS are required to vesting
     * for a minimum of one coin year and can be withdrawn once a week over a two year withdraw period.
     * VESTS are protected against dilution up until 90% of DEIP is vesting.
     *
     * @param from The account the DEIP is coming from
     * @param to The account getting the VESTS
     * @param amount The amount of DEIP to vest i.e. "100.00 DEIP"
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    transfer_to_common_tokens(const std::string& from, const std::string& to, const asset& amount, bool broadcast = false);

    /**
     * Set up a vesting withdraw request. The request is fulfilled once a week over the next two year (104 weeks).
     *
     * @param from The account the VESTS are withdrawn from
     * @param vesting_shares The amount of VESTS to withdraw over the next two years. Each week (amount/104) shares are
     *    withdrawn and deposited back as DEIP. i.e. "10.000000 VESTS"
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    withdraw_common_tokens(const std::string& from, const share_type& vesting_shares, bool broadcast = false);

    /**
     * Set up a vesting withdraw route. When vesting shares are withdrawn, they will be routed to these accounts
     * based on the specified weights.
     *
     * @param from The account the VESTS are withdrawn from.
     * @param to   The account receiving either VESTS or DEIP.
     * @param percent The percent of the withdraw to go to the 'to' account. This is denoted in hundreths of a percent.
     *    i.e. 100 is 1% and 10000 is 100%. This value must be between 1 and 100000
     * @param auto_common_token Set to true if the from account should receive the VESTS as VESTS, or false if it should receive
     *    them as DEIP.
     * @param broadcast true if you wish to broadcast the transaction.
     */
    annotated_signed_transaction set_withdraw_common_tokens_route(
        const std::string& from, const std::string& to, uint16_t percent, bool auto_common_token, bool broadcast = false);

    /**
     * Transfers research tokens from one acount to another
     *
     * @param research_external_id external id of research which tokens to transfer
     * @param from The account who transfers research tokens
     * @param to The account receiving research tokens
     * @param share The account of research tokens to transfer
     * @param broadcast
     */
    annotated_signed_transaction transfer_research_share(const external_id_type& research_external_id,
                                                         const std::string& from,
                                                         const std::string& to,
                                                         const percent& share,
                                                         bool broadcast = false);

    /** Signs a transaction.
     *
     * Given a fully-formed transaction that is only lacking signatures, this signs
     * the transaction with the necessary keys and optionally broadcasts the transaction
     * @param tx the unsigned transaction
     * @param broadcast true if you wish to broadcast the transaction
     * @return the signed version of the transaction
     */
    annotated_signed_transaction sign_transaction(const signed_transaction& tx, bool broadcast = false);

    /** Returns an uninitialized object representing a given blockchain operation.
     *
     * This returns a default-initialized object of the given type; it can be used
     * during early development of the wallet when we don't yet have custom commands for
     * creating all of the operations the blockchain supports.
     *
     * Any operation the blockchain supports can be created using the transaction builder's
     * \c add_operation_to_builder_transaction() , but to do that from the CLI you need to
     * know what the JSON form of the operation looks like.  This will give you a template
     * you can fill in.  It's better than nothing.
     *
     * @param operation_type the type of operation to return, must be one of the
     *                       operations defined in `deip/chain/operations.hpp`
     *                       (e.g., "global_parameters_update_operation")
     * @return a default-constructed operation of the given type
     */
    operation get_prototype_operation(const std::string& operation_type);

    void network_add_nodes(const vector<string>& nodes);
    vector<variant> network_get_connected_peers();

    /**
     * Sets the amount of time in the future until a transaction expires.
     */
    void set_transaction_expiration(uint32_t seconds);

    /**
     * Challenge a user's authority. The challenger pays a fee to the challenged which is depositted as
     * Deip Power. Until the challenged proves their active key, all rights are revoked.
     *
     * @param challenger The account issuing the challenge
     * @param challenged The account being challenged
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    challenge(const std::string& challenger, const std::string& challenged, bool broadcast);

    /**
     * Create an account recovery request as a recover account. The syntax for this command contains a serialized
     * authority object
     * so there is an example below on how to pass in the authority.
     *
     * request_account_recovery "your_account" "account_to_recover" {"weight_threshold": 1,"account_auths": [],
     * "key_auths": [["new_public_key",1]]} true
     *
     * @param recovery_account The name of your account
     * @param account_to_recover The name of the account you are trying to recover
     * @param new_authority The new owner authority for the recovered account. This should be given to you by the holder
     * of the compromised or lost account.
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction request_account_recovery(const std::string& recovery_account,
                                                          const std::string& account_to_recover,
                                                          const authority& new_authority,
                                                          bool broadcast);

    /**
     * Recover your account using a recovery request created by your recovery account. The syntax for this commain
     * contains a serialized
     * authority object, so there is an example below on how to pass in the authority.
     *
     * recover_account "your_account" {"weight_threshold": 1,"account_auths": [], "key_auths": [["old_public_key",1]]}
     * {"weight_threshold": 1,"account_auths": [], "key_auths": [["new_public_key",1]]} true
     *
     * @param account_to_recover The name of your account
     * @param recent_authority A recent owner authority on your account
     * @param new_authority The new authority that your recovery account used in the account recover request.
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction recover_account(const std::string& account_to_recover,
                                                 const authority& recent_authority,
                                                 const authority& new_authority,
                                                 bool broadcast);

    /**
     * Change your recovery account after a 30 day delay.
     *
     * @param owner The name of your account
     * @param new_recovery_account The name of the recovery account you wish to have
     * @param broadcast true if you wish to broadcast the transaction
     */
    annotated_signed_transaction
    change_recovery_account(const std::string& owner, const std::string& new_recovery_account, bool broadcast);

    vector<owner_authority_history_api_obj> get_owner_history(const std::string& account) const;

    /**
     *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
     *  returns operations in the range [from-limit, from]
     *
     *  @param account - account whose history will be returned
     *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
     *  @param limit - the maximum number of items that can be queried (0 to 100], must be less than from
     */
    std::map<uint32_t, applied_operation>
    get_account_history(const std::string& account, uint64_t from, uint32_t limit);
    /**
     *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
     *  returns operations in the range [from-limit, from]
     *
     *  @param account - account whose history will be returned
     *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
     *  @param limit - the maximum number of items that can be queried (0 to 100], must be less than from
     */
    std::map<uint32_t, applied_operation>
    get_account_deip_to_deip_transfers(const std::string& account, uint64_t from, uint32_t limit);

    /**
     *  Account operations have sequence numbers from 0 to N where N is the most recent operation. This method
     *  returns operations in the range [from-limit, from]
     *
     *  @param account - account whose history will be returned
     *  @param from - the absolute sequence number, -1 means most recent, limit is the number of operations before from.
     *  @param limit - the maximum number of items that can be queried (0 to 100], must be less than from
     */
    std::map<uint32_t, applied_operation>
    get_account_deip_to_common_tokens_transfers(const std::string& account, uint64_t from, uint32_t limit);

    std::map<string, std::function<string(fc::variant, const fc::variants&)>> get_result_formatters() const;

    void encrypt_keys();

    /**
     * Checks memos against private keys on account and imported in wallet
     */
    void check_memo(const string& memo, const account_api_obj& account) const;

    /**
     *  Returns the encrypted memo if memo starts with '#' otherwise returns memo
     */
    string get_encrypted_memo(const std::string& from, const std::string& to, const std::string& memo);

    /**
     * Returns the decrypted memo if possible given wallet's known private keys
     */
    string decrypt_memo(const std::string& memo);

    /**
     *  Gets the discipline_supply information for all my discipline_supplies (list_my_accounts)
     */
    vector<discipline_supply_api_obj> list_my_discipline_supplies();

    /**
     *  Gets the list of all discipline_supply owners (look list_accounts to understand input parameters)
     */
    set<string> list_discipline_supply_grantors(const std::string& lowerbound, uint32_t limit);

    /**
     *  Gets the discipline_supply information for certain account
     */
    vector<discipline_supply_api_obj> get_discipline_supplies(const std::string& account_name);

    /**
     *  Gets the list of all vesting balance for account
     */
    vector<vesting_balance_api_obj> get_vesting_balances(const std::string& account_name);

    /**
     *  Gets proposal details
     */
    fc::optional<proposal_api_obj> get_proposal(const external_id_type& proposal_id);

    /**
     *  Gets the list of all proposals for research group
     */
    vector<proposal_api_obj> get_proposals_by_creator(const account_name_type& creator);

    /**
     *  Gets the list of research token sale
     */
    vector<research_token_sale_api_obj> list_research_token_sales(const uint32_t& from, uint32_t limit);

    /**
     *  Gets research content by id
     */
    fc::optional<research_content_api_obj> get_research_content(const int64_t id);

    /**
     *  Gets the list of research contents by type
     */
    vector<research_content_api_obj> get_research_contents_by_type(const int64_t research_id, const uint16_t type);

    /**
     *  Gets the list of researches by id research group
     */
    vector<research_api_obj> get_researches_by_research_group(const external_id_type& external_id) const;

    /**
     *  This method will create new discipline_supply linked to owner account.
     *
     *  @warning The owner account must have sufficient balance for discipline_supply
     *
     *  @param grantor The future owner of creating discipline_supply
     *  @param amount The amount of discipline_supply
     *  @param start_time Time when discipline_supply will be distributed
     *  @param end_time Time when discipline_supply distribution ends
     *  @param target_discipline The target discipline name discipline_supply will be distributed to
     *  @param content_hash Hash of description of discipline_supply
     *  @param is_extendable Set to 'true' if you want your discipline_supply to extend if not distributed in specified period
     *  @param broadcast
     */
    annotated_signed_transaction create_discipline_supply(const std::string& grantor,
                                                          const asset& amount,
                                                          const uint32_t start_time,
                                                          const uint32_t end_time,
                                                          const external_id_type& target_discipline,
                                                          const std::string& content_hash,
                                                          const bool is_extendable,
                                                          const bool broadcast);

    /**
     * Vote for review
     *
     * @param voter The account who votes for review
     * @param review_id Id of review
     * @param discipline_id Id of dscipline vote will be giver for review in
     * @param weight Weight of vote from 0 to 10000
     * @param broadcast
     */
    annotated_signed_transaction vote_for_review(const external_id_type& external_id,
                                                 const std::string& voter,
                                                 const external_id_type& review_external_id,
                                                 const external_id_type& discipline_external_id,
                                                 const percent& weight,
                                                 const bool broadcast);

    /**
     * Make review for specified research content
     *
     * @param author The account who makes a review
     * @param research_content_id Id of research content to make review for
     * @param is_positive Boolean indicating whether review is positive or negative
     * @param content Review text
     * @param broadcast
     */
    annotated_signed_transaction create_review(const external_id_type& external_id,
                                               const std::string& author,
                                               const external_id_type& research_content_id,
                                               const bool& is_positive,
                                               const std::string& content,
                                               const bool broadcast);

    /**
     * Participate in token sale and make your contribution
     *

     * @param contributor The account who is participating in token sale
     * @param research_external_id Id of research token sale
     * @param amount Amount of DEIP tokens to contribute to token sale (ex. "1.000 DEIP")
     * @param broadcast
     */
    annotated_signed_transaction contribute_to_token_sale(const std::string& contributor,
                                                          const external_id_type& research_external_id,
                                                          const asset& amount,
                                                          const bool broadcast);

    /**
     * Create new vesting contract
     *
     * @param creator The account who creates vesting contract
     * @param owner The account who owns tokens from contract
     * @param balance Amount to vest (i.e. "1.000 DEIP")
     * @param vesting_duration_seconds Duration of vesting in seconds
     * @param vesting_cliff_seconds Duration of vesting cliff in seconds
     * @param period_duration_seconds Duration of withdraw period in seconds (funds will be available every period, i.e. every 3 months)
     * @param broadcast
     */
    annotated_signed_transaction create_vesting_balance(const std::string &creator, const std::string &owner, const asset &balance,
                                                             const uint32_t &vesting_duration_seconds, const uint32_t &vesting_cliff_seconds,
                                                             const uint32_t &period_duration_seconds, const bool broadcast);

    /**
     * Withdraw from vesting contract. Only withdraws the amount available for withdrawal
     *
     * @param vesting_balance_id The account who created vesting contract
     * @param owner The account who owns tokens from contract
     * @param amount Amount to withdraw (i.e. "1.000 DEIP")
     * @param broadcast
     */
    annotated_signed_transaction withdraw_vesting_balance(const int64_t &vesting_balance_id,
                                                           const std::string &owner,
                                                           const asset &amount,
                                                           const bool broadcast);


    annotated_signed_transaction create_expertise_allocation_proposal(const std::string &claimer, const std::string &description,
                                                                          const int64_t discipline_id, const bool broadcast);


    annotated_signed_transaction vote_for_expertise_allocation_proposal(const int64_t proposal_id, const std::string &voter,
                                                                            const int64_t voting_power, const bool broadcast);

    annotated_signed_transaction create_grant(const std::string& grantor,
                                              const asset& amount,
                                              const external_id_type& target_discipline,
                                              const std::string& funding_opportunity_number,
                                              const external_id_type& review_committee_id,
                                              const uint16_t& min_number_of_positive_reviews,
                                              const uint16_t& min_number_of_applications,
                                              const uint16_t& max_number_of_research_to_grant,
                                              const uint32_t& start_time,
                                              const uint32_t& end_time,
                                              const bool broadcast);

    annotated_signed_transaction create_grant_application(const std::string& funding_opportunity_number,
                                                          const int64_t& research_id,
                                                          const std::string& creator,
                                                          const std::string& application_hash,
                                                          const bool broadcast);

    annotated_signed_transaction create_review_for_application(const std::string& author,
                                                               const int64_t& grant_application_id,
                                                               const bool& is_positive,
                                                               const std::string& content,
                                                               const bool broadcast);

    annotated_signed_transaction approve_grant_application(const int64_t& grant_application_id,
                                                           const std::string& approver,
                                                           const bool broadcast);

    annotated_signed_transaction reject_grant_application(const int64_t& grant_application_id,
                                                          const std::string& rejector,
                                                          const bool broadcast);

    annotated_signed_transaction create_asset(const std::string& issuer,
                                              const std::string& symbol,
                                              const uint8_t& precision,
                                              const std::string& description,
                                              const bool broadcast);

    annotated_signed_transaction issue_asset(const std::string& issuer,
                                             const asset& amount,
                                             const bool broadcast);

    annotated_signed_transaction reserve_asset(const std::string& owner,
                                               const asset& amount,
                                               const bool broadcast);

    annotated_signed_transaction create_contract_agreement(const std::string& external_id,
                                                           const std::string& creator,
                                                           const flat_set<std::string>& parties,
                                                           const std::string& hash,
                                                           const bool broadcast);

    annotated_signed_transaction accept_contract_agreement(const std::string& external_id,
                                                           const std::string& party,
                                                           const bool broadcast);

    public : fc::signal<void(bool)> lock_changed;

private:
    std::shared_ptr<detail::wallet_api_impl> my;
};

struct plain_keys
{
    fc::sha512 checksum;
    map<public_key_type, string> keys;
};
} // namespace wallet
} // namespace deip

// clang-format off

FC_REFLECT(deip::wallet::wallet_data,
           (cipher_keys)
           (ws_server)
           (ws_user)
           (ws_password)
           (chain_id))

FC_REFLECT( deip::wallet::brain_key_info, (brain_priv_key)(wif_priv_key) (pub_key))

FC_REFLECT( deip::wallet::plain_keys, (checksum)(keys) )

FC_REFLECT_ENUM( deip::wallet::authority_type, (owner)(active) )

FC_API( deip::wallet::wallet_api,
        /// wallet api
        (help)(gethelp)
        (about)(is_new)(is_locked)(lock)(unlock)(set_password)
        (load_wallet_file)(save_wallet_file)

        /// key api
        (import_key)
        (suggest_brain_key)
        (list_keys)
        (get_private_key)
        (get_private_key_from_password)
        (normalize_brain_key)

        /// query api
        (info)
        (list_my_accounts)
        (list_accounts)
        (list_witnesses)
        (get_witness)
        (get_account)
        (get_block_header)
        (get_block)
        (get_block_headers_history)
        (get_blocks_history)
        (get_ops_in_block)
        (get_ops_history)
        (get_account_history)
        (get_account_deip_to_deip_transfers)
        (get_account_deip_to_common_tokens_transfers)
        (get_state)
        (get_withdraw_routes)
        (list_my_discipline_supplies)
        (list_discipline_supply_grantors)
        (get_discipline_supplies)
        (get_vesting_balances)
        (get_proposal)
        (get_proposals_by_creator)
        (list_research_token_sales)
        (get_research_content)
        (get_research_contents_by_type)
        (get_researches_by_research_group)

        /// transaction api
        (create_account)
        (create_account_with_keys)
        (update_account)
        (update_account_auth_key)
        (update_account_auth_account)
        (update_account_auth_threshold)
        (update_account_meta)
        (update_account_memo_key)
        (update_witness)
        (set_voting_proxy)
        (vote_for_witness)
        (transfer)
        (transfer_to_common_tokens)
        (withdraw_common_tokens)
        (set_withdraw_common_tokens_route)
        (transfer_research_share)
        (set_transaction_expiration)
        (challenge)
        (request_account_recovery)
        (recover_account)
        (change_recovery_account)
        (get_owner_history)
        (get_encrypted_memo)
        (decrypt_memo)
        (create_discipline_supply)
        (vote_for_review)
        (create_review)
        (contribute_to_token_sale)
        (create_vesting_balance)
        (withdraw_vesting_balance)
        (create_expertise_allocation_proposal)
        (vote_for_expertise_allocation_proposal)
        (create_grant)
        (create_grant_application)
        (create_review_for_application)
        (approve_grant_application)
        (reject_grant_application)
        (create_asset)
        (issue_asset)
        (reserve_asset)
        (create_contract_agreement)
        (accept_contract_agreement)

        /// helper api
        (get_prototype_operation)
        (serialize_transaction)
        (sign_transaction)

        (network_add_nodes)
        (network_get_connected_peers)

        (get_active_witnesses)
        (get_transaction)
      )

FC_REFLECT( deip::wallet::memo_data, (from)(to)(nonce)(check)(encrypted) )

// clang-format on
