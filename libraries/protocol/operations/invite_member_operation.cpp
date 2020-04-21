#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void invite_member_operation::validate() const
{
    validate_account_name(creator);
    validate_account_name(invitee);
    validate_160_bits_hexadecimal_string(research_group_external_id);

    FC_ASSERT(research_group_token_amount_in_percent > 0 && research_group_token_amount_in_percent <= DEIP_100_PERCENT,
              "Research group tokens amount should be > 0. Provided amount: ${1}.",
              ("1", research_group_token_amount_in_percent));

    FC_ASSERT(fc::is_utf8(cover_letter),
              "Cover letter should be valid UTF8 string");

    FC_ASSERT(expiration_time > fc::time_point_sec());
}

} /* deip::protocol */
} /* protocol */