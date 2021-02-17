#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/nda_contract_object.hpp>
#include <deip/chain/schema/nda_contract_file_access_object.hpp>

namespace deip {
namespace chain {

class dbs_nda_contract_requests : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_nda_contract_requests() = delete;

protected:
    explicit dbs_nda_contract_requests(database& db);

public:
    using nda_content_access_request_refs_type = std::vector<std::reference_wrapper<const nda_contract_file_access_object>>;
    using nda_content_access_optional_ref_type = fc::optional<std::reference_wrapper<const nda_contract_file_access_object>>;

    const nda_contract_file_access_object& get_content_access_request(const external_id_type& external_id) const;

    const bool content_access_request_exists(const external_id_type& external_id) const;

    const nda_content_access_optional_ref_type get_content_access_request_if_exists(const external_id_type& external_id) const;

    const nda_content_access_request_refs_type get_content_access_requests_by_nda(const external_id_type& nda_external_id) const;

    const nda_content_access_request_refs_type get_content_access_requests_by_requester(const account_name_type& requester) const;

    const nda_contract_file_access_object& create_content_access_request(const external_id_type& external_id,
                                                                         const external_id_type& nda_external_id,
                                                                         const account_name_type& requester,
                                                                         const fc::string& encrypted_payload_hash,
                                                                         const fc::string& encrypted_payload_iv);

    void fulfill_content_access_request(const nda_contract_file_access_object& request,
                                        const fc::string& encrypted_payload_encryption_key,
                                        const fc::string& proof_of_encrypted_payload_encryption_key);
};
} // namespace chain
} // namespace deip