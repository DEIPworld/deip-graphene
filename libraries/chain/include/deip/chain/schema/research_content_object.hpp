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

using chainbase::allocator;
using fc::shared_string;
using protocol::external_id_type;

enum research_content_type : uint16_t
{
    announcement = 1,
    final_result = 2,
    milestone_article = 3,
    milestone_book = 4,
    milestone_chapter = 5,
    milestone_code = 6,
    milestone_conference_paper = 7,
    milestone_cover_page = 8,
    milestone_data = 9,
    milestone_experiment_findings = 10,
    milestone_method = 11,
    milestone_negative_results = 12,
    milestone_patent = 13,
    milestone_poster = 14,
    milestone_preprint = 15,
    milestone_presentation = 16,
    milestone_raw_data = 17,
    milestone_research_proposal = 18,
    milestone_technical_report = 19,
    milestone_thesis = 20,

    start_milestone_type = milestone_article,
    last_milestone_type = milestone_thesis
};

enum research_content_activity_state : uint16_t
{
    active = 1,
    pending = 2,
    closed = 3
};

class research_content_object : public object<research_content_object_type, research_content_object>
{
    
    research_content_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    research_content_object(Constructor &&c, allocator<Allocator> a) 
      : description(a), 
        content(a)
    {
        c(*this);
    }

    research_content_id_type id;
    external_id_type external_id;

    research_id_type research_id;
    external_id_type research_external_id;

    shared_string description;
    shared_string content;

    research_content_type type;

    flat_set<account_name_type> authors;
    time_point_sec created_at;

    flat_set<external_id_type> references;

    flat_map<discipline_id_type, share_type> eci_per_discipline;

    // deprecated
    uint16_t activity_round;
    research_content_activity_state activity_state;
    time_point_sec activity_window_start;
    time_point_sec activity_window_end;
};

struct by_research_id;
struct by_research_id_and_content_type;
struct by_activity_state;
struct by_activity_window_start;
struct by_activity_window_end;
struct by_external_id;

typedef multi_index_container<research_content_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
        member<
          research_content_object,
          research_content_id_type,
          &research_content_object::id
        >
    >,

    ordered_unique<
      tag<by_external_id>,
        member<
          research_content_object,
          external_id_type,
          &research_content_object::external_id
        >
    >,

    ordered_non_unique<
      tag<by_research_id>,
        member<
          research_content_object,
          research_id_type,
          &research_content_object::research_id
        >
    >,

    ordered_non_unique<
      tag<by_research_id_and_content_type>,
        composite_key<research_content_object,
          member<
            research_content_object,
            research_id_type,
            &research_content_object::research_id
          >,
          member<
            research_content_object,
            research_content_type,
            &research_content_object::type
          >
        >
    >,

    ordered_non_unique<
      tag<by_activity_window_start>,
        member<
          research_content_object,
          time_point_sec,
          &research_content_object::activity_window_start
        >
    >,

    ordered_non_unique<
      tag<by_activity_window_end>,
        member<
          research_content_object,
          time_point_sec,
          &research_content_object::activity_window_end
        >
    >,

    ordered_non_unique<
      tag<by_activity_state>,
        member<
          research_content_object,
          research_content_activity_state,
          &research_content_object::activity_state
        >
    >
  >,
  allocator<research_content_object>>
  research_content_index;
}
}

FC_REFLECT_ENUM(deip::chain::research_content_type, 
  (announcement)
  (final_result)
  (milestone_article)
  (milestone_book)
  (milestone_chapter)
  (milestone_code)
  (milestone_conference_paper)
  (milestone_cover_page)
  (milestone_data)
  (milestone_experiment_findings)
  (milestone_method)
  (milestone_negative_results)
  (milestone_patent)
  (milestone_poster)
  (milestone_preprint)
  (milestone_presentation)
  (milestone_raw_data)
  (milestone_research_proposal)
  (milestone_technical_report)
  (milestone_thesis)
)

FC_REFLECT_ENUM(deip::chain::research_content_activity_state, (active)(pending)(closed))

FC_REFLECT(deip::chain::research_content_object, 
  (id)
  (external_id)
  (research_id)
  (research_external_id)
  (type)
  (description)
  (content)
  (authors)
  (created_at)
  (references)
  (activity_round)
  (activity_state)
  (activity_window_start)
  (activity_window_end)
  (eci_per_discipline)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_content_object, deip::chain::research_content_index)
