#pragma once

#include <deip/content_references_history/cr_operation_object.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace cr_history {

template <uint16_t CRHistoryType> struct cr_history_object : public object<CRHistoryType, cr_history_object<CRHistoryType>>
{
    template <typename Constructor, typename Allocator>
    cr_history_object(Constructor&& c, allocator<Allocator> a) : content(a), content_reference(a)
    {
        c(*this);
    }

    typedef typename object<CRHistoryType, cr_history_object<CRHistoryType>>::id_type id_type;

    id_type id;

    research_content_id_type research_content_id;
    research_id_type research_id;
    fc::shared_string content;

    research_content_id_type research_content_reference_id;
    research_id_type research_reference_id;
    fc::shared_string content_reference;

    cr_operation_object::id_type op;
};

struct by_research_content;
struct by_research_content_reference;

template <typename cr_history_object_t>
using cr_history_index
    = chainbase::shared_multi_index_container<cr_history_object_t,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<cr_history_object_t,
                                                                    typename cr_history_object_t::id_type,
                                                                    &cr_history_object_t::id>>,
                                              ordered_non_unique<tag<by_research_content>,
                                                             member<cr_history_object_t,
                                                                     research_content_id_type,
                                                                    &cr_history_object_t::research_content_id>>,
                                              ordered_non_unique<tag<by_research_content_reference>,
                                                             member<cr_history_object_t,
                                                                     research_content_id_type,
                                                                    &cr_history_object_t::research_content_reference_id>>>>;

using cr_operations_history_object = cr_history_object<all_cr_operations_history>;
using cr_operations_history_index = cr_history_index<cr_operations_history_object>;

//
} // namespace cr_history
} // namespace deip

FC_REFLECT(deip::cr_history::cr_operations_history_object, (id)(research_content_id)(research_id)(content)(research_content_reference_id)(research_reference_id)(content_reference)(op))

CHAINBASE_SET_INDEX_TYPE(deip::cr_history::cr_operations_history_object,
                         deip::cr_history::cr_operations_history_index)