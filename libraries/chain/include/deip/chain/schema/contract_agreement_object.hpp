#pragma once

#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace chain {

using deip::protocol::external_id_type;

enum class acceptance_status: uint8_t
{
    NotAccepted,
    Accepted
};

class contract_agreement_object : public object<contract_agreement_object_type, contract_agreement_object>
{
    contract_agreement_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    contract_agreement_object(Constructor&& c, allocator<Allocator> a)
      : hash(a)
    {
        c(*this);
    }

    contract_agreement_id_type id;
    external_id_type external_id;
    account_name_type creator;

    flat_map<account_name_type, acceptance_status> parties;
    fc::shared_string hash;

    fc::time_point_sec created_at;
    fc::time_point_sec start_time;
    fc::time_point_sec end_time;
};

struct by_external_id;
struct by_creator;
struct by_hash;

typedef multi_index_container<contract_agreement_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<contract_agreement_object,
                                                                contract_agreement_id_type,
                                                               &contract_agreement_object::id>>,
                                         ordered_unique<tag<by_external_id>,
                                                        member<contract_agreement_object,
                                                               external_id_type,
                                                               &contract_agreement_object::external_id>>,
                                         ordered_non_unique<tag<by_creator>,
                                                        member<contract_agreement_object,
                                                                account_name_type,
                                                               &contract_agreement_object::creator>>,
                                         ordered_non_unique<tag<by_hash>,
                                                        member<contract_agreement_object,
                                                                fc::shared_string,
                                                               &contract_agreement_object::hash>>>,
        allocator<contract_agreement_object>>
        contract_agreement_index;

} // namespace chain
} // namespace deip

FC_REFLECT( deip::chain::contract_agreement_object,
             (id)
             (external_id)
             (creator)
             (parties)
             (hash)
             (created_at)
             (start_time)
             (end_time)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::contract_agreement_object, deip::chain::contract_agreement_index)

FC_REFLECT_ENUM(deip::chain::acceptance_status,
  (NotAccepted)
  (Accepted)
)
