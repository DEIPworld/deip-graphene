#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_research_content_operation::validate() const
{
    validate_account_name(research_group);
    validate_160_bits_hexadecimal_string(external_id);
    validate_160_bits_hexadecimal_string(research_external_id);

    FC_ASSERT(!content.empty(), "Content is required.");
    FC_ASSERT(title.size() <= DEIP_MAX_TITLE_SIZE);
    FC_ASSERT(content.size() <= DEIP_MAX_MEMO_SIZE);
    
    FC_ASSERT(!authors.empty(), "Content author(s) are required.");
    for (auto& author : authors)
    {
        validate_account_name(author);
    }

    for (auto& ref_id : references)
    {
        validate_160_bits_hexadecimal_string(ref_id);
    }

    for (auto& ref : foreign_references)
    {
        FC_ASSERT(!ref.empty(), "Foreign reference link cannot be empty");
        FC_ASSERT(fc::is_utf8(ref), "Foreign reference link is not valid UTF8 string");
    }
}

} /* deip::protocol */
} /* protocol */