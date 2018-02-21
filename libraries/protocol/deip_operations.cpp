#include <deip/protocol/deip_operations.hpp>
#include <fc/io/json.hpp>

#include <locale>

namespace deip {
namespace protocol {

bool inline is_asset_type(asset asset, asset_symbol_type symbol)
{
    return asset.symbol == symbol;
}

void account_create_operation::validate() const
{
    validate_account_name(new_account_name);
    FC_ASSERT(is_asset_type(fee, DEIP_SYMBOL), "Account creation fee must be DEIP");
    owner.validate();
    active.validate();

    if (json_metadata.size() > 0)
    {
        FC_ASSERT(fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8");
        FC_ASSERT(fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON");
    }
    FC_ASSERT(fee >= asset(0, DEIP_SYMBOL), "Account creation fee cannot be negative");
}

void account_create_with_delegation_operation::validate() const
{
    validate_account_name(new_account_name);
    validate_account_name(creator);
    FC_ASSERT(is_asset_type(fee, DEIP_SYMBOL), "Account creation fee must be DEIP");
    FC_ASSERT(is_asset_type(delegation, VESTS_SYMBOL), "Delegation must be VESTS");

    owner.validate();
    active.validate();
    posting.validate();

    if (json_metadata.size() > 0)
    {
        FC_ASSERT(fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8");
        FC_ASSERT(fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON");
    }

    FC_ASSERT(fee >= asset(0, DEIP_SYMBOL), "Account creation fee cannot be negative");
    FC_ASSERT(delegation >= asset(0, VESTS_SYMBOL), "Delegation cannot be negative");
}

void account_update_operation::validate() const
{
    validate_account_name(account);
    /*if( owner )
       owner->validate();
    if( active )
       active->validate();
    if( posting )
       posting->validate();*/

    if (json_metadata.size() > 0)
    {
        FC_ASSERT(fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8");
        FC_ASSERT(fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON");
    }
}

void comment_operation::validate() const
{
    FC_ASSERT(title.size() < 256, "Title larger than size limit");
    FC_ASSERT(fc::is_utf8(title), "Title not formatted in UTF8");
    FC_ASSERT(body.size() > 0, "Body is empty");
    FC_ASSERT(fc::is_utf8(body), "Body not formatted in UTF8");

    if (parent_author.size())
        validate_account_name(parent_author);
    validate_account_name(author);
    validate_permlink(parent_permlink);
    validate_permlink(permlink);

    if (json_metadata.size() > 0)
    {
        FC_ASSERT(fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON");
    }
}

struct comment_options_extension_validate_visitor
{
    comment_options_extension_validate_visitor()
    {
    }

    typedef void result_type;

    void operator()(const comment_payout_beneficiaries& cpb) const
    {
        cpb.validate();
    }
};

void comment_payout_beneficiaries::validate() const
{
    uint32_t sum = 0;

    FC_ASSERT(beneficiaries.size(), "Must specify at least one beneficiary");
    FC_ASSERT(beneficiaries.size() < 128,
              "Cannot specify more than 127 beneficiaries."); // Require size serializtion fits in one byte.

    validate_account_name(beneficiaries[0].account);
    FC_ASSERT(beneficiaries[0].weight <= DEIP_100_PERCENT,
              "Cannot allocate more than 100% of rewards to one account");
    sum += beneficiaries[0].weight;
    FC_ASSERT(
        sum <= DEIP_100_PERCENT,
        "Cannot allocate more than 100% of rewards to a comment"); // Have to check incrementally to avoid overflow

    for (size_t i = 1; i < beneficiaries.size(); i++)
    {
        validate_account_name(beneficiaries[i].account);
        FC_ASSERT(beneficiaries[i].weight <= DEIP_100_PERCENT,
                  "Cannot allocate more than 100% of rewards to one account");
        sum += beneficiaries[i].weight;
        FC_ASSERT(
            sum <= DEIP_100_PERCENT,
            "Cannot allocate more than 100% of rewards to a comment"); // Have to check incrementally to avoid overflow
        FC_ASSERT(beneficiaries[i - 1] < beneficiaries[i],
                  "Benficiaries must be specified in sorted order (account ascending)");
    }
}

void comment_options_operation::validate() const
{
    validate_account_name(author);
    FC_ASSERT(percent_deips <= DEIP_100_PERCENT, "Percent cannot exceed 100%");
    FC_ASSERT(max_accepted_payout.symbol == DEIP_SYMBOL, "Max accepted payout must be in DEIP");
    FC_ASSERT(max_accepted_payout.amount.value >= 0, "Cannot accept less than 0 payout");
    validate_permlink(permlink);
    for (auto& e : extensions)
        e.visit(comment_options_extension_validate_visitor());
}

void delete_comment_operation::validate() const
{
    validate_permlink(permlink);
    validate_account_name(author);
}

void prove_authority_operation::validate() const
{
    validate_account_name(challenged);
}

void vote_operation::validate() const
{
    validate_account_name(voter);
    FC_ASSERT(weight > 0 && weight <= DEIP_100_PERCENT, "Weight should be in 1% to 100% range");
    validate_permlink(permlink);
}

void transfer_operation::validate() const
{
    try
    {
        validate_account_name(from);
        validate_account_name(to);
        FC_ASSERT(amount.symbol != VESTS_SYMBOL, "transferring of Deip Power (STMP) is not allowed.");
        FC_ASSERT(amount.amount > 0, "Cannot transfer a negative amount (aka: stealing)");
        FC_ASSERT(memo.size() < DEIP_MAX_MEMO_SIZE, "Memo is too large");
        FC_ASSERT(fc::is_utf8(memo), "Memo is not UTF8");
    }
    FC_CAPTURE_AND_RETHROW((*this))
}

void transfer_to_vesting_operation::validate() const
{
    validate_account_name(from);
    FC_ASSERT(is_asset_type(amount, DEIP_SYMBOL), "Amount must be DEIP");
    if (to != account_name_type())
        validate_account_name(to);
    FC_ASSERT(amount > asset(0, DEIP_SYMBOL), "Must transfer a nonzero amount");
}

void withdraw_vesting_operation::validate() const
{
    validate_account_name(account);
    FC_ASSERT(is_asset_type(vesting_shares, VESTS_SYMBOL), "Amount must be VESTS");
}

void set_withdraw_vesting_route_operation::validate() const
{
    validate_account_name(from_account);
    validate_account_name(to_account);
    FC_ASSERT(0 <= percent && percent <= DEIP_100_PERCENT, "Percent must be valid deip percent");
}

void witness_update_operation::validate() const
{
    validate_account_name(owner);
    FC_ASSERT(url.size() > 0, "URL size must be greater than 0");
    FC_ASSERT(fc::is_utf8(url), "URL is not valid UTF8");
    FC_ASSERT(fee >= asset(0, DEIP_SYMBOL), "Fee cannot be negative");
    props.validate();
}

void account_witness_vote_operation::validate() const
{
    validate_account_name(account);
    validate_account_name(witness);
}

void account_witness_proxy_operation::validate() const
{
    validate_account_name(account);
    if (proxy.size())
        validate_account_name(proxy);
    FC_ASSERT(proxy != account, "Cannot proxy to self");
}

void escrow_transfer_operation::validate() const
{
    validate_account_name(from);
    validate_account_name(to);
    validate_account_name(agent);
    FC_ASSERT(fee.amount >= 0, "fee cannot be negative");
    FC_ASSERT(deip_amount.amount > 0, "deip amount cannot be negative");
    FC_ASSERT(from != agent && to != agent, "agent must be a third party");
    FC_ASSERT(fee.symbol == DEIP_SYMBOL, "fee must be DEIP");
    FC_ASSERT(deip_amount.symbol == DEIP_SYMBOL, "deip amount must contain DEIP");
    FC_ASSERT(ratification_deadline < escrow_expiration, "ratification deadline must be before escrow expiration");
    if (json_meta.size() > 0)
    {
        FC_ASSERT(fc::is_utf8(json_meta), "JSON Metadata not formatted in UTF8");
        FC_ASSERT(fc::json::is_valid(json_meta), "JSON Metadata not valid JSON");
    }
}

void escrow_approve_operation::validate() const
{
    validate_account_name(from);
    validate_account_name(to);
    validate_account_name(agent);
    validate_account_name(who);
    FC_ASSERT(who == to || who == agent, "to or agent must approve escrow");
}

void escrow_dispute_operation::validate() const
{
    validate_account_name(from);
    validate_account_name(to);
    validate_account_name(agent);
    validate_account_name(who);
    FC_ASSERT(who == from || who == to, "who must be from or to");
}

void escrow_release_operation::validate() const
{
    validate_account_name(from);
    validate_account_name(to);
    validate_account_name(agent);
    validate_account_name(who);
    validate_account_name(receiver);
    FC_ASSERT(who == from || who == to || who == agent, "who must be from or to or agent");
    FC_ASSERT(receiver == from || receiver == to, "receiver must be from or to");
    FC_ASSERT(deip_amount.amount >= 0, "deip amount cannot be negative");
    FC_ASSERT(deip_amount.symbol == DEIP_SYMBOL, "deip amount must contain DEIP");
}

void request_account_recovery_operation::validate() const
{
    validate_account_name(recovery_account);
    validate_account_name(account_to_recover);
    new_owner_authority.validate();
}

void recover_account_operation::validate() const
{
    validate_account_name(account_to_recover);
    FC_ASSERT(!(new_owner_authority == recent_owner_authority),
              "Cannot set new owner authority to the recent owner authority");
    FC_ASSERT(!new_owner_authority.is_impossible(), "new owner authority cannot be impossible");
    FC_ASSERT(!recent_owner_authority.is_impossible(), "recent owner authority cannot be impossible");
    FC_ASSERT(new_owner_authority.weight_threshold, "new owner authority cannot be trivial");
    new_owner_authority.validate();
    recent_owner_authority.validate();
}

void change_recovery_account_operation::validate() const
{
    validate_account_name(account_to_recover);
    validate_account_name(new_recovery_account);
}

void decline_voting_rights_operation::validate() const
{
    validate_account_name(account);
}

void delegate_vesting_shares_operation::validate() const
{
    validate_account_name(delegator);
    validate_account_name(delegatee);
    FC_ASSERT(delegator != delegatee, "You cannot delegate VESTS to yourself");
    FC_ASSERT(is_asset_type(vesting_shares, VESTS_SYMBOL), "Delegation must be VESTS");
    FC_ASSERT(vesting_shares >= asset(0, VESTS_SYMBOL), "Delegation cannot be negative");
}

void create_budget_operation::validate() const
{
    validate_account_name(owner);
    FC_ASSERT(is_asset_type(balance, DEIP_SYMBOL), "Balance must be DEIP");
    FC_ASSERT(balance > asset(0, DEIP_SYMBOL), "Balance must be positive");
}

void create_proposal_operation::validate() const
{
    validate_enum_value_by_range(action, proposal_action_type::First, proposal_action_type::Last);
    validate_account_name(creator);
    FC_ASSERT(expiration_time > fc::time_point_sec());
    FC_ASSERT(fc::is_utf8(data), "Data is not valid UTF8 string");
}

void create_research_group_operation::validate() const
{
    validate_account_name(creator);
    validate_permlink(permlink);
    FC_ASSERT(fc::is_utf8(desciption), "Description is not valid UTF8 string");
    FC_ASSERT(quorum_percent > 5 && quorum_percent <= 100, "Quorum percent must be in 5% to 100& range");
    FC_ASSERT(tokens_amount > 0, "Initial research group tokens amount must be greater than 0");
}

void vote_proposal_operation::validate() const
{
    validate_account_name(voter);
}

void make_research_review_operation::validate() const
{
    validate_account_name(author);
    FC_ASSERT(!content.empty(), "Research content cannot be empty");
    for (auto& link : research_external_references)
    {
        FC_ASSERT(!link.empty(), "External reference link cannot be empty");
        FC_ASSERT(fc::is_utf8(link), "External reference is not valid UTF8 string");
    }
}

void contribute_to_token_sale_operation::validate() const
{
    validate_account_name(owner);
    FC_ASSERT(amount > 0, "Amount must be bigger than 0");
}

}
} // deip::protocol
