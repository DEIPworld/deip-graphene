#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void join_research_group_membership_operation::validate() const
{
    validate_account_name(member);
    validate_account_name(research_group);

    const auto& min_reward_share = percent(DEIP_1_PERCENT);
    const auto& max_reward_share = percent(DEIP_1_PERCENT * 100);
    FC_ASSERT(reward_share >= min_reward_share && reward_share < max_reward_share,
      "Research group tokens share should be in range of ${1} - ${2}. Provided value: ${3}.",
      ("1", min_reward_share)("2", max_reward_share)("3", reward_share));

    if (researches.valid())
    {
        const auto& list = *researches;
        for (auto& external_id : list)
        {
            validate_160_bits_hexadecimal_string(external_id);
        }
    }
}

} /* deip::protocol */
} /* protocol */