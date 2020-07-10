#pragma once

#include <boost/multi_index/composite_key.hpp>

using namespace deip::chain;
using namespace std;

namespace deip {
namespace eci_history {

using chainbase::allocator;

class discipline_eci_history_object : public object<discipline_eci_history_object_type, discipline_eci_history_object>
{
public:
    template <typename Constructor, typename Allocator>
    discipline_eci_history_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    discipline_eci_history_object()
    {
    }

    discipline_eci_history_id_type id;
    discipline_id_type discipline_id;

    share_type eci;
    percent percentage;

    share_type total_eci;

    flat_map<uint16_t, assessment_criteria_value> assessment_criterias;

    fc::time_point_sec timestamp;
};

struct by_discipline;


typedef chainbase::shared_multi_index_container<discipline_eci_history_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
         member<
          discipline_eci_history_object,
          discipline_eci_history_id_type,
          &discipline_eci_history_object::id
        >
    >,
    ordered_non_unique<
      tag<by_discipline>,
         member<
          discipline_eci_history_object,
          discipline_id_type,
          &discipline_eci_history_object::discipline_id
        >
    >
    // , allocator<discipline_eci_history_object>
    >>
    discipline_eci_history_index;

} // namespace eci_history
} // namespace deip

FC_REFLECT(deip::eci_history::discipline_eci_history_object,
  (id)
  (discipline_id)
  (eci)
  (percentage)
  (total_eci)
  (assessment_criterias)
  (timestamp)
)

CHAINBASE_SET_INDEX_TYPE(deip::eci_history::discipline_eci_history_object, deip::eci_history::discipline_eci_history_index)
