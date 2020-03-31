#pragma once

#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace chain {

using deip::protocol::asset;

class award_recipient_object : public object<award_recipient_object_type, award_recipient_object>
{
    award_recipient_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    award_recipient_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    award_recipient_id_type id;
    award_id_type award_id;
    funding_opportunity_id_type funding_opportunity_id; 
    research_id_type research_id;
    research_group_id_type research_group_id;
    account_name_type awardee;

    asset total_amount;
    asset total_expenses;

    research_group_id_type university_id;
    share_type university_overhead;
};

struct by_award_and_research;
struct by_award;
struct by_funding_opportunity;
struct by_awardee;

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
      ordered_unique<
        tag<by_award_and_research>,
          composite_key<award_recipient_object,
            member<
              award_recipient_object,
              award_id_type,
              &award_recipient_object::award_id
            >,
            member<
              award_recipient_object,
              research_id_type,
              &award_recipient_object::research_id
            >
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
        tag<by_award>,
          member<
            award_recipient_object,
            award_id_type,
            &award_recipient_object::award_id
          >
      >,
      ordered_non_unique<
        tag<by_funding_opportunity>,
          member<
            award_recipient_object,
            funding_opportunity_id_type,
            &award_recipient_object::funding_opportunity_id
          >
      >
      >,
      allocator<award_recipient_object>>
      award_recipient_index;
}
}

FC_REFLECT(deip::chain::award_recipient_object,
  (id)
  (award_id)
  (funding_opportunity_id)
  (research_id)
  (research_group_id)
  (awardee)
  (total_amount)
  (total_expenses)
  (university_id)
  (university_overhead)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::award_recipient_object, deip::chain::award_recipient_index)
