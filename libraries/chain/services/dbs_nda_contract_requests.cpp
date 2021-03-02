#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_nda_contract_requests.hpp>
#include <deip/chain/schema/nda_contract_file_access_object.hpp>

namespace deip {
namespace chain {

dbs_nda_contract_requests::dbs_nda_contract_requests(database& db)
    : _base_type(db)
{
}

const nda_contract_file_access_object& dbs_nda_contract_requests::get_content_access_request(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<nda_contract_content_access_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    FC_ASSERT(itr != idx.end(), "Access request ${1} does not exist", ("1", external_id));
    return *itr;
}

const bool dbs_nda_contract_requests::content_access_request_exists(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<nda_contract_content_access_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    return itr != idx.end();
}

const dbs_nda_contract_requests::nda_content_access_optional_ref_type dbs_nda_contract_requests::get_content_access_request_if_exists(const external_id_type& external_id) const
{
    nda_content_access_optional_ref_type result;

    const auto& idx = db_impl()
      .get_index<nda_contract_content_access_index>()
      .indicies()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}


const dbs_nda_contract_requests::nda_content_access_request_refs_type dbs_nda_contract_requests::get_content_access_requests_by_nda(const external_id_type& nda_external_id) const
{
    nda_content_access_request_refs_type ret;

    auto it_pair = db_impl()
      .get_index<nda_contract_content_access_index>()
      .indicies()
      .get<by_nda>()
      .equal_range(nda_external_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const dbs_nda_contract_requests::nda_content_access_request_refs_type dbs_nda_contract_requests::get_content_access_requests_by_requester(const account_name_type& requester) const
{
    nda_content_access_request_refs_type ret;

    auto it_pair = db_impl()
      .get_index<nda_contract_content_access_index>()
      .indicies()
      .get<by_requester>()
      .equal_range(requester);
      
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


const nda_contract_file_access_object& dbs_nda_contract_requests::create_content_access_request(const external_id_type& external_id,
                                                                                                const external_id_type& nda_external_id,
                                                                                                const account_name_type& requester,
                                                                                                const fc::string& encrypted_payload_hash,
                                                                                                const fc::string& encrypted_payload_iv)
{
    const auto& request = db_impl().create<nda_contract_file_access_object>([&](nda_contract_file_access_object& cfa_o) {
        cfa_o.external_id = external_id;
        cfa_o.nda_external_id = nda_external_id;
        cfa_o.requester = requester;
        fc::from_string(cfa_o.encrypted_payload_hash, encrypted_payload_hash);
        fc::from_string(cfa_o.encrypted_payload_iv, encrypted_payload_iv);
        fc::from_string(cfa_o.encrypted_payload_encryption_key, "");
        fc::from_string(cfa_o.proof_of_encrypted_payload_encryption_key, "");
        cfa_o.status = static_cast<uint16_t>(nda_contract_file_access_status::pending);
    });

    return request;
}

void dbs_nda_contract_requests::fulfill_content_access_request(const nda_contract_file_access_object& request,
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