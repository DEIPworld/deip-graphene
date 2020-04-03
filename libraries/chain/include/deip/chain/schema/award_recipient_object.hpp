#pragma once

#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace chain {

using deip::protocol::asset;

enum class award_recipient_status : uint16_t
{
    unknown = 0,
    unconfirmed = 1,
    confirmed = 2,
    canceled = 3,

    FIRST = unconfirmed,
    LAST = canceled
};

class award_recipient_object : public object<award_recipient_object_type, award_recipient_object>
{
    award_recipient_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    award_recipient_object(Constructor&& c, allocator<Allocator> a)
      : funding_opportunity_number(a)
      , award_number(a)
      , subaward_number(a)
    {
        c(*this);
    }

    award_recipient_id_type id;

    fc::shared_string funding_opportunity_number;
    fc::shared_string award_number;
    fc::shared_string subaward_number;

    research_id_type research_id;
    account_name_type awardee;
    account_name_type source; // for subawardees
    asset total_amount;
    asset total_expenses;

    uint16_t status;
};

struct by_award_number;
struct by_awardee;
struct by_funding_opportunity_number;
struct by_award_and_subaward_number;

typedef multi_index_container<award_recipient_object,
    indexed_by<
      ordered_unique<
        tag<by_id>,
          member<
            award_recipient_object,
            award_recipient_id_type,
            &award_recipient_object::id
          >
      >,
      ordered_non_unique<
        tag<by_awardee>,
          member<
            award_recipient_object,
            account_name_type,
            &award_recipient_object::awardee
          >
      >,
      ordered_non_unique<
        tag<by_funding_opportunity_number>,
          member<
            award_recipient_object,
            fc::shared_string,
            &award_recipient_object::funding_opportunity_number
          >
      >,
      ordered_unique<
        tag<by_award_and_subaward_number>,
          composite_key<award_recipient_object,
            member<
              award_recipient_object,
              fc::shared_string,
              &award_recipient_object::award_number
            >,
            member<
              award_recipient_object,
              fc::shared_string,
              &award_recipient_object::subaward_number
            >
          >,
          composite_key_compare<
            fc::strcmp_less,
            fc::strcmp_less
          >
      >,
      ordered_non_unique<
        tag<by_award_number>,
          member<
            award_recipient_object,
            fc::shared_string,
            &award_recipient_object::award_number
          >
      >
      >,
      allocator<award_recipient_object>>
      award_recipient_index;
}
}

FC_REFLECT(deip::chain::award_recipient_object,
  (id)
  (funding_opportunity_number)
  (award_number)
  (subaward_number)
  (research_id)
  (awardee)
  (source)
  (total_amount)
  (total_expenses)
  (status)
)


FC_REFLECT_ENUM(deip::chain::award_recipient_status,
  (unknown)
  (confirmed)
  (unconfirmed)
  (canceled)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::award_recipient_object, deip::chain::award_recipient_index)
