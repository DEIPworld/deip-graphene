#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {


using deip::protocol::asset;
using deip::protocol::percent;

struct licensing_fee_type
{
    string terms;
    flat_map<string, percent> beneficiaries;
    optional<asset> fee;
    optional<time_point_sec> expiration_time;
};

typedef fc::static_variant<
  licensing_fee_type
  >
  license_agreement_types;

struct create_research_license_operation : public entity_operation
{
    external_id_type external_id;
    external_id_type research_external_id;
    account_name_type licenser;
    account_name_type licensee;

    license_agreement_types license_conditions;

    extensions_type extensions;

    string entity_id() const { return "external_id"; }
    external_id_type get_entity_id() const { return external_id; }

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(licenser);

        if (license_conditions.which() == license_agreement_types::tag<licensing_fee_type>::value)
        {
            const auto fee_model = license_conditions.get<licensing_fee_type>();
            if (fee_model.fee.valid())
            {
                a.insert(licensee);
            }
        }
    }
};


}
}


FC_REFLECT(deip::protocol::create_research_license_operation,
  (external_id)
  (research_external_id)
  (licenser)
  (licensee)
  (license_conditions)
  (extensions)
)

FC_REFLECT(deip::protocol::licensing_fee_type, (terms)(beneficiaries)(fee)(expiration_time))

DECLARE_STATIC_VARIANT_TYPE(deip::protocol::license_agreement_types)
FC_REFLECT_TYPENAME(deip::protocol::license_agreement_types)