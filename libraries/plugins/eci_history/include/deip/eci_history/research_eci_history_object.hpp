#pragma once

#include <boost/multi_index/composite_key.hpp>

using namespace deip::chain;
using namespace std;

namespace deip {
namespace eci_history {

using chainbase::allocator;

class research_eci_history_object : public object<research_eci_history_object_type, research_eci_history_object>
{
public:
    template <typename Constructor, typename Allocator>
    research_eci_history_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    research_eci_history_object()
    {
    }

    research_eci_history_id_type id;

    research_id_type research_id;
    discipline_id_type discipline_id;

    share_type eci;
    share_type delta;

    uint16_t alteration_source_type;
    int64_t alteration_source_id;

    fc::time_point_sec timestamp;
};

struct by_research_id;
struct by_research_and_discipline;

typedef chainbase::shared_multi_index_container<research_eci_history_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
         member<
          research_eci_history_object,
          research_eci_history_id_type,
          &research_eci_history_object::id
        >
    >,
    ordered_non_unique<
      tag<by_research_id>,
         member<
          research_eci_history_object,
          research_id_type,
          &research_eci_history_object::research_id
        >
    >,
    ordered_non_unique<
      tag<by_research_and_discipline>,
        composite_key<research_eci_history_object,
          member<
            research_eci_history_object,
            research_id_type,
            &research_eci_history_object::research_id
          >,
          member<
            research_eci_history_object,
            discipline_id_type,
            &research_eci_history_object::discipline_id
          >
        >
    >>
    >
    research_eci_history_index;

} // namespace eci_history
} // namespace deip

FC_REFLECT(deip::eci_history::research_eci_history_object,
  (id)
  (research_id)
  (discipline_id)
  (eci)
  (delta)
  (alteration_source_type)
  (alteration_source_id)
  (timestamp)
)

CHAINBASE_SET_INDEX_TYPE(deip::eci_history::research_eci_history_object, deip::eci_history::research_eci_history_index)