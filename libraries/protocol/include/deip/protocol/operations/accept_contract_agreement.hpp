#pragma once

#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct accept_contract_agreement_operation: public entity_operation
{
    external_id_type external_id;
    account_name_type party;

    extensions_type extensions;

    string entity_id() const { return "external_id"; }
    external_id_type get_entity_id() const { return external_id; }

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(party);
    }
};

} // namespace protocol
} // namespace deip

FC_REFLECT(deip::protocol::accept_contract_agreement_operation,
  (external_id)
  (party)
  (extensions)
)
