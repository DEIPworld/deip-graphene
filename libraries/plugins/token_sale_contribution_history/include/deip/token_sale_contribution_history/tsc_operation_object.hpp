#pragma once

#include <deip/chain/schema/deip_object_types.hpp>
#include <fc/shared_buffer.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/operations.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <boost/multi_index/composite_key.hpp>

#ifndef TSC_HISTORY_SPACE_ID
#define TSC_HISTORY_SPACE_ID 14
#endif


namespace deip {
namespace tsc_history {

using deip::protocol::transaction_id_type;
using deip::protocol::operation;
using namespace deip::chain;

enum tsc_history_object_type
{
    tsc_operations_history = (TSC_HISTORY_SPACE_ID << 8),
    all_tsc_operations_history,
    contribute_to_token_sale_history
};

class tsc_operation_object : public object<tsc_operations_history, tsc_operation_object>
{
    tsc_operation_object() = delete;

 public:
    template <typename Constructor, typename Allocator>
    tsc_operation_object(Constructor&& c, allocator<Allocator> a)
        : serialized_op(a)
    {
        c(*this);
    }

    typedef typename object<tsc_operations_history, tsc_operation_object>::id_type id_type;

    id_type id;

    transaction_id_type trx_id;
    uint32_t block = 0;

    time_point_sec timestamp;
    fc::shared_buffer serialized_op;
};

struct by_transaction_id;
typedef chainbase::shared_multi_index_container<tsc_operation_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<tsc_operation_object,
                                                                       tsc_operation_object::id_type,
                                                                      &tsc_operation_object::id>>,
#ifndef SKIP_BY_TX_ID
                                                ordered_unique<tag<by_transaction_id>,
                                                               composite_key<tsc_operation_object,
                                                                             member<tsc_operation_object,
                                                                                    transaction_id_type,
                                                                                    &tsc_operation_object::trx_id>,
                                                                             member<tsc_operation_object,
                                                                                     tsc_operation_object::id_type,
                                                                                    &tsc_operation_object::id>>>
#endif
                                                >>
        tsc_operation_index;

}
}

FC_REFLECT(deip::tsc_history::tsc_operation_object,
           (id)(trx_id)(block)(timestamp)(serialized_op))
CHAINBASE_SET_INDEX_TYPE(deip::tsc_history::tsc_operation_object,
        deip::tsc_history::tsc_operation_index)