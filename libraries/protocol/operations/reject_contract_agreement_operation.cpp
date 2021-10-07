#include <deip/protocol/deip_operations.hpp>

namespace deip {
namespace protocol {

void reject_contract_agreement_operation::validate() const
{
    validate_160_bits_hexadecimal_string(external_id);
    validate_account_name(party);
}

} // namespace protocol
} // namespace deip
