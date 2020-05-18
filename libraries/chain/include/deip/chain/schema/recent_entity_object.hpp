#pragma once

#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

using namespace deip::protocol;

namespace deip {
namespace chain {

class recent_entity_object : public object<recent_entity_object_type, recent_entity_object>
{

public:
    template <typename Constructor, typename Allocator> recent_entity_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

public:
    recent_entity_id_type id;
    external_id_type external_id;
    uint16_t ref_block_num;
    uint32_t ref_block_prefix;
    time_point_sec created_at;
};

struct by_external_id;
struct by_block_number;

typedef multi_index_container<recent_entity_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
        member<
          recent_entity_object,
          recent_entity_id_type,
          &recent_entity_object::id
        >
    >,

    ordered_unique<
      tag<by_external_id>,
        member<
          recent_entity_object,
          external_id_type,
          &recent_entity_object::external_id
        >
    >,
    
    ordered_non_unique<
      tag<by_block_number>,
        member<
          recent_entity_object,
          uint16_t,
          &recent_entity_object::ref_block_num
        >
    >
  >,
  allocator<recent_entity_object>>
  recent_entity_index;

} // namespace chain
} // namespace deip

FC_REFLECT(deip::chain::recent_entity_object,
  (id)
  (external_id)
  (ref_block_num)
  (ref_block_prefix)
  (created_at)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::recent_entity_object, deip::chain::recent_entity_index)
