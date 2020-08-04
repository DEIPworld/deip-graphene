#pragma once
#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>
#include <deip/protocol/protocol.hpp>
#include <fc/shared_string.hpp>

namespace deip {
namespace chain {

enum class phase_type : uint8_t
{
    unknown = 0,

    apply = 1,
    await_review = 2,
    review = 3,
    decision = 4,

    FIRST = apply,
    LAST = decision
};

enum class phase_rules : uint32_t
{
    no_rules = 0,

    create_application_rule = 1 << 0,
    update_application_rule = 1 << 1,
    delete_application_rule = 1 << 2,

    create_review_rule = 1 << 3,
    update_review_rule = 1 << 4,
    delete_review_rule = 1 << 5,
    create_curation_rule = 1 << 6,
    delete_curation_rule = 1 << 7,

    manual_decision_rule = 1 << 8,
    auto_decision_rule = 1 << 9
};

class assessment_stage_phase_object : public object<assessment_stage_phase_object_type, assessment_stage_phase_object>
{
    assessment_stage_phase_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    assessment_stage_phase_object(Constructor&& c, allocator<Allocator> a)
        : rules_impl(a)
    {
        c(*this);
    }

    assessment_stage_phase_id_type id;
    assessment_stage_id_type assessment_stage_id;
    assessment_id_type assessment_id;

    time_point_sec start_time;
    time_point_sec end_time;

    uint8_t type;

    typedef allocator<std::pair<const uint32_t, fc::shared_string>> rules_impl_map_allocator_type;
    typedef chainbase::bip::map<uint32_t, fc::shared_string, std::less<uint32_t>, rules_impl_map_allocator_type> rules_impl_map_type;

    uint32_t rules;
    rules_impl_map_type rules_impl;
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
  (start_time)
  (end_time)
  (type)
  (rules)
  (rules_impl)
)

FC_REFLECT_ENUM(deip::chain::phase_type,
  (unknown)
  (apply)
  (await_review)
  (review)
  (decision)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::assessment_stage_phase_object, deip::chain::assessment_stage_phase_index)