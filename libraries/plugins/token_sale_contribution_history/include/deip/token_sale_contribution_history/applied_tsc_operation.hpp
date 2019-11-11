#pragma once

#include <deip/protocol/operations.hpp>
#include "tsc_operation_object.hpp"

namespace deip {
namespace tsc_history {

using deip::protocol::transaction_id_type;
using deip::protocol::operation;

struct applied_tsc_operation
{
    applied_tsc_operation();
    applied_tsc_operation(const tsc_operation_object& op_obj);

    transaction_id_type trx_id;
    uint32_t block = 0;

    fc::time_point_sec timestamp;
    operation op;
};

}
}

FC_REFLECT(deip::tsc_history::applied_tsc_operation, (trx_id)(block)(timestamp)(op))