#pragma once

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include "deip_object_types.hpp"
#include "witness_objects.hpp"
#include "shared_authority.hpp"

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {

using deip::protocol::asset;

class discipline_supply_object : public object<discipline_supply_object_type, discipline_supply_object>
{
    discipline_supply_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    discipline_supply_object(Constructor&& c, allocator<Allocator> a) : content_hash(a), additional_info(a)
    {
        c(*this);
    }

    discipline_supply_id_type id;

    account_name_type grantor;

    discipline_id_type target_discipline;
    
    time_point_sec created = time_point_sec::min();

    asset balance = asset(0, DEIP_SYMBOL);
    share_type per_block = 0;
    fc::time_point_sec start_time = time_point_sec::min();
    fc::time_point_sec end_time = time_point_sec::min();

    bool is_extendable;
    fc::shared_string content_hash;

    shared_string_type_map additional_info;
};

struct by_grantor_name;
struct by_start_time;
struct by_end_time;

typedef multi_index_container<discipline_supply_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<discipline_supply_object,
                                                                discipline_supply_id_type,
                                                               &discipline_supply_object::id>>,
                                         ordered_non_unique<tag<by_start_time>,
                                                        member<discipline_supply_object,
                                                                fc::time_point_sec,
                                                               &discipline_supply_object::start_time>>,
                                         ordered_non_unique<tag<by_end_time>,
                                                        member<discipline_supply_object,
                                                                fc::time_point_sec,
                                                               &discipline_supply_object::end_time>>,
                                         ordered_non_unique<tag<by_grantor_name>,
                                                        member<discipline_supply_object,
                                                               account_name_type,
                                                               &discipline_supply_object::grantor>>>,
                              allocator<discipline_supply_object>>
    discipline_supply_index;

}
}

FC_REFLECT( deip::chain::discipline_supply_object,
             (id)(grantor)(target_discipline)(created)(balance)(per_block)(start_time)(end_time)(is_extendable)(content_hash)(additional_info)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::discipline_supply_object, deip::chain::discipline_supply_index )

