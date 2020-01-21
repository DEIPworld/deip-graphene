#pragma once

#include <deip/research_content_eci_history/research_content_eci_operation_object.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace research_content_eci_history {

template <uint16_t RCEciHistoryType> struct rc_eci_history_object : public object<RCEciHistoryType, rc_eci_history_object<RCEciHistoryType>>
{
    template <typename Constructor, typename Allocator>
    rc_eci_history_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    typedef typename object<RCEciHistoryType, rc_eci_history_object<RCEciHistoryType>>::id_type id_type;

    id_type id;

    research_content_id_type research_content_id;
    discipline_id_type discipline_id;

    share_type new_eci_amount;
    share_type delta;

    uint16_t action;
    int64_t action_object_id;

    uint32_t timestamp;

    research_content_eci_operation_object::id_type op;
};

struct by_content_and_discipline;
struct by_content_id;

template <typename research_content_eci_history_object_t>
using rc_eci_history_index = chainbase::shared_multi_index_container<
    research_content_eci_history_object_t,
    indexed_by<ordered_unique<tag<by_id>,
                              member<research_content_eci_history_object_t,
                                     typename research_content_eci_history_object_t::id_type,
                                     &research_content_eci_history_object_t::id>>,
               ordered_non_unique<tag<by_content_and_discipline>,
                                  composite_key<research_content_eci_history_object_t,
                                                member<research_content_eci_history_object_t,
                                                       research_content_id_type,
                                                       &research_content_eci_history_object_t::research_content_id>,
                                                member<research_content_eci_history_object_t,
                                                       discipline_id_type,
                                                       &research_content_eci_history_object_t::discipline_id>>>,
               ordered_non_unique<tag<by_content_id>,
                                  member<research_content_eci_history_object_t,
                                         research_content_id_type,
                                         &research_content_eci_history_object_t::research_content_id>>>>;

using research_content_eci_operations_history_object = rc_eci_history_object<all_research_content_eci_operations_history>;
using research_content_eci_operations_history_index = rc_eci_history_index<research_content_eci_operations_history_object>;

//
} // namespace research_content_eci_history
} // namespace deip

FC_REFLECT(deip::research_content_eci_history::research_content_eci_operations_history_object, (id)(research_content_id)(discipline_id)(new_eci_amount)(delta)(action)(action_object_id)(timestamp)(op))

CHAINBASE_SET_INDEX_TYPE(deip::research_content_eci_history::research_content_eci_operations_history_object,
                         deip::research_content_eci_history::research_content_eci_operations_history_index)