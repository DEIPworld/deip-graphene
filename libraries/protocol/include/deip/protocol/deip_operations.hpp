#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/asset.hpp>

#include <fc/utf8.hpp>
#include <fc/crypto/equihash.hpp>
#include <fc/shared_string.hpp>
#include <fc/io/json.hpp>

namespace deip {
namespace protocol {

inline void validate_account_name(const string& name)
{
    FC_ASSERT(is_valid_account_name(name), "Account name ${n} is invalid", ("n", name));
}

inline void validate_permlink(const string& permlink)
{
    FC_ASSERT(permlink.size() < DEIP_MAX_PERMLINK_LENGTH, "permlink is too long");
    FC_ASSERT(fc::is_utf8(permlink), "permlink not formatted in UTF8");
}

inline void validate_enum_value_by_range(int val, int first, int last)
{
    FC_ASSERT(val >= first && val <= last, "Provided enum value is outside of the range: val = ${enum_val}, first = ${first}, last = ${last}", 
                                            ("enum_val", val)("first", first)("last", last));
}

struct account_create_operation : public base_operation
{
    asset fee;
    account_name_type creator;
    account_name_type new_account_name;
    authority owner;
    authority active;
    authority posting;
    public_key_type memo_key;
    string json_metadata;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(creator);
    }
};

struct account_update_operation : public base_operation
{
    account_name_type account;
    optional<authority> owner;
    optional<authority> active;
    optional<authority> posting;
    public_key_type memo_key;
    string json_metadata;

    void validate() const;

    void get_required_owner_authorities(flat_set<account_name_type>& a) const
    {
        if (owner)
            a.insert(account);
    }

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        if (!owner)
            a.insert(account);
    }
};

struct beneficiary_route_type
{
    beneficiary_route_type()
    {
    }
    beneficiary_route_type(const account_name_type& a, const uint16_t& w)
        : account(a)
        , weight(w)
    {
    }

    account_name_type account;
    uint16_t weight;

    // For use by std::sort such that the route is sorted first by name (ascending)
    bool operator<(const beneficiary_route_type& o) const
    {
        return account < o.account;
    }
};

struct vote_for_review_operation : public base_operation
{
    account_name_type voter;
    int64_t review_id;
    int64_t discipline_id;
    int16_t weight = 0;

    void validate() const;
    void get_required_posting_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(voter);
    }
};

/**
 * @ingroup operations
 *
 * @brief Transfers DEIP from one account to another.
 */
struct transfer_operation : public base_operation
{
    account_name_type from;
    /// Account to transfer asset to
    account_name_type to;
    /// The amount of asset to transfer from @ref from to @ref to
    asset amount;

    /// The memo is plain-text, any encryption on the memo is up to
    /// a higher level protocol.
    string memo;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(from);
    }
};

/**
 *  This operation converts DEIP into VFS (Vesting Fund Shares) at
 *  the current exchange rate. With this operation it is possible to
 *  give another account vesting shares so that faucets can
 *  pre-fund new accounts with vesting shares.
 */
struct transfer_to_common_tokens_operation : public base_operation
{
    account_name_type from;
    account_name_type to; ///< if null, then same as from
    asset amount; ///< must be DEIP

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(from);
    }
};

/**
 * At any given point in time an account can be withdrawing from their
 * vesting shares. A user may change the number of shares they wish to
 * cash out at any time between 0 and their total vesting stake.
 *
 * After applying this operation, common_tokens will be withdrawn
 * at a rate of common_tokens/104 per week for two years starting
 * one week after this operation is included in the blockchain.
 *
 * This operation is not valid if the user has no vesting shares.
 */
struct withdraw_common_tokens_operation : public base_operation
{
    account_name_type account;
    share_type total_common_tokens_amount;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(account);
    }
};

/**
 * Allows an account to setup a vesting withdraw but with the additional
 * request for the funds to be transferred directly to another account's
 * balance rather than the withdrawing account. In addition, those funds
 * can be immediately vested again, circumventing the conversion from
 * vests to deip and back, guaranteeing they maintain their value.
 */
struct set_withdraw_common_tokens_route_operation : public base_operation
{
    account_name_type from_account;
    account_name_type to_account;
    uint16_t percent = 0;
    bool auto_common_token = false;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(from_account);
    }
};

/**
 * Witnesses must vote on how to set certain chain properties to ensure a smooth
 * and well functioning network.  Any time @owner is in the active set of witnesses these
 * properties will be used to control the blockchain configuration.
 */
struct chain_properties
{
    /**
     *  This fee, paid in DEIP, is converted into VESTING SHARES for the new account. Accounts
     *  without vesting shares cannot earn usage rations and therefore are powerless. This minimum
     *  fee requires all accounts to have some kind of commitment to the network that includes the
     *  ability to vote and make transactions.
     */
    asset account_creation_fee = asset(DEIP_MIN_ACCOUNT_CREATION_FEE, DEIP_SYMBOL);

    /**
     *  This witnesses vote for the maximum_block_size which is used by the network
     *  to tune rate limiting and capacity
     */
    uint32_t maximum_block_size = DEIP_MIN_BLOCK_SIZE_LIMIT * 2;

    void validate() const
    {
        FC_ASSERT(account_creation_fee.amount >= DEIP_MIN_ACCOUNT_CREATION_FEE);
        FC_ASSERT(maximum_block_size >= DEIP_MIN_BLOCK_SIZE_LIMIT);
    }
};

/**
 *  Users who wish to become a witness must pay a fee acceptable to
 *  the current witnesses to apply for the position and allow voting
 *  to begin.
 *
 *  If the owner isn't a witness they will become a witness.  Witnesses
 *  are charged a fee equal to 1 weeks worth of witness pay which in
 *  turn is derived from the current share supply.  The fee is
 *  only applied if the owner is not already a witness.
 *
 *  If the block_signing_key is null then the witness is removed from
 *  contention.  The network will pick the top 21 witnesses for
 *  producing blocks.
 */
struct witness_update_operation : public base_operation
{
    account_name_type owner;
    string url;
    public_key_type block_signing_key;
    chain_properties props;
    asset fee; ///< the fee paid to register a new witness, should be 10x current block production pay

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(owner);
    }
};

/**
 * All accounts with a VFS can vote for or against any witness.
 *
 * If a proxy is specified then all existing votes are removed.
 */
struct account_witness_vote_operation : public base_operation
{
    account_name_type account;
    account_name_type witness;
    bool approve = true;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(account);
    }
};

struct account_witness_proxy_operation : public base_operation
{
    account_name_type account;
    account_name_type proxy;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(account);
    }
};

/**
 * All account recovery requests come from a listed recovery account. This
 * is secure based on the assumption that only a trusted account should be
 * a recovery account. It is the responsibility of the recovery account to
 * verify the identity of the account holder of the account to recover by
 * whichever means they have agreed upon. The blockchain assumes identity
 * has been verified when this operation is broadcast.
 *
 * This operation creates an account recovery request which the account to
 * recover has 24 hours to respond to before the request expires and is
 * invalidated.
 *
 * There can only be one active recovery request per account at any one time.
 * Pushing this operation for an account to recover when it already has
 * an active request will either update the request to a new new owner authority
 * and extend the request expiration to 24 hours from the current head block
 * time or it will delete the request. To cancel a request, simply set the
 * weight threshold of the new owner authority to 0, making it an open authority.
 *
 * Additionally, the new owner authority must be satisfiable. In other words,
 * the sum of the key weights must be greater than or equal to the weight
 * threshold.
 *
 * This operation only needs to be signed by the the recovery account.
 * The account to recover confirms its identity to the blockchain in
 * the recover account operation.
 */
struct request_account_recovery_operation : public base_operation
{
    account_name_type
        recovery_account; ///< The recovery account is listed as the recovery account on the account to recover.

    account_name_type
        account_to_recover; ///< The account to recover. This is likely due to a compromised owner authority.

    authority new_owner_authority; ///< The new owner authority the account to recover wishes to have. This is secret
    ///< known by the account to recover and will be confirmed in a recover_account_operation

    extensions_type extensions; ///< Extensions. Not currently used.

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(recovery_account);
    }

    void validate() const;
};

/**
 * Recover an account to a new authority using a previous authority and verification
 * of the recovery account as proof of identity. This operation can only succeed
 * if there was a recovery request sent by the account's recover account.
 *
 * In order to recover the account, the account holder must provide proof
 * of past ownership and proof of identity to the recovery account. Being able
 * to satisfy an owner authority that was used in the past 30 days is sufficient
 * to prove past ownership. The get_owner_history function in the database API
 * returns past owner authorities that are valid for account recovery.
 *
 * Proving identity is an off chain contract between the account holder and
 * the recovery account. The recovery request contains a new authority which
 * must be satisfied by the account holder to regain control. The actual process
 * of verifying authority may become complicated, but that is an application
 * level concern, not a blockchain concern.
 *
 * This operation requires both the past and future owner authorities in the
 * operation because neither of them can be derived from the current chain state.
 * The operation must be signed by keys that satisfy both the new owner authority
 * and the recent owner authority. Failing either fails the operation entirely.
 *
 * If a recovery request was made inadvertantly, the account holder should
 * contact the recovery account to have the request deleted.
 *
 * The two setp combination of the account recovery request and recover is
 * safe because the recovery account never has access to secrets of the account
 * to recover. They simply act as an on chain endorsement of off chain identity.
 * In other systems, a fork would be required to enforce such off chain state.
 * Additionally, an account cannot be permanently recovered to the wrong account.
 * While any owner authority from the past 30 days can be used, including a compromised
 * authority, the account can be continually recovered until the recovery account
 * is confident a combination of uncompromised authorities were used to
 * recover the account. The actual process of verifying authority may become
 * complicated, but that is an application level concern, not the blockchain's
 * concern.
 */
struct recover_account_operation : public base_operation
{
    account_name_type account_to_recover; ///< The account to be recovered

    authority new_owner_authority; ///< The new owner authority as specified in the request account recovery operation.

    authority recent_owner_authority; ///< A previous owner authority that the account holder will use to prove past
    /// ownership of the account to be recovered.

    extensions_type extensions; ///< Extensions. Not currently used.

    void get_required_authorities(vector<authority>& a) const
    {
        a.push_back(new_owner_authority);
        a.push_back(recent_owner_authority);
    }

    void validate() const;
};

/**
 * Each account lists another account as their recovery account.
 * The recovery account has the ability to create account_recovery_requests
 * for the account to recover. An account can change their recovery account
 * at any time with a 30 day delay. This delay is to prevent
 * an attacker from changing the recovery account to a malicious account
 * during an attack. These 30 days match the 30 days that an
 * owner authority is valid for recovery purposes.
 *
 * On account creation the recovery account is set either to the creator of
 * the account (The account that pays the creation fee and is a signer on the transaction)
 * or to the empty string if the account was mined. An account with no recovery
 * has the top voted witness as a recovery account, at the time the recover
 * request is created. Note: This does mean the effective recovery account
 * of an account with no listed recovery account can change at any time as
 * witness vote weights. The top voted witness is explicitly the most trusted
 * witness according to stake.
 */
struct change_recovery_account_operation : public base_operation
{
    account_name_type account_to_recover; ///< The account that would be recovered in case of compromise
    account_name_type new_recovery_account; ///< The account that creates the recover request
    extensions_type extensions; ///< Extensions. Not currently used.

    void get_required_owner_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(account_to_recover);
    }
    void validate() const;
};

// DEIP native operations

struct invitee_type
{
    invitee_type()
    {
    }
    invitee_type(const account_name_type& a, const uint32_t& t, const std::string& c)
            : account(a)
            , research_group_tokens_in_percent(t)
            , cover_letter(c)
    {
    }

    account_name_type account;
    uint32_t research_group_tokens_in_percent;
    std::string cover_letter;
};

struct expertise_amount_pair_type
{
    expertise_amount_pair_type()
    {
    }

    expertise_amount_pair_type(const int64_t& d, const int64_t& a)
            : discipline_id(d)
            , amount(a)
    {
    }

    int64_t discipline_id;
    int64_t amount;
};

struct create_grant_operation : public base_operation
{
    account_name_type owner;
    asset balance;

    discipline_name_type target_discipline;
    uint32_t start_block;
    uint32_t end_block;

    bool is_extendable;
    string content_hash;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(owner);
    }
};

struct create_research_group_operation : public base_operation
{
    account_name_type creator;
    std::string name;
    std::string permlink;
    std::string description;
    uint32_t quorum_percent;
    std::map<uint16_t, uint32_t> proposal_quorums;
    bool is_personal;
    vector<invitee_type> invitees;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(creator);
    }
};

struct create_proposal_operation : public base_operation
{
    account_name_type creator;
    int64_t research_group_id;
    string data; ///< must be proper utf8 / JSON string.

    uint16_t action;
    time_point_sec expiration_time;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(creator);
    }
};

struct vote_proposal_operation : public base_operation
{
    account_name_type voter;
    int64_t proposal_id;
    int64_t research_group_id;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(voter);
    }
};

struct make_review_operation : public base_operation
{
    account_name_type author;
    int64_t research_content_id;
    bool is_positive;
    std::string content;
    uint16_t weight;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(author);
    }
};

struct contribute_to_token_sale_operation : public base_operation
{
    int64_t research_token_sale_id;
    account_name_type owner;
    asset amount;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(owner);
    }
};

struct approve_research_group_invite_operation : public base_operation
{
    int64_t research_group_invite_id;
    account_name_type owner;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(owner);
    }
};

struct reject_research_group_invite_operation : public base_operation
{
    int64_t research_group_invite_id;
    account_name_type owner;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(owner);
    }
};

struct transfer_research_tokens_to_research_group_operation : public base_operation
{
    int64_t research_id;
    account_name_type owner;
    uint32_t amount;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(owner);
    }
};

struct set_expertise_tokens_operation : public base_operation
{
    account_name_type owner;
    account_name_type account_name;
    std::vector<expertise_amount_pair_type> disciplines_to_add;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(owner);
    }
};

struct research_update_operation : public base_operation
{
    int64_t research_id;
    string title;
    string abstract;
    string permlink;
    account_name_type owner;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(owner);
    }
};

struct create_vesting_balance_operation : public base_operation
{
    account_name_type creator;  
    account_name_type owner;
    asset balance;
    uint32_t vesting_duration_seconds;
    uint32_t vesting_cliff_seconds;
    uint32_t period_duration_seconds;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(creator);
    }
};

struct withdraw_vesting_balance_operation : public base_operation
{
    int64_t vesting_balance_id;
    account_name_type owner;
    asset amount;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(owner);
    }
};

struct transfer_research_tokens_operation : public base_operation
{
    int64_t research_id;
    account_name_type sender;
    account_name_type receiver;
    uint32_t amount;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(sender);
    }
};

struct delegate_expertise_operation : public base_operation
{
    account_name_type sender;
    account_name_type receiver;
    int64_t discipline_id;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(sender);
    }
};

struct revoke_expertise_delegation_operation : public base_operation
{
    account_name_type sender;
    account_name_type receiver;
    int64_t discipline_id;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(sender);
    }
};

struct accept_research_token_offer_operation : public base_operation
{
    int64_t offer_research_tokens_id;
    account_name_type buyer;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(buyer);
    }
};

struct reject_research_token_offer_operation : public base_operation
{
    int64_t offer_research_tokens_id;
    account_name_type buyer;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(buyer);
    }
};

} // namespace protocol
} // namespace deip

// clang-format off

FC_REFLECT( deip::protocol::chain_properties, (account_creation_fee)(maximum_block_size) )

FC_REFLECT( deip::protocol::account_create_operation,
            (fee)
            (creator)
            (new_account_name)
            (owner)
            (active)
            (posting)
            (memo_key)
            (json_metadata) )

FC_REFLECT( deip::protocol::account_update_operation,
            (account)
            (owner)
            (active)
            (posting)
            (memo_key)
            (json_metadata) )

FC_REFLECT( deip::protocol::transfer_operation, (from)(to)(amount)(memo) )
FC_REFLECT( deip::protocol::transfer_to_common_tokens_operation, (from)(to)(amount) )
FC_REFLECT( deip::protocol::withdraw_common_tokens_operation, (account)(total_common_tokens_amount) )
FC_REFLECT( deip::protocol::set_withdraw_common_tokens_route_operation, (from_account)(to_account)(percent)(auto_common_token) )
FC_REFLECT( deip::protocol::witness_update_operation, (owner)(url)(block_signing_key)(props)(fee) )
FC_REFLECT( deip::protocol::account_witness_vote_operation, (account)(witness)(approve) )
FC_REFLECT( deip::protocol::account_witness_proxy_operation, (account)(proxy) )


FC_REFLECT( deip::protocol::beneficiary_route_type, (account)(weight) )

FC_REFLECT( deip::protocol::request_account_recovery_operation, (recovery_account)(account_to_recover)(new_owner_authority)(extensions) )
FC_REFLECT( deip::protocol::recover_account_operation, (account_to_recover)(new_owner_authority)(recent_owner_authority)(extensions) )
FC_REFLECT( deip::protocol::change_recovery_account_operation, (account_to_recover)(new_recovery_account)(extensions) )

// DEIP native operations
FC_REFLECT( deip::protocol::invitee_type, (account)(research_group_tokens_in_percent)(cover_letter))
FC_REFLECT( deip::protocol::expertise_amount_pair_type, (discipline_id)(amount) )
FC_REFLECT( deip::protocol::create_grant_operation, (owner)(balance)(target_discipline)(start_block)(end_block)(is_extendable)(content_hash) )
FC_REFLECT( deip::protocol::create_research_group_operation, (creator)(name)(permlink)(description)(quorum_percent)(proposal_quorums)(is_personal)(invitees))
FC_REFLECT( deip::protocol::create_proposal_operation, (creator)(research_group_id)(data)(action)(expiration_time))
FC_REFLECT( deip::protocol::vote_proposal_operation, (voter)(proposal_id)(research_group_id))
FC_REFLECT( deip::protocol::make_review_operation, (author)(research_content_id)(content)(is_positive)(weight))

FC_REFLECT( deip::protocol::contribute_to_token_sale_operation, (research_token_sale_id)(owner)(amount))
FC_REFLECT( deip::protocol::approve_research_group_invite_operation, (research_group_invite_id)(owner))
FC_REFLECT( deip::protocol::reject_research_group_invite_operation, (research_group_invite_id)(owner))
FC_REFLECT( deip::protocol::vote_for_review_operation, (voter)(review_id)(discipline_id)(weight))
FC_REFLECT( deip::protocol::transfer_research_tokens_to_research_group_operation, (research_id)(owner)(amount))
FC_REFLECT( deip::protocol::set_expertise_tokens_operation, (owner)(account_name)(disciplines_to_add))
FC_REFLECT( deip::protocol::research_update_operation, (research_id)(title)(abstract)(permlink)(owner))
FC_REFLECT( deip::protocol::create_vesting_balance_operation, (creator)(owner)(balance)(vesting_duration_seconds)(vesting_cliff_seconds)(period_duration_seconds))
FC_REFLECT( deip::protocol::withdraw_vesting_balance_operation, (vesting_balance_id)(owner)(amount))
FC_REFLECT( deip::protocol::transfer_research_tokens_operation, (research_id)(sender)(receiver)(amount))
FC_REFLECT( deip::protocol::delegate_expertise_operation, (sender)(receiver)(discipline_id))
FC_REFLECT( deip::protocol::revoke_expertise_delegation_operation, (sender)(receiver)(discipline_id))
FC_REFLECT( deip::protocol::accept_research_token_offer_operation, (offer_research_tokens_id)(buyer))
FC_REFLECT( deip::protocol::reject_research_token_offer_operation, (offer_research_tokens_id)(buyer))


// clang-format on
