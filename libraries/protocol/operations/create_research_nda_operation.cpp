#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_research_nda_operation::validate() const
{
    validate_160_bits_hexadecimal_string(external_id);
    FC_ASSERT(parties.size() > 1, "At least 2 parties should be specified");
    validate_account_name(creator);
    validate_256_bits_hexadecimal_string(description);
}

} /* deip::protocol */
} /* protocol */