#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_nda_contract_requests.hpp>
#include <deip/chain/schema/nda_contract_file_access_object.hpp>

namespace deip {
namespace chain {

dbs_nda_contract_requests::dbs_nda_contract_requests(database& db)
    : _base_type(db)
{
}

const nda_contract_file_access_object&
dbs_nda_contract_requests::get(const nda_contract_file_access_id_type& request_id)
{
    const auto& request = db_impl().get<nda_contract_file_access_object>(request_id);
    return request;
}

const bool dbs_nda_contract_requests::request_exists(const nda_contract_file_access_id_type& request_id) const
{
    const auto& idx = db_impl()
            .get_index<nda_contract_file_access_index>()
            .indices()
            .get<by_id>();

    auto itr = idx.find(request_id);
    return itr != idx.end();
}

const dbs_nda_contract_requests::nda_contract_optional_ref_type dbs_nda_contract_requests::get_request_if_exists(const nda_contract_file_access_id_type& id) const
{
    nda_contract_optional_ref_type result;

    const auto& idx = db_impl()
      .get_index<nda_contract_file_access_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const nda_contract_file_access_object&
dbs_nda_contract_requests::get_by_contract_id_and_hash(const nda_contract_id_type& contract_id, const fc::string& encrypted_payload_hash)
{

    const auto& idx = db_impl().get_index<nda_contract_file_access_index>().indices().get<by_contract_id_and_hash>();
    auto itr = idx.find(boost::make_tuple(contract_id, encrypted_payload_hash));
    FC_ASSERT(itr != idx.end(), "File access object with ${hash} hash for contract ${contract} is not found", ("hash", encrypted_payload_hash)("contract", contract_id));
    return *itr;
}

const dbs_nda_contract_requests::nda_contract_optional_ref_type
dbs_nda_contract_requests::get_request_by_contract_id_and_hash_if_exists(const nda_contract_id_type& contract_id,
                                                                         const fc::string& encrypted_payload_hash) const
{
    nda_contract_optional_ref_type result;

    const auto& idx = db_impl()
            .get_index<nda_contract_file_access_index>()
            .indicies()
            .get<by_contract_id_and_hash>();

    auto itr = idx.find(std::make_tuple(contract_id, encrypted_payload_hash));
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

dbs_nda_contract_requests::nda_contracts_refs_type
dbs_nda_contract_requests::get_by_contract_id(const nda_contract_id_type& contract_id) const
{
    nda_contracts_refs_type ret;

    auto it_pair = db_impl().get_index<nda_contract_file_access_index>().indicies().get<by_contract_id>().equal_range(contract_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_nda_contract_requests::nda_contracts_refs_type
dbs_nda_contract_requests::get_by_requester(const account_name_type& requester) const
{
    nda_contracts_refs_type ret;

    auto it_pair = db_impl().get_index<nda_contract_file_access_index>().indicies().get<by_requester>().equal_range(requester);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const nda_contract_file_access_object&
dbs_nda_contract_requests::create_file_access_request(const nda_contract_id_type& contract_id,
                                                      const account_name_type& requester,
                                                      const fc::string& encrypted_payload_hash,
                                                      const fc::string& encrypted_payload_iv)
{
    const auto& request = db_impl().create<nda_contract_file_access_object>([&](nda_contract_file_access_object& cfa_o) {
        cfa_o.contract_id = contract_id;
        cfa_o.requester = requester;
        fc::from_string(cfa_o.encrypted_payload_hash, encrypted_payload_hash);
        fc::from_string(cfa_o.encrypted_payload_iv, encrypted_payload_iv);
        fc::from_string(cfa_o.encrypted_payload_encryption_key, "");
        fc::from_string(cfa_o.proof_of_encrypted_payload_encryption_key, "");
        cfa_o.status = static_cast<uint16_t>(nda_contract_file_access_status::pending);
    });

    return request;
}

void dbs_nda_contract_requests::fulfill_file_access_request(const nda_contract_file_access_object& request,
                                                            const fc::string& encrypted_payload_encryption_key,
                                                            const fc::string& proof_of_encrypted_payload_encryption_key)
{
    db_impl().modify(request, [&](nda_contract_file_access_object& cfa_o) {
        fc::from_string(cfa_o.encrypted_payload_encryption_key, encrypted_payload_encryption_key);
        fc::from_string(cfa_o.proof_of_encrypted_payload_encryption_key, proof_of_encrypted_payload_encryption_key);
        cfa_o.status = static_cast<uint16_t>(nda_contract_file_access_status::fulfilled);
    });
}

} //namespace chain
} //namespace deip