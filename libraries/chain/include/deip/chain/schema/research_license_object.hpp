#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include "deip_object_types.hpp"

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

#include <vector>

namespace deip {
namespace chain {

using fc::shared_string;
using protocol::external_id_type;
using protocol::asset;

enum class research_license_status : uint16_t
{
    active = 1,
    expired = 2
};

class research_license_object : public object<research_license_object_type, research_license_object>
{
    research_license_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    research_license_object(Constructor&& c, allocator<Allocator> a)
        : terms(a)
    {
        c(*this);
    }

    research_license_id_type id;
    external_id_type external_id;

    account_name_type licenser;
    external_id_type research_external_id;

    account_name_type licensee;

    shared_string terms;
    uint16_t status;

    time_point_sec expiration_time;
    time_point_sec created_at;

    optional<asset> fee;
};

struct by_external_id;
struct by_research;
struct by_licenser;
struct by_licensee;

struct by_licensee_and_research;
struct by_licensee_and_licenser;


typedef multi_index_container<research_license_object,
  indexed_by<

    ordered_unique<
      tag<by_id>,
        member<
          research_license_object,
          research_license_id_type,
          &research_license_object::id
        >
    >,

    ordered_unique<
      tag<by_external_id>,
        member<
          research_license_object,
          external_id_type,
          &research_license_object::external_id
        >
    >,

    ordered_non_unique<
      tag<by_licensee_and_research>,
        composite_key<research_license_object,
          member<
            research_license_object,
            account_name_type,
            &research_license_object::licensee
          >,
          member<
            research_license_object,
            external_id_type,
            &research_license_object::research_external_id
          >
        >
    >,

    ordered_non_unique<
      tag<by_licensee_and_licenser>,
        composite_key<research_license_object,
          member<
            research_license_object,
            account_name_type,
            &research_license_object::licensee
          >,
          member<
            research_license_object,
            account_name_type,
            &research_license_object::licenser
          >
        >
    >,

    ordered_non_unique<
      tag<by_research>,
        member<
          research_license_object,
          external_id_type,
          &research_license_object::research_external_id
        >
    >,

    ordered_non_unique<
      tag<by_licensee>,
        member<
          research_license_object,
          account_name_type,
          &research_license_object::licensee
        >
    >,

    ordered_non_unique<
      tag<by_licenser>,
        member<
          research_license_object,
          account_name_type,
          &research_license_object::licenser
        >
    >
  >,

  allocator<research_license_object>>
  research_license_index;
}
}


FC_REFLECT(deip::chain::research_license_object,
  (id)
  (external_id)
  (licenser)
  (research_external_id)
  (licensee)
  (terms)
  (status)
  (created_at)
  (expiration_time)
  (fee)
)

FC_REFLECT_ENUM(deip::chain::research_license_status, (active)(expired))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_license_object, deip::chain::research_license_index)