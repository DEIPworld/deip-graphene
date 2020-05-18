#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

struct create_request_by_nda_contract_operation : public base_operation
{
    account_name_type requester;
    std::string encrypted_payload_hash;
    std::string encrypted_payload_iv;

    int64_t contract_id;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(requester);
    }

};

}
}

FC_REFLECT( deip::protocol::create_request_by_nda_contract_operation,
    (requester)
    (encrypted_payload_hash)
    (encrypted_payload_iv)
    (contract_id)
)
