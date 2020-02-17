#pragma once
#include <deip/chain/schema/deip_object_types.hpp>

#include <fc/shared_buffer.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/operations.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <boost/multi_index/composite_key.hpp>

#ifndef IP_PROTECTION_HISTORY_SPACE_ID
#define IP_PROTECTION_SPACE_ID 13
#endif

namespace deip {
namespace ip_protection_history {

using deip::protocol::transaction_id_type;
using deip::protocol::operation;
using namespace deip::chain;

enum ip_protection_history_object_type
{
    ip_protection_operations_history = (IP_PROTECTION_SPACE_ID << 8),
    all_ip_protection_operations_history,
    create_research_materials_history
};

class ip_protection_operation_object : public object<ip_protection_operations_history, ip_protection_operation_object>
{

    ip_protection_operation_object() = delete;
 public:
    template <typename Constructor, typename Allocator>
    ip_protection_operation_object(Constructor&& c, allocator<Allocator> a)
        : serialized_op(a)
    {
        c(*this);
    }

    typedef typename object<ip_protection_operations_history, ip_protection_operation_object>::id_type id_type;

    id_type id;

    transaction_id_type trx_id;
    uint32_t block = 0;

    time_point_sec timestamp;
    fc::shared_buffer serialized_op;
};

struct by_transaction_id;
typedef chainbase::shared_multi_index_container<ip_protection_operation_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<ip_protection_operation_object,
                                                                       ip_protection_operation_object::id_type,
                                                                      &ip_protection_operation_object::id>>
#ifndef SKIP_BY_TX_ID
                                                ,
                                                ordered_unique<tag<by_transaction_id>,
                                                               composite_key<ip_protection_operation_object,
                                                                             member<ip_protection_operation_object,
                                                                                    transaction_id_type,
                                                                                    &ip_protection_operation_object::trx_id>,
                                                                             member<ip_protection_operation_object,
                                                                                     ip_protection_operation_object::id_type,
                                                                                    &ip_protection_operation_object::id>>>
#endif
                                                >>
        ip_protection_operation_index;

}
}

FC_REFLECT(deip::ip_protection_history::ip_protection_operation_object,
           (id)(trx_id)(block)(timestamp)(serialized_op))
CHAINBASE_SET_INDEX_TYPE(deip::ip_protection_history::ip_protection_operation_object, deip::ip_protection_history::ip_protection_operation_index)
