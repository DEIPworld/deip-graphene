#pragma once

#include <deip/protocol/operations.hpp>
#include "operation_objects.hpp"

namespace deip {
namespace blockchain_history {

using deip::protocol::transaction_id_type;
using deip::protocol::operation;

struct applied_operation
{
    applied_operation();
    applied_operation(const operation_object& op_obj);

    transaction_id_type trx_id;
    uint32_t block = 0;
    uint32_t trx_in_block = 0;
    uint16_t op_in_trx = 0;
    fc::time_point_sec timestamp;
    operation op;
};
}
}

FC_REFLECT(deip::blockchain_history::applied_operation, (trx_id)(block)(trx_in_block)(op_in_trx)(timestamp)(op))