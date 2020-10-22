#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct tokenization_agreement_type
{
    string terms;
};

typedef fc::static_variant<
  tokenization_agreement_type
  >
  tokenization_condition_models;


struct tokenize_research_operation : public base_operation
{
    external_id_type external_id;
    external_id_type research_external_id;
    account_name_type research_group;
    uint64_t amount;

    optional<tokenization_condition_models> distribution_model;
    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(research_group);
    }
};


}
}

FC_REFLECT(deip::protocol::tokenize_research_operation,
  (external_id)
  (research_group)
  (research_external_id)
  (amount)
  (distribution_model)
  (extensions)
)

FC_REFLECT( deip::protocol::tokenization_agreement_type, (terms) )

DECLARE_STATIC_VARIANT_TYPE(deip::protocol::tokenization_condition_models)
FC_REFLECT_TYPENAME(deip::protocol::tokenization_condition_models)