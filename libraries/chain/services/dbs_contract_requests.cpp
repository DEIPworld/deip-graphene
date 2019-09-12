#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_contract.hpp>
#include <deip/chain/services/dbs_contract_requests.hpp>
#include <deip/chain/schema/contract_file_access_object.hpp>

namespace deip {
namespace chain {

dbs_contract_requests::dbs_contract_requests(database& db)
    : _base_type(db)
{
}


const contract_file_access_object&
dbs_contract_requests::get_file_access_request(const contract_file_access_id_type& request_id)
{
    const auto& request = db_impl().get<contract_file_access_object>(request_id);
    return request;
}

const contract_file_access_object& 
dbs_contract_requests::create_file_access_request(const contract_id_type& contract_id,
                                                  const account_name_type& requester,
                                                  const fc::string& encrypted_payload_hash,
                                                  const fc::string& encrypted_payload_iv)
{
    const auto& request = db_impl().create<contract_file_access_object>([&](contract_file_access_object& cfa_o) {
        cfa_o.contract_id = contract_id;
        cfa_o.requester = requester;
        fc::from_string(cfa_o.encrypted_payload_hash, encrypted_payload_hash);
        fc::from_string(cfa_o.encrypted_payload_iv, encrypted_payload_iv);
        fc::from_string(cfa_o.encrypted_payload_encryption_key, "");
        fc::from_string(cfa_o.proof_of_encrypted_payload_encryption_key, "");
    });

    return request;
}

void dbs_contract_requests::fulfill_file_access_request(const contract_file_access_object& request,
                                                        const fc::string& encrypted_payload_encryption_key,
                                                        const fc::string& proof_of_encrypted_payload_encryption_key)
{
    db_impl().modify(request, [&](contract_file_access_object& cfa_o) {
        fc::from_string(cfa_o.encrypted_payload_encryption_key, encrypted_payload_encryption_key);
        fc::from_string(cfa_o.proof_of_encrypted_payload_encryption_key, proof_of_encrypted_payload_encryption_key);
    });
}

} //namespace chain
} //namespace deip