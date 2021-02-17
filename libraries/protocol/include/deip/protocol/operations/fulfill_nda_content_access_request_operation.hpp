#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

using deip::protocol::external_id_type;

struct fulfill_nda_content_access_request_operation : public base_operation
{
    external_id_type external_id;
    account_name_type grantor;
    string encrypted_payload_encryption_key;
    string proof_of_encrypted_payload_encryption_key;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(grantor);
    }
};

}
}

FC_REFLECT( deip::protocol::fulfill_nda_content_access_request_operation,
    (external_id)
    (grantor)
    (encrypted_payload_encryption_key)
    (proof_of_encrypted_payload_encryption_key)
    (extensions)
)
