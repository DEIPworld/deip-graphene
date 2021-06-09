#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_research_operation::validate() const
{
    validate_account_name(account);
    validate_160_bits_hexadecimal_string(external_id);

    FC_ASSERT(disciplines.size() != 0, "Research must be related to one or several disciplines.");
    for (const auto& external_id : disciplines)
    {
        validate_160_bits_hexadecimal_string(external_id);
    }

    FC_ASSERT(!description.empty(), "Research description cannot be empty.");
    FC_ASSERT(description.size() <= DEIP_MAX_MEMO_SIZE);
    
    FC_ASSERT(!review_share.valid(), "'review_share' field is deprecated, use join_project_contract op");
    FC_ASSERT(!compensation_share.valid(), "'compensation_share' field is deprecated, use join_project_contract op");

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