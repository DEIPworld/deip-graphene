
#include <deip/protocol/deip_operations.hpp>
#include <deip/protocol/operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

using deip::protocol::authority_type;

void delete_proposal_operation::validate() const
{
    validate_160_bits_hexadecimal_string(external_id);
    validate_account_name(account);
    validate_enum_value_by_range(authority, static_cast<uint16_t>(authority_type::FIRST), static_cast<uint16_t>(authority_type::LAST));
}

} // namespace protocol
} // namespace deip