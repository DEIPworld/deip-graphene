#include <deip/witness/witness_operations.hpp>

#include <deip/protocol/operation_util_impl.hpp>

namespace deip {
namespace witness {

void enable_content_editing_operation::validate() const
{
    chain::validate_account_name(account);
}
}
} // deip::witness

DEFINE_OPERATION_TYPE(deip::witness::witness_plugin_operation)
