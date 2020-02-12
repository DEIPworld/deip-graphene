#pragma once

#include <deip/protocol/operations.hpp>
#include "research_content_eci_operation_object.hpp"

namespace deip {
namespace research_content_eci_history {

using deip::protocol::transaction_id_type;
using deip::protocol::operation;

struct applied_research_content_eci_operation
{
    applied_research_content_eci_operation();
    applied_research_content_eci_operation(const research_content_eci_operation_object& op_obj);

    transaction_id_type trx_id;
    uint32_t block = 0;

    fc::time_point_sec timestamp;
    operation op;
};

}
}

FC_REFLECT(deip::research_content_eci_history::applied_research_content_eci_operation, (trx_id)(block)(timestamp)(op))