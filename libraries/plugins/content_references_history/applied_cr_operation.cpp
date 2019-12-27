#include <deip/content_references_history/applied_cr_operation.hpp>

namespace deip {
namespace cr_history {

applied_cr_operation::applied_cr_operation()
{
}

applied_cr_operation::applied_cr_operation(const cr_operation_object& op_obj)
    : trx_id(op_obj.trx_id)
    , block(op_obj.block)
    , timestamp(op_obj.timestamp)
{
    // fc::raw::unpack( op_obj.serialized_op, op );     // g++ refuses to compile this as ambiguous
    op = fc::raw::unpack<operation>(op_obj.serialized_op);
}
}
}