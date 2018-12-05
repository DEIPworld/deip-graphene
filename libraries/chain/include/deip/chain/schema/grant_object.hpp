#pragma once

#include "deip_object_types.hpp"

namespace deip {
namespace chain {

class grant_object : public object<grant_object_type, grant_object>
{
    grant_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    grant_object(Constructor&& c, allocator<Allocator> a) :
    {
        c(*this);
    }

    grant_id_type id;
    discipline_id_type target_discipline;

    asset amount = asset(0, DEIP_SYMBOL);

    int16_t min_number_of_positive_reviews;
    int16_t researches_to_grant;

    fc::time_point_sec created_at;
    fc::time_point_sec start_time;
    fc::time_point_sec end_time;
};

struct by_grant_id;
struct by_target_discipline;
struct by_start_time;
struct by_end_time;

typedef multi_index_container<grant_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<grant_object,
                                                                grant_id_type,
                                                               &grant_object::id>>,
                                         ordered_non_unique<tag<by_target_discipline>,
                                                        member<grant_object,
                                                                discipline_id_type,
                                                               &grant_object::target_discipline>>,
                                         ordered_non_unique<tag<by_start_time>,
                                                        member<grant_object,
                                                                fc::time_point_sec,
                                                               &grant_object::start_time>>,
                                         ordered_non_unique<tag<by_end_time>,
                                                        member<grant_object,
                                                                fc::time_point_sec,
                                                               &grant_object::end_time>>>,
                              allocator<grant_object>>
    grant_index;

}
}

FC_REFLECT( deip::chain::grant_object,
             (id)(target_discipline)(amount)(min_number_of_positive_reviews)(researches_to_grant)(created_at)(start_time)(end_time)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::grant_object, deip::chain::grant_index )

