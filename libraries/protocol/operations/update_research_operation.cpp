#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void update_research_operation::validate() const
{
    validate_account_name(account);
    validate_160_bits_hexadecimal_string(external_id);

    if (description.valid())
    {
        FC_ASSERT(!(*description).empty(), "Research description cannot be empty");
        FC_ASSERT(fc::is_utf8(*description), "Research description is not valid UTF-8 string");
    }

    FC_ASSERT(!review_share.valid(), "'review_share' field is deprecated, use join_project_contract op");
    FC_ASSERT(!compensation_share.valid(), "'compensation_share' field is deprecated, use join_project_contract op");

    if (members.valid()) // deprecated
    {
        const auto& list = *members;
        for (auto& member : list)
        {
            validate_account_name(member);
        }
    }

    FC_ASSERT(update_extensions.size() == 0, "Research transfer is not supported currently");
}

} /* deip::protocol */
} /* protocol */

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::update_research_extension)