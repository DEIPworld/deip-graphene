#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void fulfill_request_by_nda_contract_operation::validate() const
{
    validate_account_name(grantor);
    FC_ASSERT(encrypted_payload_encryption_key.size() > 0, "Encrypted payload hash must be specified");
    FC_ASSERT(fc::is_utf8(encrypted_payload_encryption_key), "Encrypted payload hash is not valid UTF8 string");
    FC_ASSERT(proof_of_encrypted_payload_encryption_key.size() > 0, "Encrypted payload IV must be specified");
    FC_ASSERT(fc::is_utf8(proof_of_encrypted_payload_encryption_key), "Encrypted payload IV is not valid UTF8 string");

}

} /* deip::protocol */
} /* protocol */