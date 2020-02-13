#include <deip/account_eci_history/applied_account_eci_operation.hpp>

namespace deip {
namespace account_eci_history {

applied_account_eci_operation::applied_account_eci_operation()
{
}

applied_account_eci_operation::applied_account_eci_operation(const account_eci_operation_object& op_obj)
    : trx_id(op_obj.trx_id)
    , block(op_obj.block)
    , timestamp(op_obj.timestamp)
{
    // fc::raw::unpack( op_obj.serialized_op, op );     // g++ refuses to compile this as ambiguous
    op = fc::raw::unpack<operation>(op_obj.serialized_op);
}
}
}
