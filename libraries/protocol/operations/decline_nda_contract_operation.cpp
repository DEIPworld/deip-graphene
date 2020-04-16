#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void decline_nda_contract_operation::validate() const
{
    validate_account_name(decliner);
}

} /* deip::protocol */
} /* protocol */