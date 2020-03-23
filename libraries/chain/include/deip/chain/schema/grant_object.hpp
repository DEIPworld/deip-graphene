#pragma once

#include "deip_object_types.hpp"

namespace deip {
namespace chain {

using deip::protocol::asset;

class grant_object : public object<grant_object_type, grant_object>
{
    grant_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    grant_object(Constructor&& c, allocator<Allocator> a) 
      : target_disciplines(a)
    {
        c(*this);
    }

    grant_id_type id;

    account_name_type grantor;
    asset amount = asset(0, DEIP_SYMBOL);
    discipline_id_type_set target_disciplines;
    research_group_id_type review_committee_id;
    fc::time_point_sec created_at;

    // per block grant
    uint16_t min_number_of_positive_reviews;
    uint16_t min_number_of_applications;
    uint16_t max_number_of_research_to_grant;
    fc::time_point_sec start_date;
    fc::time_point_sec end_date;

    // foa
};

struct by_owner;
struct by_start_date;
struct by_end_date;

typedef multi_index_container<grant_object,
  indexed_by<
    ordered_unique<tag<by_id>,
      member<grant_object,
            grant_id_type,
            &grant_object::id>>,
    ordered_non_unique<tag<by_owner>,
      member<grant_object,
            account_name_type,
            &grant_object::grantor>>,
    ordered_non_unique<tag<by_start_date>,
      member<grant_object,
            fc::time_point_sec,
            &grant_object::start_date>>,
    ordered_non_unique<tag<by_end_date>,
      member<grant_object,
              fc::time_point_sec,
              &grant_object::end_date>>>,
              allocator<grant_object>>

    grant_index;
}
}

FC_REFLECT( deip::chain::grant_object,
  (id)
  (grantor)
  (amount)
  (target_disciplines)
  (review_committee_id)
  (created_at)
  (min_number_of_positive_reviews)
  (min_number_of_applications)
  (max_number_of_research_to_grant)
  (start_date)
  (end_date)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::grant_object, deip::chain::grant_index )

