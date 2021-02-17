#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_nda_content_access_request_operation::validate() const
{
    validate_160_bits_hexadecimal_string(external_id);
    validate_160_bits_hexadecimal_string(nda_external_id);
    validate_account_name(requester);
    validate_256_bits_hexadecimal_string(encrypted_payload_hash);
    validate_128_bits_hexadecimal_string(encrypted_payload_iv);
}

} /* deip::protocol */
} /* protocol */