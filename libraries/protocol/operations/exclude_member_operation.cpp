#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void exclude_member_operation::validate() const
{
    validate_account_name(creator);
    validate_account_name(account_to_exclude);
    validate_160_bits_hexadecimal_string(research_group_external_id);
}

} /* deip::protocol */
} /* protocol */