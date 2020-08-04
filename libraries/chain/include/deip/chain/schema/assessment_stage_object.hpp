#pragma once
#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>
#include <deip/protocol/protocol.hpp>
#include <fc/shared_string.hpp>

namespace deip {
namespace chain {

class assessment_stage_object : public object<assessment_stage_object_type, assessment_stage_object>
{
    assessment_stage_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    assessment_stage_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    assessment_stage_id_type id;
    external_id_type external_id;

    assessment_id_type assessment_id;

    time_point_sec start_time;
    time_point_sec end_time;

    uint16_t seq;
};

struct by_external_id;
struct by_assessment_id;

typedef multi_index_container<assessment_stage_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
        member<
          assessment_stage_object,
          assessment_stage_id_type,
          &assessment_stage_object::id
        >
    >,
    ordered_unique<
      tag<by_external_id>,
        member<
          assessment_stage_object,
          external_id_type,
          &assessment_stage_object::external_id
        >
    >,
    ordered_non_unique<
      tag<by_assessment_id>,
        member<
          assessment_stage_object,
          assessment_id_type,
          &assessment_stage_object::assessment_id
        >
    >>,
    allocator<assessment_stage_object>>
    assessment_stage_index;

}
}

FC_REFLECT( deip::chain::assessment_stage_object,
  (id)
  (external_id)
  (assessment_id)
  (start_time)
  (end_time)
  (seq)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::assessment_stage_object, deip::chain::assessment_stage_index)