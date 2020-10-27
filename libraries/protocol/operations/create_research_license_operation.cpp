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

        FC_ASSERT(fee_model.beneficiaries.size() != 0, "Revenue beneficiaries are not specified");

        for (const auto& pair : fee_model.beneficiaries)
        {
            validate_160_bits_hexadecimal_string(pair.first);
        }

        const std::set<uint8_t>& all_decimals = std::accumulate(
            fee_model.beneficiaries.begin(), fee_model.beneficiaries.end(), std::set<uint8_t>(),
            [](std::set<uint8_t> acc, const std::pair<external_id_type, percent>& beneficiary) {
                acc.insert(beneficiary.second.decimals);
                return acc;
            });

        FC_ASSERT(all_decimals.size() == 1, "Revenue shares should have the same precision for all beneficiaries");

        const uint8_t decimals = *all_decimals.begin();

        const percent& total_share = std::accumulate(
            fee_model.beneficiaries.begin(), fee_model.beneficiaries.end(), percent(0, decimals),
            [](percent acc, const std::pair<external_id_type, percent>& beneficiary) {
                return acc + beneficiary.second;
            });

        const auto precision = total_share.precision(total_share.decimals);
        const percent full_share = percent(100 * precision, decimals);

        FC_ASSERT(total_share == full_share, "Total revenue share ${1} is not equal to full share ${2}", ("1", total_share)("2", full_share));
    }
};

void create_research_license_operation::validate() const
{
    validate_account_name(licenser);
    validate_account_name(licensee);
    validate_160_bits_hexadecimal_string(external_id);
    validate_160_bits_hexadecimal_string(research_external_id);

    license_conditions_validator conditions_validator;
    conditions_validator(license_conditions);
}

} // namespace protocol
} // namespace deip

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::license_agreement_types)