#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_research_content_operation::validate() const
{
    validate_account_name(creator);
    validate_160_bits_hexadecimal_string(research_group_external_id);
    validate_160_bits_hexadecimal_string(research_external_id);

    FC_ASSERT(!content.empty(), "Content cannot be empty.");
    FC_ASSERT(!permlink.empty(), "Permlink cannot be empty.");
    FC_ASSERT(fc::is_utf8(permlink), "Permlink is not valid UTF8 string.");
    FC_ASSERT(!authors.empty(), "Content should have author(s).");
    for (auto& author : authors)
    {
        validate_account_name(author);
    }

    for (auto& ref : references)
    {
        FC_ASSERT(ref > 0, "Reference id must be > 0. Provided id: ${1}.", ("1", ref));
    }

    for (auto& ref : external_references)
    {
        FC_ASSERT(!ref.empty(), "External reference link cannot be empty");
        FC_ASSERT(fc::is_utf8(ref), "External reference link is not valid UTF8 string");
    }
}

} /* deip::protocol */
} /* protocol */