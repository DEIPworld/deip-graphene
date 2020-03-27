#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void reject_award_operation::validate() const
{
    validate_account_name(rejector);
    validate_award_number(award_number);
}

} /* deip::protocol */
} /* protocol */