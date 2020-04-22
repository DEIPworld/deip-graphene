
#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_proposal_operation::validate() const
{
    validate_account_name(creator);
    FC_ASSERT(!proposed_ops.empty());
    // for (const auto& op : proposed_ops)
    // {
    //     // operation_validate(op);
    // }
}

} // namespace protocol
} // namespace deip