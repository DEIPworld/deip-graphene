#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

using deip::protocol::external_id_type;

struct create_nda_content_access_request_operation : public entity_operation
{
    external_id_type external_id;
    external_id_type nda_external_id;
    account_name_type requester;
    std::string encrypted_payload_hash;
    std::string encrypted_payload_iv;

    extensions_type extensions;

    string entity_id() const { return "external_id"; }
    external_id_type get_entity_id() const { return external_id; }

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(requester);
    }

};

}
}

FC_REFLECT( deip::protocol::create_nda_content_access_request_operation,
    (external_id)
    (nda_external_id)
    (requester)
    (encrypted_payload_hash)
    (encrypted_payload_iv)
    (extensions)
)
