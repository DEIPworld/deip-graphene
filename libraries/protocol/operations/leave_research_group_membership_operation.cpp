#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void leave_research_group_membership_operation::validate() const
{
    validate_account_name(member);
    validate_account_name(research_group);
}

} /* deip::protocol */
} /* protocol */