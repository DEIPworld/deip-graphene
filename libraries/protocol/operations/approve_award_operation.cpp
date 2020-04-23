#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void approve_award_operation::validate() const
{
    validate_account_name(approver);
    validate_160_bits_hexadecimal_string(award_number);
}

} /* deip::protocol */
} /* protocol */