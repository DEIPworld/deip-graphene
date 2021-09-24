#include <deip/protocol/deip_operations.hpp>

namespace deip {
namespace protocol {

void create_contract_agreement_operation::validate() const
{
    validate_160_bits_hexadecimal_string(external_id);
    validate_account_name(creator);
    FC_ASSERT(!parties.empty(), "At least one party should be specified");
    validate_256_bits_hexadecimal_string(hash);
}

} // namespace protocol
} // namespace deip
