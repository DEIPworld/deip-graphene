#pragma once

#include <deip/protocol/operations.hpp>
#include "cr_operation_object.hpp"

namespace deip {
namespace cr_history {

using deip::protocol::transaction_id_type;
using deip::protocol::operation;

struct applied_cr_operation
{
    applied_cr_operation();
    applied_cr_operation(const cr_operation_object& op_obj);

    transaction_id_type trx_id;
    uint32_t block = 0;

    fc::time_point_sec timestamp;
    operation op;
};

}
}

FC_REFLECT(deip::cr_history::applied_cr_operation, (trx_id)(block)(timestamp)(op))