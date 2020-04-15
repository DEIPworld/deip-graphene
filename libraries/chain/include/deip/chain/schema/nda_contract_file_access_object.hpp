#pragma once

#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace chain {

enum class nda_contract_file_access_status : uint16_t
{
    unknown = 0,
    pending = 1,
    fulfilled = 2,

    FIRST = pending,
    LAST = fulfilled
};

class nda_contract_file_access_object : public object<nda_contract_file_access_object_type, nda_contract_file_access_object>
{
    nda_contract_file_access_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    nda_contract_file_access_object(Constructor&& c, allocator<Allocator> a) : encrypted_payload_hash(a), encrypted_payload_iv(a), encrypted_payload_encryption_key(a), proof_of_encrypted_payload_encryption_key(a)
    {
        c(*this);
    }

    nda_contract_file_access_id_type id;
    nda_contract_id_type contract_id;
    uint16_t status = static_cast<uint16_t>(nda_contract_file_access_status::pending);

    account_name_type requester;
    fc::shared_string encrypted_payload_hash;
    fc::shared_string encrypted_payload_iv;
    fc::shared_string encrypted_payload_encryption_key;
    fc::shared_string proof_of_encrypted_payload_encryption_key;
};

struct by_contract_id_and_hash;
struct by_contract_id;
struct by_requester;

typedef multi_index_container<
    nda_contract_file_access_object,
    indexed_by<ordered_unique<tag<by_id>,
                              member<nda_contract_file_access_object,
                                     nda_contract_file_access_id_type,
                                     &nda_contract_file_access_object::id>>,
                ordered_non_unique<tag<by_contract_id>,
                                    member<nda_contract_file_access_object,
                                            nda_contract_id_type,
                                            &nda_contract_file_access_object::contract_id>>,
                ordered_non_unique<tag<by_requester>,
                                    member<nda_contract_file_access_object,
                                            account_name_type,
                                            &nda_contract_file_access_object::requester>>,
                ordered_unique<tag<by_contract_id_and_hash>,
                            composite_key<nda_contract_file_access_object,
                                    member<nda_contract_file_access_object,
                                            nda_contract_id_type,
                                            &nda_contract_file_access_object::contract_id>,
                                    member<nda_contract_file_access_object,
                                            fc::shared_string,
                                            &nda_contract_file_access_object::encrypted_payload_hash>>,
                            composite_key_compare<
                                std::less<nda_contract_id_type>,
                                fc::strcmp_less>>>,
    allocator<nda_contract_file_access_object>>
    nda_contract_file_access_index;
}
}

FC_REFLECT_ENUM(deip::chain::nda_contract_file_access_status,
                (unknown)
                (pending)
                (fulfilled)
)

FC_REFLECT(deip::chain::nda_contract_file_access_object,
                (id)
                (contract_id)
                (requester)
                (status)
                (encrypted_payload_hash)
                (encrypted_payload_iv)
                (encrypted_payload_encryption_key)
                (proof_of_encrypted_payload_encryption_key)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::nda_contract_file_access_object, deip::chain::nda_contract_file_access_index)