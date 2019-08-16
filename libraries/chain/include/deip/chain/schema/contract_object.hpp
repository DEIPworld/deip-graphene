#pragma once

#include "deip_object_types.hpp"

namespace deip {
namespace chain {

enum contract_status : uint16_t
{
    contract_pending = 1,
    contract_approved = 2,
    contract_rejected = 3
};

class contract_object : public object<contract_object_type, contract_object>
{
    contract_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    contract_object(Constructor&& c, allocator<Allocator> a) : contract_hash(a)
    {
        c(*this);
    }

    contract_id_type id;

    account_name_type creator;
    account_name_type receiver;

    protocol::public_key_type creator_key;
    protocol::public_key_type receiver_key = protocol::public_key_type();

    fc::shared_string contract_hash;
    contract_status status = contract_status::contract_pending;

    fc::time_point_sec created_at;
};

struct by_creator;
struct by_contract_hash;

typedef multi_index_container<contract_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<contract_object,
                                                                contract_id_type,
                                                               &contract_object::id>>,
                                         ordered_non_unique<tag<by_creator>,
                                                        member<contract_object,
                                                                account_name_type,
                                                               &contract_object::creator>>,
                                         ordered_unique<tag<by_contract_hash>,
                                                        member<contract_object,
                                                                fc::shared_string,
                                                               &contract_object::contract_hash>>>,
                              allocator<contract_object>>
    contract_index;

}
}

FC_REFLECT_ENUM(deip::chain::contract_status, (contract_pending)(contract_approved)(contract_rejected) )

FC_REFLECT( deip::chain::contract_object,
             (id)(creator)(receiver)(creator_key)(receiver_key)(contract_hash)(status)(created_at)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::contract_object, deip::chain::contract_index )

