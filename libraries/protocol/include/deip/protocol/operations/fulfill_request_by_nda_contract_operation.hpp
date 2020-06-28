#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

struct fulfill_request_by_nda_contract_operation : public base_operation
{
    account_name_type grantor;
    std::string encrypted_payload_encryption_key;
    std::string proof_of_encrypted_payload_encryption_key;
    int64_t request_id;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(grantor);
    }
};

}
}

FC_REFLECT( deip::protocol::fulfill_request_by_nda_contract_operation,
    (grantor)
    (encrypted_payload_encryption_key)
    (proof_of_encrypted_payload_encryption_key)
    (request_id)
    (extensions)
)