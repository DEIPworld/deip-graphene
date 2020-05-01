#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void join_research_group_operation::validate() const
{
    validate_account_name(member);
    validate_account_name(research_group);

    const auto& min_weight = percent(DEIP_1_PERCENT);
    const auto& max_weight = percent(DEIP_1_PERCENT * 100);
    FC_ASSERT(weight >= min_weight && weight < max_weight,
      "Research group tokens share should be in range of ${1} - ${2}. Provided value: ${3}.",
      ("1", min_weight)("2", max_weight)("3", weight));
}

} /* deip::protocol */
} /* protocol */