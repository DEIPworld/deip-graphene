#pragma once

#include <deip/chain/deip_object_types.hpp>

namespace deip {
namespace chain {

class proposal_object : public object<proposal_object_type, proposal_object>
{
public:
    template <typename Constructor, typename Allocator> proposal_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

public:
    proposal_id_type id;
};

using proposal_index = multi_index_container<proposal_object,
                                                indexed_by<ordered_unique<tag<by_id>, 
                                                                member<proposal_object, 
                                                                        proposal_id_type, 
                                                                        &proposal_object::id>>>,
                                                allocator<proposal_object>>;

} // namespace chain
} // namespace deip


FC_REFLECT( deip::chain::proposal_object,
            (id)
           )

CHAINBASE_SET_INDEX_TYPE(deip::chain::proposal_object, deip::chain::proposal_index)