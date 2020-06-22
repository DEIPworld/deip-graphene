#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_research_operation::validate() const
{
    validate_account_name(research_group);
    validate_160_bits_hexadecimal_string(external_id);

    FC_ASSERT(disciplines.size() != 0, "Research must be related to one or several disciplines.");
    for (const auto& external_id : disciplines)
    {
        validate_160_bits_hexadecimal_string(external_id);
    }

    FC_ASSERT(!title.empty(), "Research name cannot be empty.");
    FC_ASSERT(title.size() <= DEIP_MAX_TITLE_SIZE);
    FC_ASSERT(abstract.size() <= DEIP_MAX_MEMO_SIZE);

    if (review_share.valid())
    {
        const auto& share = *review_share;
        const auto& min_review_share = percent(0);
        const auto& max_review_share = percent(DEIP_1_PERCENT * 50);
        FC_ASSERT(share >= min_review_share && share <= max_review_share,
          "Percent for review should be in ${1} to ${2} range. Provided value: ${3}",
          ("1", min_review_share)("2", max_review_share)("3", share));
    }

    if (compensation_share.valid())
    {
        const auto& share = *compensation_share;
        const auto& min_compensation_share = percent(0);
        const auto& max_compensation_share = percent(DEIP_1_PERCENT * 50);
        FC_ASSERT(share >= min_compensation_share && share <= max_compensation_share,
          "Percent for dropout compensation should be in ${1} to ${2} range. Provided value: ${3}.",
          ("1", min_compensation_share)("2", max_compensation_share)("3", share));
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