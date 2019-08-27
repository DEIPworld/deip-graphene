#pragma once

#include "deip_object_types.hpp"

namespace deip {
namespace chain {

enum contract_status : uint16_t
{
    contract_sent = 1,
    contract_signed = 2,
    contract_declined = 3,
    contract_expired = 4
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
    account_name_type signee = account_name_type();

    protocol::public_key_type creator_key;
    protocol::public_key_type signee_key = protocol::public_key_type();

    fc::shared_string contract_hash;
    contract_status status = contract_status::contract_sent;

    fc::time_point_sec created_at;
    fc::time_point_sec start_date;
    fc::time_point_sec end_date;
};

struct by_creator;
struct by_signee;
struct by_contract_hash;
struct by_end_date;

typedef multi_index_container<contract_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<contract_object,
                                                                contract_id_type,
                                                               &contract_object::id>>,
                                         ordered_non_unique<tag<by_creator>,
                                                        member<contract_object,
                                                                account_name_type,
                                                               &contract_object::creator>>,
                                         ordered_non_unique<tag<by_signee>,
                                                        member<contract_object,
                                                                account_name_type,
                                                               &contract_object::signee>>,
                                         ordered_non_unique<tag<by_contract_hash>,
                                                        member<contract_object,
                                                                fc::shared_string,
                                                               &contract_object::contract_hash>>,
                                         ordered_non_unique<tag<by_end_date>,
                                                        member<contract_object,
                                                                fc::time_point_sec,
                                                                &contract_object::end_date>>>,
                              allocator<contract_object>>
    contract_index;

}
}

FC_REFLECT_ENUM(deip::chain::contract_status, (contract_sent)(contract_signed)(contract_declined)(contract_expired))

FC_REFLECT( deip::chain::contract_object,
             (id)(creator)(signee)(creator_key)(signee_key)(contract_hash)(status)(created_at)(start_date)(end_date)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::contract_object, deip::chain::contract_index )

