#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

struct asset_trait_validator
{
    typedef void result_type;
    template <typename T> void operator()(const T& trait) const
    {
    }

    void operator()(const research_security_token_trait& trait) const
    {
        validate_account_name(trait.research_group);
        validate_160_bits_hexadecimal_string(trait.research_external_id);
    }

    void operator()(const research_license_revenue_trait& trait) const
    {
        const auto precision = trait.holders_share.precision(trait.holders_share.decimals);
        const percent full_share = percent(100 * precision, trait.holders_share.decimals);
        // Currently we allow 100% of revenue per asset only
        FC_ASSERT(trait.holders_share == full_share, "Total revenue share ${1} is not equal to full share ${2}", ("1", trait.holders_share)("2", full_share));
    }
};

void create_asset_operation::validate() const
{
    validate_account_name(issuer);
    FC_ASSERT(symbol.size() > 0 && symbol.size() < 7, "Asset symbol must be specified");
    FC_ASSERT(fc::is_utf8(symbol), "Asset symbol is not valid UTF8 string");
    FC_ASSERT(precision < 15, "Precision must be less than 15.");
    FC_ASSERT(max_supply <= DEIP_MAX_SHARE_SUPPLY, "Max supply is too large");

    if (description.size() > 0) 
    {
        FC_ASSERT(fc::is_utf8(description), "Description is not valid UTF8 string");
    }

    asset_trait_validator trait_validator;
    for (const auto& asset_trait : traits)
    {
        trait_validator(asset_trait);
    }
}

} // namespace protocol
} // namespace deip


DEFINE_STATIC_VARIANT_TYPE(deip::protocol::asset_trait_type)