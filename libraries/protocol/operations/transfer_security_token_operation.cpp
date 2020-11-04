#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void transfer_security_token_operation::validate() const
{
    validate_account_name(from);
    validate_account_name(to);
    
    validate_160_bits_hexadecimal_string(security_token_external_id);
    FC_ASSERT(amount > 0, "Amount is not specified");
    FC_ASSERT(memo.size() < DEIP_MAX_MEMO_SIZE, "Memo is too large");
    FC_ASSERT(fc::is_utf8(memo), "Memo is not UTF8");
}

} // namespace protocol
} // namespace deip