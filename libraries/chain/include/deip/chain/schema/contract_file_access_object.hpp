#pragma once

#include "deip_object_types.hpp"

namespace deip {
namespace chain {

class contract_file_access_object : public object<contract_file_access_object_type, contract_file_access_object>
{
    contract_file_access_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    contract_file_access_object(Constructor&& c, allocator<Allocator> a) : encrypted_payload_hash(a), initialiazation_vector(a), file_encryption_key(a)
    {
        c(*this);
    }

    contract_file_access_id_type id;

    contract_id_type contract_id;

    account_name_type requester;
    fc::shared_string encrypted_payload_hash;
    fc::shared_string initialiazation_vector;

    fc::shared_string file_encryption_key;
};

struct by_request_and_hash;

typedef multi_index_container<contract_file_access_object,
                                            indexed_by<
                                                ordered_unique<tag<by_id>,
                                                                member<contract_file_access_object,
                                                                        contract_file_access_id_type,
                                                                       &contract_file_access_object::id>>,
                                                ordered_unique<tag<by_request_and_hash>,
                                                                composite_key<contract_file_access_object,
                                                                      member<contract_file_access_object,
                                                                              contract_id_type,
                                                                             &contract_file_access_object::contract_id>,
                                                                      member<contract_file_access_object,
                                                                              fc::shared_string,
                                                                             &contract_file_access_object::encrypted_payload_hash>>>>,
                                            allocator<contract_file_access_object>>
    contract_file_access_index;

}
}

FC_REFLECT( deip::chain::contract_file_access_object,
             (id)(contract_id)(requester)(encrypted_payload_hash)(initialiazation_vector)(file_encryption_key))

CHAINBASE_SET_INDEX_TYPE( deip::chain::contract_file_access_object, deip::chain::contract_file_access_index )

