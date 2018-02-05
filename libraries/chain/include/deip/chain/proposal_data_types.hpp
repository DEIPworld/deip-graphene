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

struct member_proposal_data_type : base_proposal_data_type
{
    research_group_id_type research_group_id;
    deip::protocol::account_name_type name;

    void validate() const
    {
        FC_ASSERT(is_valid_account_name(name), "Account name ${n} is invalid", ("n", name));
    }
};

struct invite_member_proposal_data_type : base_proposal_data_type
{
    research_group_id_type research_group_id;
    deip::protocol::account_name_type name;
    uint32_t research_group_token_amount;

    void validate() const
    {
        FC_ASSERT(is_valid_account_name(name), "Account name ${n} is invalid", ("n", name));
        FC_ASSERT(research_group_token_amount > 0, "Research group tokens amount should be > 0");
    }
};

struct change_quorum_proposal_data_type : base_proposal_data_type
{
    research_group_id_type research_group_id;
    u_int16_t quorum_percent;

    void validate() const
    {
        FC_ASSERT(quorum_percent >= 5 && quorum_percent <= 100, "Quorum percent should be in 5 to 100 range");
    }
};

struct start_research_proposal_data_type : base_proposal_data_type
{
    string name;
    string abstract;
    string permlink;
    research_group_id_type research_group_id;
    double review_share_in_percent;

    void validate() const
    {
        FC_ASSERT(!name.empty(), "Research name cannot be empty");
        FC_ASSERT(!abstract.empty(), "Research abstract cannot be empty");
        FC_ASSERT(permlink.size() < DEIP_MAX_PERMLINK_LENGTH, "Research permlink is too long");
        FC_ASSERT(fc::is_utf8(permlink), "Research permlink should be valid UTF8 string");
        FC_ASSERT(review_share_in_percent >= 0 && review_share_in_percent <= 50, "Percent for review should be in 0 to 50 range");
    }
};

struct create_research_content_data_type : base_proposal_data_type
{
    research_id_type research_id;
    research_content_type type;
    string content;
    flat_set<account_name_type> authors;

    void validate() const
    {
        FC_ASSERT(!content.empty(), "Content cannot be empty");
        FC_ASSERT(!authors.empty(), "Content should have author(s)");
        for (auto& author : authors)
        {
            chain::validate_account_name(author);
        }
    }
};

}
}

FC_REFLECT(deip::chain::member_proposal_data_type, (research_group_id)(name))

FC_REFLECT(deip::chain::invite_member_proposal_data_type, (research_group_id)(name)(research_group_token_amount))

FC_REFLECT(deip::chain::change_quorum_proposal_data_type, (research_group_id)(quorum_percent))

FC_REFLECT(deip::chain::start_research_proposal_data_type, (name)(abstract)(permlink)(research_group_id)(review_share_in_percent))

FC_REFLECT(deip::chain::create_research_content_data_type, (research_id)(type)(content)(authors))

