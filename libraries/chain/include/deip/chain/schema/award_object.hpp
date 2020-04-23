#pragma once

#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace chain {

enum class award_status : uint16_t
{
    pending = 1,
    approved = 2,
    rejected = 3
};

using protocol::external_id_type;
using deip::protocol::asset;

class award_object : public object<award_object_type, award_object>
{
    award_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    award_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    award_id_type id;

    external_id_type funding_opportunity_number;
    external_id_type award_number;
    account_name_type awardee;

    research_group_id_type university_id;
    percent_type university_overhead;

    account_name_type creator;
    uint16_t status = static_cast<uint16_t>(award_status::pending);
    asset amount;

    // award_map_type_map award_map;
};

struct by_creator;
struct by_awardee;
struct by_funding_opportunity_number;
struct by_award_number;

typedef multi_index_container<award_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
        member<
          award_object,
          award_id_type,
          &award_object::id
        >
    >,
    ordered_non_unique<
      tag<by_awardee>,
        member<
          award_object,
          account_name_type,
          &award_object::awardee
        >
    >,
    ordered_non_unique<
      tag<by_creator>,
        member<
          award_object,
          account_name_type,
          &award_object::creator
        >
    >,
    ordered_unique<
      tag<by_award_number>,
        member<
          award_object,
          external_id_type,
          &award_object::award_number
        >
    >,
    ordered_non_unique<
      tag<by_funding_opportunity_number>,
        member<
          award_object,
          external_id_type,
          &award_object::funding_opportunity_number
        >
    >
    >,
    allocator<award_object>>
    award_index;

}
}

FC_REFLECT_ENUM(deip::chain::award_status, 
  (pending)
  (approved)
  (rejected)
)

FC_REFLECT(deip::chain::award_object,
  (id)
  (funding_opportunity_number)
  (award_number)
  (awardee)
  (university_id)
  (university_overhead)
  (creator)
  (status)
  (amount)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::award_object, deip::chain::award_index )
