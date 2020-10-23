#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_security_token_operation::validate() const
{
    validate_account_name(research_group);
    validate_160_bits_hexadecimal_string(external_id);
    validate_160_bits_hexadecimal_string(research_external_id);
    FC_ASSERT(amount > 0, "Amount is not specified");
}

} // namespace protocol
} // namespace deip


DEFINE_STATIC_VARIANT_TYPE(deip::protocol::tokenization_options)