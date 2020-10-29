#pragma once

#include <fc/utf8.hpp>
#include <fc/io/json.hpp>
#include <deip/protocol/base.hpp>
#include <deip/protocol/asset.hpp>
#include <deip/protocol/percent.hpp>
#include <deip/protocol/operations/create_account_operation.hpp>
#include <deip/protocol/operations/update_account_operation.hpp>
#include <deip/protocol/operations/create_review_operation.hpp>
#include <deip/protocol/operations/create_grant_operation.hpp>
#include <deip/protocol/operations/create_award_operation.hpp>
#include <deip/protocol/operations/approve_award_operation.hpp>
#include <deip/protocol/operations/reject_award_operation.hpp>
#include <deip/protocol/operations/create_award_withdrawal_request_operation.hpp>
#include <deip/protocol/operations/certify_award_withdrawal_request_operation.hpp>
#include <deip/protocol/operations/approve_award_withdrawal_request_operation.hpp>
#include <deip/protocol/operations/reject_award_withdrawal_request_operation.hpp>
#include <deip/protocol/operations/pay_award_withdrawal_request_operation.hpp>
#include <deip/protocol/operations/create_nda_contract_operation.hpp>
#include <deip/protocol/operations/sign_nda_contract_operation.hpp>
#include <deip/protocol/operations/decline_nda_contract_operation.hpp>
#include <deip/protocol/operations/close_nda_contract_operation.hpp>
#include <deip/protocol/operations/create_request_by_nda_contract_operation.hpp>
#include <deip/protocol/operations/fulfill_request_by_nda_contract_operation.hpp>
#include <deip/protocol/operations/join_research_group_membership_operation.hpp>
#include <deip/protocol/operations/leave_research_group_membership_operation.hpp>
#include <deip/protocol/operations/create_research_operation.hpp>
#include <deip/protocol/operations/create_research_content_operation.hpp>
#include <deip/protocol/operations/create_research_token_sale_operation.hpp>
#include <deip/protocol/operations/update_research_operation.hpp>
#include <deip/protocol/operations/create_proposal_operation.hpp>
#include <deip/protocol/operations/update_proposal_operation.hpp>
#include <deip/protocol/operations/delete_proposal_operation.hpp>
#include <deip/protocol/operations/transfer_research_share_operation.hpp>
#include <deip/protocol/operations/create_assessment_operation.hpp>
#include <deip/protocol/operations/create_security_token_operation.hpp>
#include <deip/protocol/operations/transfer_security_token_operation.hpp>
#include <deip/protocol/operations/create_research_license_operation.hpp>

namespace deip {
namespace protocol {

inline void validate_payment_number(const string& number)
{
    const int min = 3;
    const int max = 15;
    FC_ASSERT(number.size() >= min && number.size() <= max,
      "Track number length should be in range of ${1} - ${2}",
      ("1", min)("2", max));
}

inline void validate_foa_number(const string& number)
{
    const int min = 3;
    const int max = 15;
    FC_ASSERT(number.size() >= min && number.size() <= max,
      "Track number length should be in range of ${1} - ${2}",
      ("1", min)("2", max));
}

inline void validate_award_number(const string& number)
{
    const int min = 3;
    const int max = 15;
    FC_ASSERT(number.size() >= min && number.size() <= max,
      "Track number length should be in range of ${1} - ${2}",
      ("1", min)("2", max));
}

inline void validate_account_name(const string& name)
{
    FC_ASSERT(is_valid_account_name(name), "Account name ${n} is invalid", ("n", name));
}

inline void validate_permlink(const string& permlink)
{
    FC_ASSERT(permlink.size() < DEIP_MAX_PERMLINK_LENGTH, "permlink is too long");
    FC_ASSERT(fc::is_utf8(permlink), "permlink not formatted in UTF8");
}

inline void validate_enum_value_by_range(uint16_t val, uint16_t first, uint16_t last)
{
    FC_ASSERT(val >= first && val <= last,
      "Provided enum value is outside of the range: val: ${1}; FIRST: ${2}; LAST: ${3}",
      ("1", val)("2", first)("3", last));
}

inline void validate_520_bits_hexadecimal_string(const string& str)
{
    FC_ASSERT(((str.size() / 2) == 65), "Provided value must be a lowercase hexadecimal string 520 bits in length");
    FC_ASSERT(std::all_of(str.begin(), str.end(), ::isxdigit), "Provided value must be a lowercase hexadecimal string 520 bits in length");
    FC_ASSERT(std::all_of(str.begin(), str.end(), [](char ch) { return isdigit(ch) ? true : islower(ch); }), "Provided value must be a lowercase hexadecimal string 520 bits in length");
}

inline void validate_256_bits_hexadecimal_string(const string& str)
{
    FC_ASSERT(((str.size() / 2) == 32), "Provided value must be a lowercase hexadecimal string 256 bits in length");
    FC_ASSERT(std::all_of(str.begin(), str.end(), ::isxdigit), "Provided value must be a lowercase hexadecimal string 256 bits in length");
    FC_ASSERT(std::all_of(str.begin(), str.end(), [](char ch) { return isdigit(ch) ? true : islower(ch); }), "Provided value must be a lowercase hexadecimal string 256 bits in length");
}

inline void validate_160_bits_hexadecimal_string(const string &str)
{
    FC_ASSERT(((str.size() / 2) == 20), "Provided value must be a lowercase hexadecimal string 160 bits in length");
    FC_ASSERT(std::all_of(str.begin(), str.end(), ::isxdigit), "Provided value must be a lowercase hexadecimal string 160 bits in length");
    FC_ASSERT(std::all_of(str.begin(), str.end(), [](char ch) { return isdigit(ch) ? true : islower(ch); }), "Provided value must be a lowercase hexadecimal string 160 bits in length");
}

inline void validate_128_bits_hexadecimal_string(const string& str)
{
    FC_ASSERT(((str.size() / 2) == 16), "Provided value must be a lowercase hexadecimal string 128 bits in length");
    FC_ASSERT(std::all_of(str.begin(), str.end(), ::isxdigit), "Provided value must be a lowercase hexadecimal string 128 bits in length");
    FC_ASSERT(std::all_of(str.begin(), str.end(), [](char ch) { return isdigit(ch) ? true : islower(ch); }), "Provided value must be a lowercase hexadecimal string 128 bits in length");
}

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

struct vote_for_review_operation : public entity_operation
{
    external_id_type external_id;
    account_name_type voter;
    external_id_type review_external_id;
    external_id_type discipline_external_id;
    percent weight;

    extensions_type extensions;

    string entity_id() const { return "external_id"; }
    external_id_type get_entity_id() const { return external_id; }

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
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

    extensions_type extensions;

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

    extensions_type extensions;

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
    
    extensions_type extensions;

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

    extensions_type extensions;

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
    asset account_creation_fee = DEIP_MIN_ACCOUNT_CREATION_FEE;

    /**
     *  This witnesses vote for the maximum_block_size which is used by the network
     *  to tune rate limiting and capacity
     */
    uint32_t maximum_block_size = DEIP_MIN_BLOCK_SIZE_LIMIT * 2;

    void validate() const
    {
        FC_ASSERT(account_creation_fee >= DEIP_MIN_ACCOUNT_CREATION_FEE);
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
    
    extensions_type extensions;
    
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

    extensions_type extensions;

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

    extensions_type extensions;

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
    account_name_type recovery_account; ///< The recovery account is listed as the recovery account on the account to recover.
    account_name_type account_to_recover; ///< The account to recover. This is likely due to a compromised owner authority.
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

struct contribute_to_token_sale_operation : public base_operation
{
    external_id_type token_sale_external_id;
    external_id_type research_external_id;
    account_name_type contributor;
    asset amount;

    extensions_type extensions; ///< Extensions. Not currently used.

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(contributor);
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

    extensions_type extensions;

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

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(owner);
    }
};

struct create_expertise_allocation_proposal_operation : public base_operation
{
    account_name_type claimer;
    int64_t discipline_id;
    string description;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(claimer);
    }
};

struct vote_for_expertise_allocation_proposal_operation : public base_operation
{
    int64_t proposal_id;
    account_name_type voter;
    share_type voting_power;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(voter);
    }
};

struct create_grant_application_operation : public base_operation
{
    external_id_type funding_opportunity_number;
    int64_t research_id;
    account_name_type creator;
    string application_hash;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(creator);
    }
};

struct create_review_for_application_operation : public base_operation
{
    account_name_type author;
    int64_t grant_application_id;
    bool is_positive;
    std::string content;
    uint16_t weight;

    extensions_type extensions;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(author);
    }
};

struct approve_grant_application_operation : public base_operation
{
    int64_t grant_application_id;
    account_name_type approver;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(approver);
    }
};

struct reject_grant_application_operation : public base_operation
{
    int64_t grant_application_id;
    account_name_type rejector;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(rejector);
    }
};

struct create_asset_operation : public base_operation
{
    account_name_type issuer;
    string symbol;
    uint8_t precision;
    string name;
    string description;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(issuer);
    }
};

struct issue_asset_operation : public base_operation
{
    account_name_type issuer;
    asset amount;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(issuer);
    }
};

struct reserve_asset_operation : public base_operation
{
    account_name_type owner;
    asset amount;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(owner);
    }
};

} // namespace protocol
} // namespace deip

// clang-format off

FC_REFLECT( deip::protocol::chain_properties, (account_creation_fee)(maximum_block_size) )

FC_REFLECT( deip::protocol::transfer_operation, (from)(to)(amount)(memo)(extensions) )
FC_REFLECT( deip::protocol::transfer_to_common_tokens_operation, (from)(to)(amount)(extensions) )
FC_REFLECT( deip::protocol::withdraw_common_tokens_operation, (account)(total_common_tokens_amount)(extensions) )
FC_REFLECT( deip::protocol::set_withdraw_common_tokens_route_operation, (from_account)(to_account)(percent)(auto_common_token)(extensions) )
FC_REFLECT( deip::protocol::witness_update_operation, (owner)(url)(block_signing_key)(props)(fee)(extensions) )
FC_REFLECT( deip::protocol::account_witness_vote_operation, (account)(witness)(approve)(extensions) )
FC_REFLECT( deip::protocol::account_witness_proxy_operation, (account)(proxy)(extensions) )


FC_REFLECT( deip::protocol::beneficiary_route_type, (account)(weight) )

FC_REFLECT( deip::protocol::request_account_recovery_operation, (recovery_account)(account_to_recover)(new_owner_authority)(extensions) )
FC_REFLECT( deip::protocol::recover_account_operation, (account_to_recover)(new_owner_authority)(recent_owner_authority)(extensions) )
FC_REFLECT( deip::protocol::change_recovery_account_operation, (account_to_recover)(new_recovery_account)(extensions) )

// DEIP native operations
FC_REFLECT( deip::protocol::contribute_to_token_sale_operation, (token_sale_external_id)(research_external_id)(contributor)(amount)(extensions))
FC_REFLECT( deip::protocol::vote_for_review_operation, (external_id)(voter)(review_external_id)(discipline_external_id)(weight)(extensions))
FC_REFLECT( deip::protocol::create_vesting_balance_operation, (creator)(owner)(balance)(vesting_duration_seconds)(vesting_cliff_seconds)(period_duration_seconds)(extensions))
FC_REFLECT( deip::protocol::withdraw_vesting_balance_operation, (vesting_balance_id)(owner)(amount)(extensions))
FC_REFLECT( deip::protocol::create_expertise_allocation_proposal_operation, (claimer)(discipline_id)(description)(extensions))
FC_REFLECT( deip::protocol::vote_for_expertise_allocation_proposal_operation, (proposal_id)(voter)(voting_power)(extensions))
FC_REFLECT( deip::protocol::create_grant_application_operation, (funding_opportunity_number)(research_id)(creator)(application_hash)(extensions))
FC_REFLECT( deip::protocol::create_review_for_application_operation, (author)(grant_application_id)(content)(is_positive)(weight)(extensions))
FC_REFLECT( deip::protocol::approve_grant_application_operation, (grant_application_id)(approver)(extensions))
FC_REFLECT( deip::protocol::reject_grant_application_operation, (grant_application_id)(rejector)(extensions))
FC_REFLECT( deip::protocol::create_asset_operation, (issuer)(symbol)(precision)(name)(description)(extensions))
FC_REFLECT( deip::protocol::issue_asset_operation, (issuer)(amount)(extensions))
FC_REFLECT( deip::protocol::reserve_asset_operation, (owner)(amount)(extensions))

// clang-format on