#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_award_withdrawal_request_operation::validate() const
{
    validate_award_number(award_number);
    validate_payment_number(payment_number);
    if (subaward_number.valid())
    {
        validate_award_number(*subaward_number);
    }
    validate_account_name(requester);
    FC_ASSERT(amount.amount > 0, "Amount must be greater than 0");
    FC_ASSERT(description.size() > 0, "Description must be specified");
    FC_ASSERT(fc::is_utf8(description), "Description is not valid UTF8 string");
    FC_ASSERT(fc::is_utf8(attachment), "Attachment is not valid UTF8 string");
}

} /* deip::protocol */
} /* protocol */