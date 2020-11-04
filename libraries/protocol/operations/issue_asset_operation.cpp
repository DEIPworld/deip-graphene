#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void issue_asset_operation::validate() const
{
    validate_account_name(issuer);
    validate_account_name(recipient);
    FC_ASSERT(amount.symbol != DEIP_SYMBOL, "You cannot issue core asset.");
    FC_ASSERT(amount.amount > 0, "Amount to issue must be greater than 0");

    if (memo.valid())
    {
        const auto& val = *memo;
        FC_ASSERT(val.size() < DEIP_MAX_MEMO_SIZE, "Memo is too large");
        FC_ASSERT(fc::is_utf8(val), "Memo is not UTF8");
    }
}


} // namespace protocol
} // namespace deip

