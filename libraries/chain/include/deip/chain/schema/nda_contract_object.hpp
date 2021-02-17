#pragma once

#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace chain {

using deip::protocol::external_id_type;

class nda_contract_object : public object<nda_contract_object_type, nda_contract_object>
{
    nda_contract_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    nda_contract_object(Constructor&& c, allocator<Allocator> a) 
      : description(a)
    {
        c(*this);
    }

    nda_contract_id_type id;
    external_id_type external_id;
    account_name_type creator;

    flat_set<account_name_type> parties;
    fc::shared_string description;
    external_id_type research_external_id;

    fc::time_point_sec created_at;
    fc::time_point_sec start_time;
    fc::time_point_sec end_time;
};

struct by_external_id;
struct by_creator;
struct by_nda_hash;
struct by_end_date;
struct by_research;

typedef multi_index_container<nda_contract_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<nda_contract_object,
                                                                nda_contract_id_type,
                                                               &nda_contract_object::id>>,
                                         ordered_unique<tag<by_external_id>,
                                                        member<nda_contract_object,
                                                               external_id_type,
                                                               &nda_contract_object::external_id>>,
                                         ordered_non_unique<tag<by_creator>,
                                                        member<nda_contract_object,
                                                                account_name_type,
                                                               &nda_contract_object::creator>>,
                                         ordered_non_unique<tag<by_nda_hash>,
                                                        member<nda_contract_object,
                                                                fc::shared_string,
                                                               &nda_contract_object::description>>,
                                         ordered_non_unique<tag<by_research>,
                                                        member<nda_contract_object,
                                                               external_id_type,
                                                               &nda_contract_object::research_external_id>>,
                                         ordered_non_unique<tag<by_end_date>,
                                                        member<nda_contract_object,
                                                                fc::time_point_sec,
                                                                &nda_contract_object::end_time>>>,
        allocator<nda_contract_object>>
        nda_contract_index;

}
}

FC_REFLECT( deip::chain::nda_contract_object,
             (id)
             (external_id)
             (creator)
             (description)
             (research_external_id)
             (created_at)
             (start_time)
             (end_time)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::nda_contract_object, deip::chain::nda_contract_index)
