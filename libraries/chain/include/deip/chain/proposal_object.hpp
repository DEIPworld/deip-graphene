#pragma once

#include <deip/chain/deip_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace chain {

class proposal_object : public object<proposal_object_type, proposal_object>
{
    typedef deip::protocol::proposal_action_type action_t;
    typedef deip::protocol::proposal_life_time_type lifetime_t;
    typedef deip::protocol::account_name_type account_t;

public:
    template <typename Constructor, typename Allocator> proposal_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

public:
    proposal_id_type id;

    action_t action;
    lifetime_t lifetime;
    account_t initiator;
    std::string json;
};

typedef multi_index_container<proposal_object,
                                                indexed_by<ordered_unique<tag<by_id>, 
                                                                member<proposal_object, 
                                                                        proposal_id_type, 
                                                                        &proposal_object::id>>>,
                                                allocator<proposal_object>>
    proposal_index;

} // namespace chain
} // namespace deip


FC_REFLECT(deip::chain::proposal_object, (id)(action)(lifetime)(initiator)(json))

CHAINBASE_SET_INDEX_TYPE(deip::chain::proposal_object, deip::chain::proposal_index)