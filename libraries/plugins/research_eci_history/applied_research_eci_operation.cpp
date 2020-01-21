#include <deip/research_eci_history/applied_research_eci_operation.hpp>

namespace deip {
namespace research_eci_history {

applied_research_eci_operation::applied_research_eci_operation()
{
}

applied_research_eci_operation::applied_research_eci_operation(const research_eci_operation_object& op_obj)
    : trx_id(op_obj.trx_id)
    , block(op_obj.block)
    , timestamp(op_obj.timestamp)
{
    // fc::raw::unpack( op_obj.serialized_op, op );     // g++ refuses to compile this as ambiguous
    op = fc::raw::unpack<operation>(op_obj.serialized_op);
}
}
}