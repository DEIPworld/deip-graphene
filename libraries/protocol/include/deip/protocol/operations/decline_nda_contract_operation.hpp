#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

// DEPRECATED
struct decline_nda_contract_operation : public base_operation
{
    int64_t contract_id;
    account_name_type decliner;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(decliner);
    }

};

}
}

FC_REFLECT( deip::protocol::decline_nda_contract_operation,
    (contract_id)
    (decliner)
    (extensions)
)