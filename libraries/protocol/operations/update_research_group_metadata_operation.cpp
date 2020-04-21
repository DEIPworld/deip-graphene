#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void update_research_group_metadata_operation::validate() const
{
    validate_account_name(creator);
    validate_160_bits_hexadecimal_string(research_group_external_id);

    FC_ASSERT(!research_group_name.empty(), "Name cannot be empty");
    FC_ASSERT(fc::is_utf8(research_group_name), "Name is not valid UTF8 string");
    FC_ASSERT(!research_group_description.empty(), "Description cannot be empty");
    FC_ASSERT(fc::is_utf8(research_group_description), "Description is not valid UTF8 string");
}

} /* deip::protocol */
} /* protocol */