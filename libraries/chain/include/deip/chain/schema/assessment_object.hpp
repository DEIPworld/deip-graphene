#pragma once
#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>
#include <deip/protocol/protocol.hpp>
#include <fc/shared_string.hpp>

namespace deip {
namespace chain {

class assessment_object : public object<assessment_object_type, assessment_object>
{
    assessment_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    assessment_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    assessment_id_type id;

    external_id_type external_id;
    account_name_type creator;
};

struct by_external_id;
struct by_creator;

typedef multi_index_container<assessment_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
        member<
          assessment_object,
          assessment_id_type,
          &assessment_object::id
        >
    >,
    ordered_unique<
      tag<by_external_id>,
        member<
          assessment_object,
          external_id_type,
          &assessment_object::external_id
        >
    >,
    ordered_non_unique<
      tag<by_creator>,
        member<
          assessment_object,
          account_name_type,
          &assessment_object::creator
        >
    >>,
    allocator<assessment_object>>
    assessment_index;

}
}

FC_REFLECT( deip::chain::assessment_object,
  (id)
  (external_id)
  (creator)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::assessment_object, deip::chain::assessment_index )