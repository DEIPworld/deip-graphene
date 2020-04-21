#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_research_operation::validate() const
{
    validate_account_name(creator);
    validate_160_bits_hexadecimal_string(research_group_external_id);

    FC_ASSERT(disciplines.size() != 0, "Research must be related to one or several disciplines.");
    FC_ASSERT(!std::any_of(disciplines.begin(), disciplines.end(), [](int64_t discipline_id){
        return discipline_id == 0;
    }), "Research cannot be related to 'common' discipline.");

    FC_ASSERT(!title.empty(), "Research name cannot be empty.");
    FC_ASSERT(!abstract.empty(), "Research abstract cannot be empty.");

    FC_ASSERT(fc::is_utf8(permlink), "Research permlink should be valid UTF8 string.");
    FC_ASSERT(permlink.size() < DEIP_MAX_PERMLINK_LENGTH,
              "Research permlink is too long. Provided permlink: ${1}.",
              ("1", permlink));

    FC_ASSERT(review_share_in_percent >= 0 && review_share_in_percent <= 50 * DEIP_1_PERCENT,
              "Percent for review should be in 0 to 50 range. Provided value: ${1}",
              ("1", review_share_in_percent));

    FC_ASSERT(dropout_compensation_in_percent >= 0 && dropout_compensation_in_percent <= DEIP_100_PERCENT,
              "Percent for dropout compensation should be in 0 to 100 range. Provided value: ${1}.",
              ("1", dropout_compensation_in_percent));
}

} /* deip::protocol */
} /* protocol */