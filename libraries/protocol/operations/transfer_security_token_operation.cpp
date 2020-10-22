#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void transfer_security_token_operation::validate() const
{
    validate_account_name(from);
    validate_account_name(to);
    validate_160_bits_hexadecimal_string(security_token_external_id);
    FC_ASSERT(amount > 0, "Amount is not specified");
}

} // namespace protocol
} // namespace deip