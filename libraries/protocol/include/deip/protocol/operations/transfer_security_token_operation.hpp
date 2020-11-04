#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct transfer_security_token_operation : public base_operation
{
    account_name_type from;
    account_name_type to;
    external_id_type security_token_external_id;
    uint32_t amount;
    string memo;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(from);
    }
};

}
}


FC_REFLECT(deip::protocol::transfer_security_token_operation,
  (from)
  (to)
  (security_token_external_id)
  (amount)
  (memo)
  (extensions)
)