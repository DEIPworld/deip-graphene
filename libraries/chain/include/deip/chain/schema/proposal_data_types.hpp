#pragma once

#include "deip_object_types.hpp"
#include <fc/utf8.hpp>
#include <deip/protocol/asset.hpp>

using namespace deip::protocol;

namespace deip {
namespace chain {

struct base_proposal_data_type
{
    virtual void validate() const = 0;
};

struct dropout_member_proposal_data_type : base_proposal_data_type
{
    account_name_type name;

    void validate() const
    {
        FC_ASSERT(is_valid_account_name(name), "Account name ${n} is invalid", ("n", name));
    }
};

struct invite_member_proposal_data_type : base_proposal_data_type
{
    deip::protocol::account_name_type name;
    share_type research_group_token_amount_in_percent;
    std::string cover_letter;
    bool is_head;

    void validate() const
    {
        FC_ASSERT(is_valid_account_name(name), "Account name ${n} is invalid", ("n", name));
        FC_ASSERT(research_group_token_amount_in_percent > 0 && research_group_token_amount_in_percent <= DEIP_100_PERCENT, "Research group tokens amount should be > 0");
        FC_ASSERT(fc::is_utf8(cover_letter), "Cover letter should be valid UTF8 string");
    }
};

struct change_action_quorum_proposal_data_type : base_proposal_data_type
{
    deip::protocol::research_group_quorum_action action;
    deip::protocol::percent_type quorum;

    void validate() const
    {
        const percent_type min = DEIP_1_PERCENT;
        const percent_type max = DEIP_100_PERCENT;
        FC_ASSERT(quorum >= min && quorum <= max, 
          "Quorum value (${val}) should be in range of ${min} to ${max}. ", 
          ("min", min)("max", max)("val", quorum));
    }
};

struct start_research_proposal_data_type : base_proposal_data_type
{
    string title;
    string abstract;
    string permlink;
    uint16_t review_share_in_percent;
    uint16_t dropout_compensation_in_percent;
    std::set<int64_t> disciplines;

    bool is_private;

    void validate() const
    {
        FC_ASSERT(disciplines.size() != 0, "Research must be related to one or several disciplines");
        FC_ASSERT(!std::any_of(disciplines.begin(), disciplines.end(), [](int64_t discipline_id){
            return discipline_id == 0;
        }), "Research cannot be related to 'common' discipline");

        FC_ASSERT(!title.empty(), "Research name cannot be empty");
        FC_ASSERT(!abstract.empty(), "Research abstract cannot be empty");
        FC_ASSERT(permlink.size() < DEIP_MAX_PERMLINK_LENGTH, "Research permlink is too long");
        FC_ASSERT(fc::is_utf8(permlink), "Research permlink should be valid UTF8 string");
        FC_ASSERT(review_share_in_percent >= 0 && review_share_in_percent <= 50 * DEIP_1_PERCENT, "Percent for review should be in 0 to 50 range");
        FC_ASSERT(dropout_compensation_in_percent >= 0 && dropout_compensation_in_percent <= DEIP_100_PERCENT, "Percent for dropout compensation should be in 0 to 100 range");
    }
};

struct send_funds_data_type : base_proposal_data_type
{
    account_name_type recipient;
    asset funds;

    void validate() const
    {
        FC_ASSERT(is_valid_account_name(recipient), "Account name ${n} is invalid", ("n", recipient));
        FC_ASSERT(funds > asset(0, DEIP_SYMBOL), "Amount must be greater than 0");
    }
};

struct rebalance_info
{
    account_name_type account_name;
    share_type new_amount_in_percent;
};

struct rebalance_research_group_tokens_data_type : base_proposal_data_type
{
    std::vector<rebalance_info> accounts;

    void validate() const
    {
        int size = accounts.size();
        share_type total_amount = 0;
        for (int i = 0; i < size; ++i)
        {
            FC_ASSERT(is_valid_account_name(accounts[i].account_name), "Account name ${n} is invalid",
                      ("n", accounts[i].account_name));
            total_amount += accounts[i].new_amount_in_percent;
        }
        FC_ASSERT(total_amount == DEIP_100_PERCENT, "New total amount must be equal to 100%");
    }
};

struct create_research_content_data_type : base_proposal_data_type
{
    research_id_type research_id;
    research_content_type type;
    string title;
    string content;
    string permlink;
    std::set<account_name_type> authors;
    std::set<research_content_id_type> references;
    std::set<string> external_references;

    void validate() const
    {
        FC_ASSERT(!content.empty(), "Content cannot be empty");
        FC_ASSERT(!permlink.empty(), "Permlink cannot be empty");
        FC_ASSERT(fc::is_utf8(permlink), "Permlink is not valid UTF8 string");
        FC_ASSERT(!authors.empty(), "Content should have author(s)");
        for (auto& author : authors)
        {
            FC_ASSERT(is_valid_account_name(author), "Account name ${n} is invalid", ("n", author));
        }

        for (auto& ref : external_references)
        {
            FC_ASSERT(!ref.empty(), "External reference link cannot be empty");
            FC_ASSERT(fc::is_utf8(ref), "External reference link is not valid UTF8 string");
        }
    }
};

struct start_research_token_sale_data_type : base_proposal_data_type
{
    research_id_type research_id;
    fc::time_point_sec start_time;
    fc::time_point_sec end_time;
    share_type amount_for_sale;
    asset soft_cap;
    asset hard_cap;

    void validate() const {
        FC_ASSERT(amount_for_sale > 0, "Research tokens for sale amount should be > 0");
        FC_ASSERT(soft_cap.amount > 0, "Soft cap should be > 0");
        FC_ASSERT(hard_cap.amount > 0, "Hard cap should be > 0");
        FC_ASSERT(soft_cap.symbol == hard_cap.symbol, "Assets doesn't match.");
        FC_ASSERT(hard_cap > soft_cap, "Hard cap should be greater than soft cap");
        FC_ASSERT(end_time > start_time, "End time should be greater than start time");
    }
};

struct change_research_review_reward_percent_data_type : base_proposal_data_type
{
    research_id_type research_id;
    uint16_t review_share_in_percent;

    void validate() const
    {
        FC_ASSERT(review_share_in_percent >= 0 && review_share_in_percent <= 50 * DEIP_1_PERCENT,
                  "Percent for review should be in 0 to 50 range");
    }
};

struct offer_research_tokens_data_type : base_proposal_data_type
{
    account_name_type sender;
    account_name_type receiver;
    research_id_type research_id;

    uint32_t amount;
    asset price;

    void validate() const {
        FC_ASSERT(is_valid_account_name(sender), "Account name ${n} is invalid", ("n", sender));
        FC_ASSERT(is_valid_account_name(receiver), "Account name ${n} is invalid", ("n", receiver));
        FC_ASSERT(amount > 0, "Amount must be > 0");
        FC_ASSERT(price.amount > 0, "Price must be > 0");
    }
};

struct change_research_group_metadata_type : base_proposal_data_type
{
    string research_group_name;
    string research_group_description;

    void validate() const {
        FC_ASSERT(!research_group_name.empty(), "Name cannot be empty");
        FC_ASSERT(fc::is_utf8(research_group_name), "Name is not valid UTF8 string");
        FC_ASSERT(!research_group_description.empty(), "Description cannot be empty");
        FC_ASSERT(fc::is_utf8(research_group_description), "Description is not valid UTF8 string");
    }
};

struct change_research_metadata_type : base_proposal_data_type
{
    research_id_type research_id;
    string research_title;
    string research_abstract;

    bool is_private;

    void validate() const {
        FC_ASSERT(!research_title.empty(), "Title cannot be empty");
        FC_ASSERT(fc::is_utf8(research_title), "Title is not valid UTF8 string");
        FC_ASSERT(!research_abstract.empty(), "Abstract cannot be empty");
        FC_ASSERT(fc::is_utf8(research_abstract), "Abstract is not valid UTF8 string");
    }
};

}
}

FC_REFLECT(deip::chain::dropout_member_proposal_data_type, (name))

FC_REFLECT(deip::chain::invite_member_proposal_data_type, (name)(research_group_token_amount_in_percent)(cover_letter)(is_head))

FC_REFLECT(deip::chain::change_action_quorum_proposal_data_type, (action)(quorum))

FC_REFLECT(deip::chain::start_research_proposal_data_type, (title)(abstract)(permlink)(review_share_in_percent)(dropout_compensation_in_percent)(disciplines)(is_private))

FC_REFLECT(deip::chain::send_funds_data_type, (recipient)(funds))

FC_REFLECT(deip::chain::rebalance_info, (account_name)(new_amount_in_percent))

FC_REFLECT(deip::chain::rebalance_research_group_tokens_data_type, (accounts))

FC_REFLECT(deip::chain::create_research_content_data_type, (research_id)(type)(title)(content)(permlink)(authors)(references)(external_references))

FC_REFLECT(deip::chain::start_research_token_sale_data_type, (research_id)(start_time)(end_time)(amount_for_sale)(soft_cap)(hard_cap))

FC_REFLECT(deip::chain::change_research_review_reward_percent_data_type, (research_id)(review_share_in_percent))

FC_REFLECT(deip::chain::offer_research_tokens_data_type, (sender)(receiver)(research_id)(amount)(price))

FC_REFLECT(deip::chain::change_research_group_metadata_type, (research_group_name)(research_group_description))

FC_REFLECT(deip::chain::change_research_metadata_type, (research_id)(research_title)(research_abstract)(is_private))
