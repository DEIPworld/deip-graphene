#pragma once

#include <deip/chain/schema/deip_object_types.hpp>
#include <fc/shared_buffer.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/operations.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <boost/multi_index/composite_key.hpp>

#ifndef CR_HISTORY_SPACE_ID
#define CR_HISTORY_SPACE_ID 15
#endif


namespace deip {
namespace cr_history {

using deip::protocol::transaction_id_type;
using deip::protocol::operation;
using namespace deip::chain;

enum cr_history_object_type
{
    cr_operations_history = (CR_HISTORY_SPACE_ID << 8),
    all_cr_operations_history
};

class cr_operation_object : public object<cr_operations_history, cr_operation_object>
{
    cr_operation_object() = delete;

 public:
    template <typename Constructor, typename Allocator>
    cr_operation_object(Constructor&& c, allocator<Allocator> a)
        : serialized_op(a)
    {
        c(*this);
    }

    typedef typename object<cr_operations_history, cr_operation_object>::id_type id_type;

    id_type id;

    transaction_id_type trx_id;
    uint32_t block = 0;

    time_point_sec timestamp;
    fc::shared_buffer serialized_op;
};

struct by_transaction_id;
typedef chainbase::shared_multi_index_container<cr_operation_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<cr_operation_object,
                                                                       cr_operation_object::id_type,
                                                                       &cr_operation_object::id>>
#ifndef SKIP_BY_TX_ID
                                                                      ,
                                                ordered_unique<tag<by_transaction_id>,
                                                               composite_key<cr_operation_object,
                                                                             member<cr_operation_object,
                                                                                    transaction_id_type,
                                                                                    &cr_operation_object::trx_id>,
                                                                             member<cr_operation_object,
                                                                                     cr_operation_object::id_type,
                                                                                     &cr_operation_object::id>>>
#endif
                                                >>
        cr_operation_index;

}
}

FC_REFLECT(deip::cr_history::cr_operation_object,
           (id)(trx_id)(block)(timestamp)(serialized_op))
CHAINBASE_SET_INDEX_TYPE(deip::cr_history::cr_operation_object,
        deip::cr_history::cr_operation_index)