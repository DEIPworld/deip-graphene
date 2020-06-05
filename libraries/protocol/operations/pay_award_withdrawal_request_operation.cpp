#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void pay_award_withdrawal_request_operation::validate() const
{
    validate_account_name(payer);
    // validate_160_bits_hexadecimal_string(award_number);
    // validate_160_bits_hexadecimal_string(payment_number);
    if (subaward_number.valid())
    {
        // validate_160_bits_hexadecimal_string(*subaward_number);
    }
}


} /* deip::protocol */
} /* protocol */