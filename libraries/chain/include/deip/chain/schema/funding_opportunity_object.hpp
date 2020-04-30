#pragma once

#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace chain {

using protocol::external_id_type;

enum class funding_opportunity_distribution_type : uint16_t
{
    unknown = 0,
    eci_evaluation = 1,
    officer_evaluation = 2,
    discipline_supply = 3
};

class funding_opportunity_object : public object<funding_opportunity_object_type, funding_opportunity_object>
{
    funding_opportunity_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    funding_opportunity_object(Constructor&& c, allocator<Allocator> a)
        : additional_info(a)
        , target_disciplines(a)
        , officers(a)
    {
        c(*this);
    }

    funding_opportunity_id_type id;

    research_group_id_type organization_id;
    research_group_id_type review_committee_id;
    research_group_id_type treasury_id;

    account_name_type grantor;
    external_id_type funding_opportunity_number;

    shared_string_type_map additional_info;

    discipline_id_type_set target_disciplines;

    asset amount = asset(0, DEIP_SYMBOL);
    asset award_ceiling = asset(0, DEIP_SYMBOL);
    asset award_floor = asset(0, DEIP_SYMBOL);
    asset current_supply = asset(0, DEIP_SYMBOL);

    uint16_t min_number_of_positive_reviews;
    uint16_t min_number_of_applications;
    uint16_t max_number_of_research_to_grant;

    uint16_t expected_number_of_awards;

    account_name_type_set officers;

    fc::time_point_sec open_date;
    fc::time_point_sec close_date;
    fc::time_point_sec posted_date;

    uint16_t distribution_type;
};

struct by_funding_opportunity_number;
struct by_grantor;
struct by_open_date;
struct by_close_date;
struct by_organization;
struct by_distribution_type_and_close_date;

typedef multi_index_container<funding_opportunity_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
        member<
          funding_opportunity_object,
          funding_opportunity_id_type,
          &funding_opportunity_object::id
        >
    >,
    ordered_unique<
      tag<by_funding_opportunity_number>,
        member<
          funding_opportunity_object,
          external_id_type,
          &funding_opportunity_object::funding_opportunity_number
        >
    >,
    ordered_non_unique<
      tag<by_grantor>,
        member<
          funding_opportunity_object,
          account_name_type,
          &funding_opportunity_object::grantor
        >
    >,
    ordered_non_unique<
      tag<by_organization>,
        member<
          funding_opportunity_object,
          research_group_id_type,
          &funding_opportunity_object::organization_id
        >
    >,
    ordered_non_unique<
      tag<by_open_date>,
        member<
          funding_opportunity_object,
          fc::time_point_sec,
          &funding_opportunity_object::open_date
        >
    >,
    ordered_non_unique<
      tag<by_distribution_type_and_close_date>,
        composite_key<funding_opportunity_object,
          member<
            funding_opportunity_object,
            uint16_t,
            &funding_opportunity_object::distribution_type>,
          member<
            funding_opportunity_object,
            fc::time_point_sec,
            &funding_opportunity_object::close_date>
        >
    >,
    ordered_non_unique<
      tag<by_close_date>,
        member<
          funding_opportunity_object,
          fc::time_point_sec,
          &funding_opportunity_object::close_date>
        >
    >,
    allocator<funding_opportunity_object>>
    funding_opportunity_index;

}
}

FC_REFLECT_ENUM(deip::chain::funding_opportunity_distribution_type,
  (unknown)
  (eci_evaluation)
  (officer_evaluation)
  (discipline_supply)
)

FC_REFLECT(deip::chain::funding_opportunity_object,
        (id)
        (organization_id)
        (review_committee_id)
        (treasury_id)
        (grantor)
        (funding_opportunity_number)
        (additional_info)
        (target_disciplines)
        (amount)
        (award_ceiling)
        (award_floor)
        (current_supply)
        (min_number_of_positive_reviews)
        (min_number_of_applications)
        (max_number_of_research_to_grant)
        (expected_number_of_awards)
        (officers)
        (open_date)
        (close_date)
        (posted_date)
        (distribution_type)
)


CHAINBASE_SET_INDEX_TYPE( deip::chain::funding_opportunity_object, deip::chain::funding_opportunity_index )

