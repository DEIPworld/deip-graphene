#pragma once

#include "deip_object_types.hpp"

namespace deip {
namespace chain {

class expertise_stats_object : public object<expertise_stats_object_type, expertise_stats_object>
{
public:
    template <typename Constructor, typename Allocator>
    expertise_stats_object(Constructor&& c, allocator<Allocator> a) : used_expertise_last_week(a)
    {
        c(*this);
    }

    expertise_stats_id_type id;

    share_type_deque used_expertise_last_week;
    share_type used_expertise_per_block = 0;
    share_type total_used_expertise = 0;
};

typedef multi_index_container<expertise_stats_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<expertise_stats_object,
                        expertise_stats_id_type,
                        &expertise_stats_object::id>>>,
        allocator<expertise_stats_object>>
        expertise_stats_index;
}
} // deip::chain

FC_REFLECT(deip::chain::expertise_stats_object,
           (id)(used_expertise_last_week)(used_expertise_per_block)(total_used_expertise))
CHAINBASE_SET_INDEX_TYPE(deip::chain::expertise_stats_object, deip::chain::expertise_stats_index)

