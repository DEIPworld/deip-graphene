#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_research_token_sale_operation::validate() const
{
    validate_account_name(research_group);
    validate_160_bits_hexadecimal_string(external_id);
    validate_160_bits_hexadecimal_string(research_external_id);

    FC_ASSERT(security_tokens_on_sale.size() > 0, "Security tokens on sale are not specified");

    for (const auto& security_token_on_sale : security_tokens_on_sale)
    {
        FC_ASSERT(security_token_on_sale.amount > 0, "Security token amount is not specified ${1}", ("1", security_token_on_sale));
    }

    FC_ASSERT(soft_cap.amount > 0,
      "Soft cap should be > 0. Provided amount: ${1}.",
      ("1", soft_cap.to_string()));

    FC_ASSERT(hard_cap.amount > 0,
      "Hard cap should be > 0. Provided amount: ${1}.",
      ("1", hard_cap.to_string()));

    FC_ASSERT(soft_cap.symbol == hard_cap.symbol,
      "Assets does not match. Soft cap: ${1}. Hard cap: ${2}.",
      ("1", soft_cap)("2", hard_cap));

    FC_ASSERT(hard_cap >= soft_cap,
      "Hard cap should be greater or equal to soft cap. Hard cap: ${1}. Soft cap: ${2}.",
      ("1", hard_cap)("2", soft_cap));

    FC_ASSERT(end_time > start_time,
      "End time should be greater than start time. Provided end time: ${1}, Provided start time: ${2}.",
      ("1", end_time)("2", start_time));
}

} /* deip::protocol */
} /* protocol */