#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {


struct research_security_token_trait
{
    external_id_type research_external_id;
    account_name_type research_group;
    extensions_type extensions;
};

struct research_license_revenue_trait
{
    percent holders_share;
    extensions_type extensions;
};

typedef fc::static_variant<
  research_security_token_trait,
  research_license_revenue_trait
  > 
  asset_trait_type;


struct asset_trait_auth_extractor
{
    asset_trait_auth_extractor(flat_set<account_name_type>& a)
        : auths(a){};

    typedef void result_type;

    void operator()(const research_security_token_trait& trait)
    {
        auths.insert(trait.research_group);
    }

    template <typename T> void operator()(const T& trait) const
    {
    }

private:
    flat_set<account_name_type>& auths;
};

struct create_asset_operation : public base_operation
{
    account_name_type issuer;
    string symbol;
    uint8_t precision;
    string description;
    share_type max_supply;
    flat_set<asset_trait_type> traits;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(issuer);

        asset_trait_auth_extractor trait_auth_extractor(a);
        for (const auto& asset_trait : traits)
        {
            trait_auth_extractor(asset_trait);
        }
    }
};

} // namespace protocol
} // namespace deip

FC_REFLECT(deip::protocol::create_asset_operation, (issuer)(symbol)(precision)(description)(max_supply)(traits)(extensions))

FC_REFLECT(deip::protocol::research_security_token_trait, (research_external_id)(research_group)(extensions))
FC_REFLECT(deip::protocol::research_license_revenue_trait, (holders_share)(extensions))

DECLARE_STATIC_VARIANT_TYPE(deip::protocol::asset_trait_type)
FC_REFLECT_TYPENAME(deip::protocol::asset_trait_type)