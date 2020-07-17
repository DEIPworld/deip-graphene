#pragma once

#include <boost/multi_index/composite_key.hpp>

using namespace deip::chain;
using namespace std;

namespace deip {
namespace eci_history {

using chainbase::allocator;

class research_content_eci_history_object : public object<research_content_eci_history_object_type, research_content_eci_history_object>
{
public:
    template <typename Constructor, typename Allocator>
    research_content_eci_history_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    research_content_eci_history_object()
    {
    }

    research_content_eci_history_id_type id;

    research_content_id_type research_content_id;
    discipline_id_type discipline_id;
    external_id_type discipline_external_id;

    share_type eci;
    share_type delta;

    uint16_t contribution_type;
    int64_t contribution_id;
    flat_map<uint16_t, assessment_criteria_value> assessment_criterias;

    fc::time_point_sec timestamp;
};

struct by_research_content_id;
struct by_research_content_and_discipline;
struct by_research_content_and_cursor;

typedef chainbase::shared_multi_index_container<research_content_eci_history_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
         member<
          research_content_eci_history_object,
          research_content_eci_history_id_type,
          &research_content_eci_history_object::id
        >
    >,
    ordered_non_unique<
      tag<by_research_content_id>,
         member<
          research_content_eci_history_object,
          research_content_id_type,
          &research_content_eci_history_object::research_content_id
        >
    >,
    ordered_non_unique<
      tag<by_research_content_and_discipline>,
        composite_key<research_content_eci_history_object,
          member<
            research_content_eci_history_object,
            research_content_id_type,
            &research_content_eci_history_object::research_content_id
          >,
          member<
            research_content_eci_history_object,
            discipline_id_type,
            &research_content_eci_history_object::discipline_id
          >
        >
    >,
    ordered_non_unique<
      tag<by_research_content_and_cursor>,
        composite_key<research_content_eci_history_object,
          member<
            research_content_eci_history_object,
            research_content_id_type,
            &research_content_eci_history_object::research_content_id
          >,
          member<
            research_content_eci_history_object,
            research_content_eci_history_id_type,
            &research_content_eci_history_object::id
          >
        >
    >>
    >
    research_content_eci_history_index;

} // namespace eci_history
} // namespace deip

FC_REFLECT(deip::eci_history::research_content_eci_history_object, 
  (id)
  (research_content_id)
  (discipline_id)
  (discipline_external_id)
  (eci)
  (delta)
  (contribution_type)
  (contribution_id)
  (assessment_criterias)
  (timestamp)
)

CHAINBASE_SET_INDEX_TYPE(deip::eci_history::research_content_eci_history_object, deip::eci_history::research_content_eci_history_index)





