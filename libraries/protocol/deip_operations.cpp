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

void create_discipline_supply_operation::validate() const
{
    validate_account_name(owner);
    FC_ASSERT(is_asset_type(balance, DEIP_SYMBOL), "Balance must be DEIP");
    FC_ASSERT(balance > asset(0, DEIP_SYMBOL), "Balance must be positive");
    FC_ASSERT(content_hash.size() > 0, "Content hash must be specified");
    FC_ASSERT(fc::is_utf8(content_hash), "Content hash is not valid UTF8 string");
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
    FC_ASSERT(quorum_percent >= 5 * DEIP_1_PERCENT && quorum_percent <= DEIP_100_PERCENT, "Default proposal quorum must be in 0% to 100% range");
        for(auto& quorum_percent : proposal_quorums)
        FC_ASSERT(quorum_percent.second > 5 * DEIP_1_PERCENT && quorum_percent.second <= DEIP_100_PERCENT, "Quorum percent must be in 5% to 100% range");

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
    FC_ASSERT(weight > 0 && weight <= DEIP_100_PERCENT, "Weight should be in 1% to 100% range");
    FC_ASSERT(!content.empty(), "Research content cannot be empty");
}

void contribute_to_token_sale_operation::validate() const
{
    validate_account_name(owner);
    FC_ASSERT(amount.amount > 0, "Amount must be greater than 0");
    FC_ASSERT(amount.symbol == DEIP_SYMBOL, "Incorrect asset symbol");
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

void set_expertise_tokens_operation::validate() const
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

void create_vesting_balance_operation::validate() const
{
    FC_ASSERT(balance > asset(0, DEIP_SYMBOL), "Deposit balance must be greater than 0");
    FC_ASSERT(vesting_duration_seconds > 0 && vesting_duration_seconds > vesting_cliff_seconds,
            "Vesting  duration must be longer than 0 & longer than cliff period");
    FC_ASSERT(vesting_cliff_seconds >= 0, "Vesting cliff period should be equal or greater than 0");
    FC_ASSERT(period_duration_seconds > 0, "Vesting withdraw periods duration should be greater than 0");
    FC_ASSERT(vesting_duration_seconds % period_duration_seconds == 0,
            "Vesting duration should contain an integer number of withdraw periods");
    validate_account_name(creator);
    validate_account_name(owner);
}

void withdraw_vesting_balance_operation::validate() const
{
    FC_ASSERT(amount > asset(0, DEIP_SYMBOL), "Withdraw amount must be greater than 0");
    validate_account_name(owner);
}

void transfer_research_tokens_operation::validate() const
{
    FC_ASSERT(amount > 0, "Transfer amount must be greater than 0");
    validate_account_name(sender);
    validate_account_name(receiver);
}

void delegate_expertise_operation::validate() const
{
    FC_ASSERT(discipline_id > 0, "Cannot use root discipline (id = 0)");
    validate_account_name(sender);
    validate_account_name(receiver);
}

void revoke_expertise_delegation_operation::validate() const
{
    validate_account_name(sender);
    FC_ASSERT(discipline_id > 0, "Cannot use root discipline (id = 0)");
}

void create_expertise_allocation_proposal_operation::validate() const
{
    validate_account_name(claimer);
    FC_ASSERT(discipline_id > 0, "Cannot use root discipline (id = 0)");
    FC_ASSERT(description.size() > 0, "Description must be specified");
    FC_ASSERT(fc::is_utf8(description), "Description is not valid UTF8 string");
}

void vote_for_expertise_allocation_proposal_operation::validate() const
{
    validate_account_name(voter);
    FC_ASSERT(voting_power == DEIP_100_PERCENT || voting_power == -DEIP_100_PERCENT, "Voting power must be -100% or +100%");
}

void accept_research_token_offer_operation::validate() const
{
    validate_account_name(buyer);
}


void reject_research_token_offer_operation::validate() const
{
    validate_account_name(buyer);
}

void create_grant_operation::validate() const
{
    FC_ASSERT(target_discipline > 0, "Cannot use root discipline (id = 0)");
    FC_ASSERT(amount > asset(0, DEIP_SYMBOL), "Grant amount must be greater than 0");
    FC_ASSERT(min_number_of_positive_reviews >= 0, "Number of positive reviews must be equal or greater than 0");
    FC_ASSERT(min_number_of_applications > 0, "Number of applications must be greater than 0");
    FC_ASSERT(min_number_of_applications >= researches_to_grant, "Number of applications must be equal or greater than number of researches");
    FC_ASSERT(researches_to_grant > 0, "Number of researches must be greater than 0");
    FC_ASSERT(end_time > start_time, "End time must be greater than a start time");
    validate_account_name(owner);
}

void create_grant_application_operation::validate() const
{
    validate_account_name(creator);
    FC_ASSERT(application_hash.size() > 0, "Application hash must be specified");
    FC_ASSERT(fc::is_utf8(application_hash), "Application hash is not valid UTF8 string");
}

void add_member_to_research_operation::validate() const
{
    validate_account_name(owner);
    validate_account_name(invitee);
}

void exclude_member_from_research_operation::validate() const
{
    validate_account_name(owner);
    validate_account_name(account_to_exclude);
}

void create_contract_operation::validate() const
{
    validate_account_name(creator);
    validate_account_name(signee);
    FC_ASSERT(title.size() > 0 && title.size() < 200, "Contract title must be specified in length from 1 to 200 characters");
    FC_ASSERT(((contract_hash.size() / 2) == 256 / 8), "Contract hash must be a hexadecimal string 256 bits in length");
    FC_ASSERT(std::all_of(contract_hash.begin(), contract_hash.end(), ::isxdigit), "Contract hash must be a hexadecimal string 256 bits in length");
    FC_ASSERT(end_date > start_date, "End time must be greater than a start time");
}

void sign_contract_operation::validate() const
{
    validate_account_name(contract_signer);
    FC_ASSERT(signature.size() > 0, "Signature of contract hash from signee is required");
}

void decline_contract_operation::validate() const
{
    validate_account_name(signee);
}

void close_contract_operation::validate() const
{
    validate_account_name(creator);
}

void request_contract_file_key_operation::validate() const
{
    validate_account_name(requester);
    FC_ASSERT(encrypted_payload_hash.size() > 0, "Payload hash must be specified");
    FC_ASSERT(fc::is_utf8(encrypted_payload_hash), "Payload hash is not valid UTF8 string");
    FC_ASSERT(initialization_vector.size() > 0, "IV must be specified");
    FC_ASSERT(fc::is_utf8(initialization_vector), "IV is not valid UTF8 string");
}

void grant_access_to_contract_file_operation::validate() const
{
    validate_account_name(granter);
    FC_ASSERT(encrypted_payload_hash.size() > 0, "Payload hash must be specified");
    FC_ASSERT(fc::is_utf8(encrypted_payload_hash), "Payload hash is not valid UTF8 string");
    FC_ASSERT(initialization_vector.size() > 0, "IV must be specified");
    FC_ASSERT(fc::is_utf8(initialization_vector), "IV is not valid UTF8 string");
}

}
} // deip::protocol
