#pragma once

#include <deip/chain/deip_object_types.hpp>
#include <fc/utf8.hpp>

using namespace deip::protocol;

namespace deip {
namespace chain {

struct base_proposal_data_type
{
    virtual void validate() const = 0;
};

struct dropout_member_proposal_data_type : base_proposal_data_type
{
    research_group_id_type research_group_id;
    account_name_type name;

    void validate() const
    {
        FC_ASSERT(is_valid_account_name(name), "Account name ${n} is invalid", ("n", name));
    }
};

struct invite_member_proposal_data_type : base_proposal_data_type
{
    research_group_id_type research_group_id;
    deip::protocol::account_name_type name;
    share_type research_group_token_amount_in_percent;

    void validate() const
    {
        FC_ASSERT(is_valid_account_name(name), "Account name ${n} is invalid", ("n", name));
        FC_ASSERT(research_group_token_amount_in_percent > 0, "Research group tokens amount should be > 0");
    }
};

struct change_quorum_proposal_data_type : base_proposal_data_type
{
    research_group_id_type research_group_id;
    uint16_t quorum_percent;

    void validate() const
    {
        FC_ASSERT(quorum_percent >= 5 && quorum_percent <= 100, "Quorum percent should be in 5 to 100 range");
    }
};

struct start_research_proposal_data_type : base_proposal_data_type
{
    string title;
    string abstract;
    string permlink;
    research_group_id_type research_group_id;
    uint16_t review_share_in_percent;
    uint16_t dropout_compensation_in_percent;
    std::vector<int64_t> disciplines;


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
    research_group_id_type research_group_id;
    account_name_type recipient;
    share_type funds;

    void validate() const
    {
        FC_ASSERT(is_valid_account_name(recipient), "Account name ${n} is invalid", ("n", recipient));
        FC_ASSERT(funds >= 0, "Amount cant be negative");
    }
};
struct rebalance_info
{
    account_name_type account_name;
    share_type new_amount_in_percent;
};

struct rebalance_research_group_tokens_data_type : base_proposal_data_type
{
    research_group_id_type research_group_id;
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
    std::vector<account_name_type> authors;
    std::vector<research_content_id_type> references;
    std::vector<string> external_references;

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

        for (auto& link : external_references)
        {
            FC_ASSERT(!link.empty(), "External reference link cannot be empty");
            FC_ASSERT(fc::is_utf8(link), "External reference link is not valid UTF8 string");
        }
    }
};

struct start_research_token_sale_data_type : base_proposal_data_type
{
    research_id_type research_id;
    fc::time_point_sec start_time;
    fc::time_point_sec end_time;
    share_type amount_for_sale;
    share_type soft_cap;
    share_type hard_cap;

    void validate() const {
        FC_ASSERT(amount_for_sale > 0, "Research tokens for sale amount should be > 0");
        FC_ASSERT(soft_cap > 0, "Soft cap should be > 0");
        FC_ASSERT(hard_cap > 0, "Hard cap should be > 0");
        FC_ASSERT(hard_cap > soft_cap, "Hard cap should be greater than soft cap");
        FC_ASSERT(start_time >= fc::time_point::now(), "Start time cannot be at the past");
        FC_ASSERT(end_time > start_time, "End time should be greater than start time");
    }
};

struct change_research_review_share_percent_data_type : base_proposal_data_type
{
    research_id_type research_id;
    uint16_t review_share_in_percent;

    void validate() const
    {
        FC_ASSERT(review_share_in_percent >= 0 && review_share_in_percent <= 50 * DEIP_1_PERCENT,
                  "Percent for review should be in 0 to 50 range");
    }
};

}
}

FC_REFLECT(deip::chain::dropout_member_proposal_data_type, (research_group_id)(name))

FC_REFLECT(deip::chain::invite_member_proposal_data_type, (research_group_id)(name)(research_group_token_amount_in_percent))

FC_REFLECT(deip::chain::change_quorum_proposal_data_type, (research_group_id)(quorum_percent))

FC_REFLECT(deip::chain::start_research_proposal_data_type, (title)(abstract)(permlink)(research_group_id)(review_share_in_percent)(dropout_compensation_in_percent)(disciplines))

FC_REFLECT(deip::chain::send_funds_data_type, (research_group_id)(recipient)(funds))

FC_REFLECT(deip::chain::rebalance_info, (account_name)(new_amount_in_percent))

FC_REFLECT(deip::chain::rebalance_research_group_tokens_data_type, (research_group_id)(accounts))

FC_REFLECT(deip::chain::create_research_content_data_type, (research_id)(type)(title)(content)(permlink)(authors)(references)(external_references))

FC_REFLECT(deip::chain::start_research_token_sale_data_type, (research_id)(start_time)(end_time)(amount_for_sale)(soft_cap)(hard_cap))

FC_REFLECT(deip::chain::change_research_review_share_percent_data_type, (research_id)(review_share_in_percent))