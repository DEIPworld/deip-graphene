#pragma once

#include <deip/chain/deip_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>

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

struct exclude_member_proposal_data_type
{
    research_group_id_type research_group_id;
    deip::protocol::account_name_type name;

    exclude_member_proposal_data_type()
    {
    }

    exclude_member_proposal_data_type(const research_group_id_type& research_group_id,
                                      const deip::protocol::account_name_type& name)
        : research_group_id(research_group_id),
          name(name)
    {
    }
};

struct change_quorum_proposal_data_type
{
    research_group_id_type research_group_id;
    u_int16_t quorum_percent;

    change_quorum_proposal_data_type(){}

    change_quorum_proposal_data_type(const research_group_id_type& research_group_id,
                                     const u_int16_t quorum_percent) : research_group_id(research_group_id),
                                                                       quorum_percent(quorum_percent){}
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

FC_REFLECT(deip::chain::exclude_member_proposal_data_type, (research_group_id)(name))

FC_REFLECT(deip::chain::change_quorum_proposal_data_type, (research_group_id)(quorum_percent))

CHAINBASE_SET_INDEX_TYPE(deip::chain::proposal_object, deip::chain::proposal_index)