#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void update_research_metadata_operation::validate() const
{
    validate_account_name(creator);
    validate_160_bits_hexadecimal_string(research_group_external_id);
    validate_160_bits_hexadecimal_string(research_external_id);

    FC_ASSERT(!research_title.empty(), "Title cannot be empty");
    FC_ASSERT(fc::is_utf8(research_title), "Title is not valid UTF8 string");
    FC_ASSERT(!research_abstract.empty(), "Abstract cannot be empty");
    FC_ASSERT(fc::is_utf8(research_abstract), "Abstract is not valid UTF8 string");
}

} /* deip::protocol */
} /* protocol */