#pragma once

#include "deip_object_types.hpp"

namespace deip {
namespace chain {

class chain_property_object;

class chain_property_object : public object<chain_property_object_type, chain_property_object>
{
public:
    template <typename Constructor, typename Allocator> chain_property_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    id_type id;
    chain_id_type chain_id;
};

typedef multi_index_container<chain_property_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<chain_property_object,
                                                               chain_property_object::id_type,
                                                               &chain_property_object::id>>>,
                              allocator<chain_property_object>>
    chain_property_index;

} // namespace chain
} // namespace deip

FC_REFLECT(deip::chain::chain_property_object, (id)(chain_id))
CHAINBASE_SET_INDEX_TYPE(deip::chain::chain_property_object, deip::chain::chain_property_index)
