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

    if (title.valid())
    {
        FC_ASSERT(!(*title).empty(), "Research title cannot be empty");
        FC_ASSERT(fc::is_utf8(*title), "Research title is not valid UTF-8 string");
    }

    if (abstract.valid())
    {
        FC_ASSERT(!(*abstract).empty(), "Research abstract cannot be empty");
        FC_ASSERT(fc::is_utf8(*abstract), "Research abstract is not valid UTF-8 string");
    }

    if (permlink.valid())
    {
        FC_ASSERT(!(*permlink).empty(), "Research permlink cannot be empty");
        FC_ASSERT(fc::is_utf8(*permlink), "Research permlink is not valid UTF-8 string");
    }

    if (review_share.valid())
    {
        const auto& share = *review_share;
        const auto& min_review_share = percent(0);
        const auto& max_review_share = percent(DEIP_1_PERCENT * 50);
        FC_ASSERT(share >= min_review_share && share <= max_review_share,
          "Percent for review should be in ${1} to ${2} range. Provided value: ${3}",
          ("1", min_review_share)("1", max_review_share)("3", share));
    }
      
    if (compensation_share.valid())
    {
        const auto& share = *compensation_share;
        const auto& min_compensation_share = percent(0);
        const auto& max_compensation_share = percent(DEIP_1_PERCENT * 50);
        FC_ASSERT(share >= min_compensation_share && share <= max_compensation_share,
          "Percent for dropout compensation should be in ${1} to ${2} range. Provided value: ${3}.",
          ("1", min_compensation_share)("1", max_compensation_share)("1", share));
    }

    if (members.valid())
    {
        const auto& list = *members;
        for (auto& member : list)
        {
            validate_account_name(member);
        }
    }
}

} /* deip::protocol */
} /* protocol */