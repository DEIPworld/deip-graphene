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

void vote_operation::validate() const
{
    validate_account_name(voter);
    FC_ASSERT(weight > 0 && weight <= DEIP_100_PERCENT, "Weight should be in 1% to 100% range");
}

void vote_for_review_operation::validate() const
{
    validate_account_name(voter);
    FC_ASSERT(weight > 0 && weight <= DEIP_100_PERCENT, "Weight should be in 1% to 100% range");
    FC_ASSERT(discipline_id != 0, "You cannot vote with common token");
}

void transfer_operation::validate() const
{
    try
    {
        validate_account_name(from);
        validate_account_name(to);

        FC_ASSERT(amount.symbol == DEIP_SYMBOL, "Only transferring of Deip (DEIP) token is allowed.");
        
        FC_ASSERT(amount.amount > 0, "Cannot transfer a negative amount (aka: stealing)");
        FC_ASSERT(memo.size() < DEIP_MAX_MEMO_SIZE, "Memo is too large");
        FC_ASSERT(fc::is_utf8(memo), "Memo is not UTF8");
    }
    FC_CAPTURE_AND_RETHROW((*this))
}

void transfer_to_common_tokens_operation::validate() const
{
    validate_account_name(from);
    FC_ASSERT(is_asset_type(amount, DEIP_SYMBOL), "Amount must be DEIP");
    if (to != account_name_type())
        validate_account_name(to);
    FC_ASSERT(amount > asset(0, DEIP_SYMBOL), "Must transfer a nonzero amount");
}

void withdraw_common_tokens_operation::validate() const
{
    validate_account_name(account);

    // TODO Add Common token validation
    // FC_ASSERT(is_asset_type(vesting_shares, VESTS_SYMBOL), "Amount must be VESTS");
}

void set_withdraw_common_tokens_route_operation::validate() const
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

void create_grant_operation::validate() const
{
    validate_account_name(owner);
    FC_ASSERT(is_asset_type(balance, DEIP_SYMBOL), "Balance must be DEIP");
    FC_ASSERT(balance > asset(0, DEIP_SYMBOL), "Balance must be positive");
}

void create_proposal_operation::validate() const
{
    validate_enum_value_by_range(action, proposal_action_type::First_proposal, proposal_action_type::Last_proposal);
    validate_account_name(creator);
    FC_ASSERT(expiration_time > fc::time_point_sec());
    FC_ASSERT(fc::is_utf8(data), "Data is not valid UTF8 string");
}

void create_research_group_operation::validate() const
{
    validate_account_name(creator);
    validate_permlink(permlink);
    FC_ASSERT(name.size() > 0, "Group name must be specified");
    FC_ASSERT(fc::is_utf8(name), "Group name is not valid UTF8 string");
    FC_ASSERT(fc::is_utf8(description), "Description is not valid UTF8 string");
    FC_ASSERT(quorum_percent > 5 * DEIP_1_PERCENT && quorum_percent <= DEIP_100_PERCENT, "Quorum percent must be in 5% to 100% range");

    auto total_tokens_percents = share_type(0);
    for (auto& invitee : invitees) {
        validate_account_name(invitee.account);
        FC_ASSERT(invitee.research_group_tokens_in_percent > 0
                  && invitee.research_group_tokens_in_percent <= 95 * DEIP_1_PERCENT,
                  "Invitee should receive 0% to 95% of research group tokens");
        total_tokens_percents += invitee.research_group_tokens_in_percent;
    }
    FC_ASSERT(total_tokens_percents <= 95 * DEIP_1_PERCENT);
}

void vote_proposal_operation::validate() const
{
    validate_account_name(voter);
}

void make_review_operation::validate() const
{
    validate_account_name(author);
    FC_ASSERT(!content.empty(), "Research content cannot be empty");
}

void contribute_to_token_sale_operation::validate() const
{
    validate_account_name(owner);
    FC_ASSERT(amount > 0, "Amount must be greater than 0");
}

void approve_research_group_invite_operation::validate() const
{
    validate_account_name(owner);
}

void reject_research_group_invite_operation::validate() const
{
    validate_account_name(owner);
}

void transfer_research_tokens_to_research_group_operation::validate() const 
{
    FC_ASSERT(amount > 0, "Transfer amount must be greater than 0");
    validate_account_name(owner);
}    

void add_expertise_tokens_operation::validate() const
{
    validate_account_name(owner);
    validate_account_name(account_name);
    FC_ASSERT(!disciplines_to_add.empty(), "List of disciplines to adjust cannot be empty");
}

void research_update_operation::validate() const
{
    FC_ASSERT(!title.empty(), "Title cannot be empty");
    FC_ASSERT(!abstract.empty(), "Abstract cannot be empty");
    FC_ASSERT(!permlink.empty(), "Permlink cannot be empty");
    validate_account_name(owner);
}

void deposit_to_vesting_contract_operation::validate() const
{
    FC_ASSERT(balance > 0, "Deposit balance must be greater than 0");
    FC_ASSERT(withdrawal_period > 0, "You must divide contract at least by 1 part");
    FC_ASSERT(contract_duration > 0, "Contract duration must be longer than 0");
    validate_account_name(sender);
    validate_account_name(receiver);
}

void withdraw_from_vesting_contract_operation::validate() const
{
    FC_ASSERT(amount > 0, "Withdraw amount must be greater than 0");
    validate_account_name(sender);
    validate_account_name(receiver);
}

void transfer_research_tokens_operation::validate() const
{
    FC_ASSERT(amount > 0, "Transfer amount must be greater than 0");
    validate_account_name(sender);
    validate_account_name(receiver);
}

}
} // deip::protocol
