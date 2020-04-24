#pragma once
#include <deip/protocol/base.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct delete_proposal_operation : public base_operation
{
    external_id_type external_id;
    account_name_type account;
    uint16_t authority_type;
    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(account);
    }
};

}
}


FC_REFLECT( deip::protocol::delete_proposal_operation,
  (external_id)
  (account)
  (authority_type)
  (extensions) 
)
