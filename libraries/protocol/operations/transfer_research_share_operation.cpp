#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void transfer_research_share_operation::validate() const
{
    validate_160_bits_hexadecimal_string(research_external_id);
    validate_account_name(sender);
    validate_account_name(receiver);
    FC_ASSERT(share > percent(0) , "Transfer share must be greater than 0 %");

}

} /* deip::protocol */
} /* protocol */