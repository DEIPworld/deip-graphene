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

    funding_opportunity_id_type funding_opportunity_id;
    account_name_type creator;
    award_status status = award_status::pending;
    asset amount;
};

struct by_creator;
struct by_funding_opportunity;

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
      tag<by_creator>,
        member<
          award_object,
          account_name_type,
          &award_object::creator
        >
    >,
    ordered_non_unique<
      tag<by_funding_opportunity>,
        member<
          award_object,
          funding_opportunity_id_type,
          &award_object::funding_opportunity_id
        >
    >
    >,
    allocator<award_object>>
    award_index;
    }
}

FC_REFLECT_ENUM(deip::chain::award_status, (pending)(approved)(rejected))

FC_REFLECT( deip::chain::award_object,
    (id)
    (funding_opportunity_id)
    (creator)
    (status)
    (amount)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::award_object, deip::chain::award_index )
