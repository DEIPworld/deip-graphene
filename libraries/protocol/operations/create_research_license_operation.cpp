#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

struct license_conditions_validator
{
    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
    }

    void operator()(const licensing_fee_type& fee_model) const
    {
        FC_ASSERT(fee_model.terms.size() != 0, "Terms are not specified for research license");
        if (fee_model.fee.valid()) 
        {
            const auto& fee = *fee_model.fee;
            FC_ASSERT(fee > asset(0, fee.symbol), "Licensing fee is specified but its value is not positive ${1}", ("1", fee));
        }
    }
};

void create_research_license_operation::validate() const
{
    validate_account_name(research_group);
    validate_account_name(licensee);
    validate_160_bits_hexadecimal_string(external_id);
    validate_160_bits_hexadecimal_string(research_external_id);

    license_conditions_validator conditions_validator;
    conditions_validator(license_conditions);
}

} // namespace protocol
} // namespace deip

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::license_agreement_types)