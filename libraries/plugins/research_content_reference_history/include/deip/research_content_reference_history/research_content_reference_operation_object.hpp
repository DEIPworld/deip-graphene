#pragma once

#include <deip/chain/schema/deip_object_types.hpp>
#include <fc/shared_buffer.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/operations.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <boost/multi_index/composite_key.hpp>

#ifndef RESEARCH_CONTENT_REFERENCE_HISTORY_SPACE_ID
#define RESEARCH_CONTENT_REFERENCE_HISTORY_SPACE_ID 15
#endif


namespace deip {
namespace research_content_references_history {

using deip::protocol::transaction_id_type;
using deip::protocol::operation;
using namespace deip::chain;

enum research_content_reference_history_object_type
{
    research_content_reference_operations_history = (RESEARCH_CONTENT_REFERENCE_HISTORY_SPACE_ID << 8),
    all_research_content_reference_operations_history
};

class research_content_reference_operation_object : public object<research_content_reference_operations_history, research_content_reference_operation_object>
{
    research_content_reference_operation_object() = delete;

 public:
    template <typename Constructor, typename Allocator>
    research_content_reference_operation_object(Constructor&& c, allocator<Allocator> a)
        : serialized_op(a)
    {
        c(*this);
    }

    typedef typename object<research_content_reference_operations_history, research_content_reference_operation_object>::id_type id_type;

    id_type id;

    transaction_id_type trx_id;
    uint32_t block = 0;

    time_point_sec timestamp;
    fc::shared_buffer serialized_op;
};

struct by_transaction_id;
typedef chainbase::shared_multi_index_container<
    research_content_reference_operation_object,
    indexed_by<ordered_unique<tag<by_id>,
                              member<research_content_reference_operation_object,
                                     research_content_reference_operation_object::id_type,
                                     &research_content_reference_operation_object::id>>
#ifndef SKIP_BY_TX_ID
               ,
               ordered_unique<tag<by_transaction_id>,
                              composite_key<research_content_reference_operation_object,
                                            member<research_content_reference_operation_object,
                                                   transaction_id_type,
                                                   &research_content_reference_operation_object::trx_id>,
                                            member<research_content_reference_operation_object,
                                                   research_content_reference_operation_object::id_type,
                                                   &research_content_reference_operation_object::id>>>
#endif
               >>
    research_content_reference_operation_index;

}
}

FC_REFLECT(deip::research_content_references_history::research_content_reference_operation_object,
           (id)(trx_id)(block)(timestamp)(serialized_op))
CHAINBASE_SET_INDEX_TYPE(deip::research_content_references_history::research_content_reference_operation_object,
                         deip::research_content_references_history::research_content_reference_operation_index)