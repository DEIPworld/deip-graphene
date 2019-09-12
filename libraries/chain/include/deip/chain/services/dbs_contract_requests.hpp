#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/contract_object.hpp>
#include <deip/chain/schema/contract_file_access_object.hpp>

namespace deip {
namespace chain {

class dbs_contract_requests : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_contract_requests() = delete;

protected:
    explicit dbs_contract_requests(database& db);

public:
    using contracts_refs_type = std::vector<std::reference_wrapper<const contract_file_access_object>>;

    const contract_file_access_object& get_file_access_request(const contract_file_access_id_type& request_id);

    const contract_file_access_object& create_file_access_request(const contract_id_type& contract_id,
                                                                  const account_name_type& requester,
                                                                  const std::string& encrypted_payload_hash,
                                                                  const std::string& encrypted_payload_iv);

    void fulfill_file_access_request(const contract_file_access_object& request,
                                     const std::string& encrypted_payload_encryption_key,
                                     const std::string& proof_of_encrypted_payload_encryption_key);
};
} // namespace chain
} // namespace deip