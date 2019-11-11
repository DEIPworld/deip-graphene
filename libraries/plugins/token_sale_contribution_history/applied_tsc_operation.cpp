#include <deip/token_sale_contribution_history/applied_tsc_operation.hpp>

namespace deip {
namespace tsc_history {

applied_tsc_operation::applied_tsc_operation()
{
}

applied_tsc_operation::applied_tsc_operation(const tsc_operation_object& op_obj)
    : trx_id(op_obj.trx_id)
    , block(op_obj.block)
    , timestamp(op_obj.timestamp)
{
    // fc::raw::unpack( op_obj.serialized_op, op );     // g++ refuses to compile this as ambiguous
    op = fc::raw::unpack<operation>(op_obj.serialized_op);
}
}
}