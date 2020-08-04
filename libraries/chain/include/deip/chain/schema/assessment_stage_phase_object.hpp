#pragma once
#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>
#include <deip/protocol/protocol.hpp>
#include <fc/shared_string.hpp>

namespace deip {
namespace chain {

class assessment_stage_phase_object : public object<assessment_stage_phase_object_type, assessment_stage_phase_object>
{
    assessment_stage_phase_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    assessment_stage_phase_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    assessment_stage_phase_id_type id;
    assessment_stage_id_type assessment_stage_id;
    assessment_id_type assessment_id;
};

struct by_assessment_stage_id;
struct by_assessment_id;


typedef multi_index_container<assessment_stage_phase_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
        member<
          assessment_stage_phase_object,
          assessment_stage_phase_id_type,
          &assessment_stage_phase_object::id
        >
    >,
    ordered_non_unique<
      tag<by_assessment_stage_id>,
        member<
          assessment_stage_phase_object,
          assessment_stage_id_type,
          &assessment_stage_phase_object::assessment_stage_id
        >
    >,
    ordered_non_unique<
      tag<by_assessment_id>,
        member<
          assessment_stage_phase_object,
          assessment_id_type,
          &assessment_stage_phase_object::assessment_id
        >
    >>,
    allocator<assessment_stage_phase_object>>
    assessment_stage_phase_index;

}
}

FC_REFLECT( deip::chain::assessment_stage_phase_object,
  (id)
  (assessment_stage_id)
  (assessment_id)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::assessment_stage_phase_object, deip::chain::assessment_stage_phase_index)