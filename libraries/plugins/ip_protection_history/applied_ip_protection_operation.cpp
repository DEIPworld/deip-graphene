#include <deip/ip_protection_history/applied_ip_protection_operation.hpp>

namespace deip {
namespace ip_protection_history {

applied_ip_protection_operation::applied_ip_protection_operation()
{
}

applied_ip_protection_operation::applied_ip_protection_operation(const ip_protection_operation_object& op_obj)
    : trx_id(op_obj.trx_id)
    , block(op_obj.block)
    , timestamp(op_obj.timestamp)
{
    // fc::raw::unpack( op_obj.serialized_op, op );     // g++ refuses to compile this as ambiguous
    op = fc::raw::unpack<operation>(op_obj.serialized_op);
}
}
}