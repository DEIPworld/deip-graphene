#include <deip/research_content_reference_history/applied_research_content_reference_operation.hpp>

namespace deip {
namespace research_content_references_history {

applied_research_content_reference_operation::applied_research_content_reference_operation()
{
}

applied_research_content_reference_operation::applied_research_content_reference_operation(const research_content_reference_operation_object& op_obj)
    : trx_id(op_obj.trx_id)
    , block(op_obj.block)
    , timestamp(op_obj.timestamp)
{
    // fc::raw::unpack( op_obj.serialized_op, op );     // g++ refuses to compile this as ambiguous
    op = fc::raw::unpack<operation>(op_obj.serialized_op);
}
}
}