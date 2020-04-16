#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void sign_nda_contract_operation::validate() const
{
    validate_account_name(contract_signer);
    validate_520_bits_hexadecimal_string(signature);
}

} /* deip::protocol */
} /* protocol */