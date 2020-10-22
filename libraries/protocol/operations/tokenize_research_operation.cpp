#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void tokenize_research_operation::validate() const
{
    validate_account_name(research_group);
    validate_160_bits_hexadecimal_string(external_id);
    validate_160_bits_hexadecimal_string(research_external_id);
    FC_ASSERT(amount > 0, "Weight should be in 1% to 100% range");
}

} // namespace protocol
} // namespace deip

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::tokenization_condition_models)