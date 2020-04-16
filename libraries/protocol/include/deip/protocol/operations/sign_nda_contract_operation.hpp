#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct sign_nda_contract_operation : public base_operation
{
    int64_t contract_id;
    account_name_type contract_signer;
    string signature;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(contract_signer);
    }

};

}
}

FC_REFLECT( deip::protocol::sign_nda_contract_operation,
    (contract_id)
    (contract_signer)
    (signature)
)
