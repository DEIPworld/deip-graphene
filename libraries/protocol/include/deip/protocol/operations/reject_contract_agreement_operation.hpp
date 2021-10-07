#pragma once

#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct reject_contract_agreement_operation : public base_operation
{
    external_id_type external_id;
    account_name_type party;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(party);
    }
};

} // namespace protocol
} // namespace deip

FC_REFLECT(deip::protocol::reject_contract_agreement_operation,
  (external_id)
  (party)
  (extensions)
)
