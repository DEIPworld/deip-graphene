#pragma once

#include <deip/research_content_reference_history/research_content_reference_operation_object.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace research_content_references_history {

using protocol::external_id_type;

template <uint16_t CRHistoryType>
struct research_content_reference_history_object : public object<CRHistoryType, research_content_reference_history_object<CRHistoryType>>
{
    template <typename Constructor, typename Allocator>
    research_content_reference_history_object(Constructor&& c, allocator<Allocator> a) : content(a), content_reference(a)
    {
        c(*this);
    }

    typedef typename object<CRHistoryType, research_content_reference_history_object<CRHistoryType>>::id_type id_type;

    id_type id;

    research_content_id_type research_content_id;
    external_id_type research_content_external_id;
    research_id_type research_id;
    external_id_type research_external_id;
    fc::shared_string content;

    research_content_id_type research_content_reference_id;
    external_id_type research_content_reference_external_id;
    research_id_type research_reference_id;
    external_id_type research_reference_external_id;
    fc::shared_string content_reference;

    research_content_reference_operation_object::id_type op;
};

struct by_research_content;
struct by_research_content_external_id;

struct by_research_content_reference;
struct by_research_content_reference_external_id;

template <typename cr_history_object_t>
using research_content_reference_history_index
    = chainbase::shared_multi_index_container<cr_history_object_t,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<cr_history_object_t,
                                                                    typename cr_history_object_t::id_type,
                                                                    &cr_history_object_t::id>>,
                                              ordered_non_unique<tag<by_research_content>,
                                                             member<cr_history_object_t,
                                                                    research_content_id_type,
                                                                    &cr_history_object_t::research_content_id>>,

                                              ordered_non_unique<tag<by_research_content_external_id>,
                                                             member<cr_history_object_t,
                                                                    external_id_type,
                                                                    &cr_history_object_t::research_content_external_id>>,

                                              ordered_non_unique<tag<by_research_content_reference_external_id>,
                                                             member<cr_history_object_t,
                                                                    external_id_type,
                                                                    &cr_history_object_t::research_content_reference_external_id>>,

                                              ordered_non_unique<tag<by_research_content_reference>,
                                                             member<cr_history_object_t,
                                                                    research_content_id_type,
                                                                    &cr_history_object_t::research_content_reference_id>>>>;

using research_content_reference_operations_history_object = research_content_reference_history_object<all_research_content_reference_operations_history>;
using research_content_reference_operations_history_index = research_content_reference_history_index<research_content_reference_operations_history_object>;

//
} // namespace research_content_references_history
} // namespace deip

FC_REFLECT(deip::research_content_references_history::research_content_reference_operations_history_object, 
  (id)
  (research_content_id)
  (research_content_external_id)
  (research_id)
  (research_external_id)
  (content)
  (research_content_reference_id)
  (research_content_reference_external_id)
  (research_reference_id)
  (research_reference_external_id)
  (content_reference)
  (op)
)

CHAINBASE_SET_INDEX_TYPE(deip::research_content_references_history::research_content_reference_operations_history_object,
                         deip::research_content_references_history::research_content_reference_operations_history_index)