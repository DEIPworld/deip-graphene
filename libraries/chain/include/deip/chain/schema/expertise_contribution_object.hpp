#pragma once

#include <fc/fixed_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>
#include <deip/protocol/eci_diff.hpp>

#include "deip_object_types.hpp"
#include "research_content_object.hpp"

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {
  
namespace bip = chainbase::bip;
using deip::protocol::asset;
using deip::protocol::eci_diff;

enum class expertise_contribution_type : uint16_t
{
    unknown = 0,
    publication = 1,
    review = 2,
    review_support = 3,

    FIRST = publication,
    LAST = review_support
};

typedef allocator<eci_diff> eci_diff_allocator_type;
typedef bip::vector<eci_diff, eci_diff_allocator_type> eci_diff_type_vector;

class expertise_contribution_object : public object<expertise_contribution_object_type, expertise_contribution_object>
{
    expertise_contribution_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    expertise_contribution_object(Constructor&& c, allocator<Allocator> a)
      : eci_current_block_diffs(a)
    {
        c(*this);
    }

    expertise_contribution_id_type id;
    research_id_type research_id;
    research_content_id_type research_content_id;
    discipline_id_type discipline_id;
    share_type eci;
    bool has_eci_current_block_diffs;
    share_type eci_current_block_delta;
    eci_diff_type_vector eci_current_block_diffs;
    flat_map<uint16_t, uint16_t> assessment_criterias;
};

struct by_discipline_id;
struct by_research_id;
struct by_research_content_id;
struct by_research_and_discipline;
struct by_research_content_and_discipline;
struct by_eci_current_block_delta;
struct by_has_eci_current_block_diffs;

typedef multi_index_container<expertise_contribution_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
        member<
          expertise_contribution_object,
          expertise_contribution_id_type,
          &expertise_contribution_object::id
        >
    >,
    ordered_non_unique<
      tag<by_research_id>,
        member<
          expertise_contribution_object,
          research_id_type,
          &expertise_contribution_object::research_id
        >
    >,
    ordered_non_unique<
      tag<by_discipline_id>,
        member<
          expertise_contribution_object,
          discipline_id_type,
          &expertise_contribution_object::discipline_id
        >
    >,
    ordered_non_unique<
      tag<by_eci_current_block_delta>,
        member<
          expertise_contribution_object,
          share_type,
          &expertise_contribution_object::eci_current_block_delta
        >
    >,
    ordered_non_unique<
      tag<by_has_eci_current_block_diffs>,
        member<
          expertise_contribution_object,
          bool,
          &expertise_contribution_object::has_eci_current_block_diffs
        >
    >,
    ordered_non_unique<
      tag<by_research_and_discipline>,
        composite_key<expertise_contribution_object,
          member<
            expertise_contribution_object,
            research_id_type,
            &expertise_contribution_object::research_id>,
          member<
            expertise_contribution_object,
            discipline_id_type,
            &expertise_contribution_object::discipline_id
          >
        >
    >,
    ordered_unique<
      tag<by_research_content_and_discipline>,
        composite_key<expertise_contribution_object,
          member<
            expertise_contribution_object,
            research_content_id_type,
            &expertise_contribution_object::research_content_id>,
          member<
            expertise_contribution_object,
            discipline_id_type,
            &expertise_contribution_object::discipline_id
          >
        >
    >,
    ordered_non_unique<
      tag<by_research_content_id>,
        member<
          expertise_contribution_object,
          research_content_id_type,
          &expertise_contribution_object::research_content_id
        >
      >
    >,
    allocator<expertise_contribution_object>>
    expertise_contribution_index;
}
}

FC_REFLECT_ENUM(deip::chain::expertise_contribution_type,
  (unknown)
  (publication)
  (review)
  (review_support)
)

FC_REFLECT( deip::chain::expertise_contribution_object,
  (id)
  (research_id)
  (research_content_id)
  (discipline_id)
  (eci)
  (has_eci_current_block_diffs)
  (eci_current_block_delta)
  (eci_current_block_diffs)
  (assessment_criterias)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::expertise_contribution_object, deip::chain::expertise_contribution_index)
