#pragma once

#include <deip/chain/deip_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

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
    share_type quorum_percent;
    share_type current_votes_amount;

    flat_set<account_name_type> voted_accounts;
};

struct by_research_group_id;
struct by_expiration_time;
struct by_data;


typedef multi_index_container<proposal_object,
                                                indexed_by<ordered_unique<tag<by_id>,
                                                                member<proposal_object,
                                                                       proposal_id_type,
                                                                       &proposal_object::id>>,
                                                           hashed_unique<tag<by_data>,
                                                                member<proposal_object,
                                                                       std::string,
                                                                       &proposal_object::data>,
                                                                std::hash<std::string>>,
                                                           ordered_unique<tag<by_expiration_time>,
                                                                member<proposal_object,
                                                                        fc::time_point_sec,
                                                                        &proposal_object::expiration_time>>,
                                                           ordered_non_unique<tag<by_research_group_id>,
                                                                member<proposal_object,
                                                                        research_group_id_type,
                                                                        &proposal_object::research_group_id>>>,
                                                           allocator<proposal_object>>
    proposal_index;


} // namespace chain
} // namespace deip


FC_REFLECT(deip::chain::proposal_object, (id)(research_group_id)(action)(creation_time)(expiration_time)(creator)(data)(quorum_percent)(current_votes_amount))

CHAINBASE_SET_INDEX_TYPE(deip::chain::proposal_object, deip::chain::proposal_index)