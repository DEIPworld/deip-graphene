#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct basic_tokenization_type
{
};

typedef fc::static_variant<
  basic_tokenization_type
  >
  tokenization_options;


struct create_security_token_operation : public entity_operation
{
    external_id_type external_id;
    external_id_type research_external_id;
    account_name_type research_group;
    uint32_t amount;

    tokenization_options options;
    extensions_type extensions;

    void validate() const;

    string entity_id() const { return "external_id"; }
    external_id_type get_entity_id() const { return external_id; }

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(research_group);
    }
};


}
}

FC_REFLECT(deip::protocol::create_security_token_operation,
  (external_id)
  (research_external_id)
  (research_group)
  (amount)
  (options)
  (extensions)
)

FC_REFLECT(deip::protocol::basic_tokenization_type, )

DECLARE_STATIC_VARIANT_TYPE(deip::protocol::tokenization_options)
FC_REFLECT_TYPENAME(deip::protocol::tokenization_options)