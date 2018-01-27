#pragma once

#include <deip/chain/deip_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <fc/utf8.hpp>

using namespace deip::protocol;

namespace deip {
namespace chain {

class proposal_object : public object<proposal_object_type, proposal_object>
{
    typedef deip::protocol::proposal_action_type action_t;
    typedef deip::protocol::account_name_type account_t;

public:
    template <typename Constructor, typename Allocator> proposal_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

public:
    proposal_id_type id;

    research_group_id_type research_group_id;
    action_t action;
    fc::time_point_sec creation_time;
    fc::time_point_sec expiration_time;
    account_t creator;
    std::string data;
    u_int16_t quorum_percent;
    share_type current_votes_amount;

    flat_set<account_name_type> voted_accounts;
};

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
    uint32_t percent_for_review;

    void validate() const
    {
        FC_ASSERT(!name.empty(), "Research name cannot be empty");
        FC_ASSERT(!abstract.empty(), "Research abstract cannot be empty");
        FC_ASSERT(permlink.size() < DEIP_MAX_PERMLINK_LENGTH, "Research permlink is too long");
        FC_ASSERT(fc::is_utf8(permlink), "Research permlink should be valid UTF8 string");
        FC_ASSERT(percent_for_review >= 0 && percent_for_review <= 50, "Percent for review should be in 0 to 50 range");
    }
};

typedef multi_index_container<proposal_object,
                                                indexed_by<ordered_unique<tag<by_id>, 
                                                                member<proposal_object, 
                                                                        proposal_id_type, 
                                                                        &proposal_object::id>>,
                                                            ordered_unique<tag<by_expiration_time>,
                                                                member<proposal_object,
                                                                        fc::time_point_sec,
                                                                        &proposal_object::expiration_time>>>,
                                                allocator<proposal_object>>
    proposal_index;

} // namespace chain
} // namespace deip


FC_REFLECT(deip::chain::proposal_object, (id)(research_group_id)(action)(creation_time)(expiration_time)(creator)(data)(quorum_percent)(current_votes_amount))

FC_REFLECT(deip::chain::member_proposal_data_type, (research_group_id)(name))

FC_REFLECT(deip::chain::invite_member_proposal_data_type, (research_group_id)(name)(research_group_token_amount))

FC_REFLECT(deip::chain::change_quorum_proposal_data_type, (research_group_id)(quorum_percent))

FC_REFLECT(deip::chain::start_research_proposal_data_type, (name)(abstract)(permlink)(research_group_id)(percent_for_review))

CHAINBASE_SET_INDEX_TYPE(deip::chain::proposal_object, deip::chain::proposal_index)