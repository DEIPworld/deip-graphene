#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void update_research_operation::validate() const
{
    validate_account_name(research_group);
    validate_160_bits_hexadecimal_string(external_id);

    FC_ASSERT(!title.empty(), "Research title cannot be empty");
    FC_ASSERT(fc::is_utf8(title), "Research title is not valid UTF-8 string");
    FC_ASSERT(!abstract.empty(), "Research abstract cannot be empty");
    FC_ASSERT(fc::is_utf8(abstract), "Research abstract is not valid UTF-8 string");
    FC_ASSERT(!permlink.empty(), "Research permlink cannot be empty");
    FC_ASSERT(fc::is_utf8(permlink), "Research permlink is not valid UTF-8 string");

    const auto& min_review_share = percent(0);
    const auto& max_review_share = percent(DEIP_1_PERCENT * 50);
    FC_ASSERT(review_share >= min_review_share && review_share <= max_review_share,
              "Percent for review should be in ${1} to ${2} range. Provided value: ${3}",
              ("1", review_share)("3", review_share));

    const auto& min_compensation_share = percent(0);
    const auto& max_compensation_share = percent(DEIP_1_PERCENT * 50);
    FC_ASSERT(compensation_share >= min_compensation_share && compensation_share <= max_compensation_share,
              "Percent for dropout compensation should be in 0 to 100 range. Provided value: ${1}.",
              ("1", compensation_share));
}

} /* deip::protocol */
} /* protocol */