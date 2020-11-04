#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void reserve_asset_operation::validate() const
{
    validate_account_name(owner);
    FC_ASSERT(amount.symbol != DEIP_SYMBOL, "You cannot reserve core asset.");
    FC_ASSERT(amount.amount > 0, "Amount to reserve must be greater than 0");
}

} // namespace protocol
} // namespace deip
