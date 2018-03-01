#pragma once

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/witness_objects.hpp>
#include <deip/chain/shared_authority.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {

using deip::protocol::asset;

class grant_object : public object<grant_object_type, grant_object>
{
    grant_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    grant_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    id_type id;

    account_name_type owner;

    discipline_id_type target_discipline;
    
    time_point_sec created = time_point_sec::min();

    asset balance = asset(0, DEIP_SYMBOL);
    share_type per_block = 0;
    uint32_t start_block = 0;
    uint32_t end_block = 0;
};

struct by_owner_name;

typedef multi_index_container<grant_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<grant_object,
                                                               grant_id_type,
                                                               &grant_object::id>>,
                                         ordered_non_unique<tag<by_owner_name>,
                                                        member<grant_object,
                                                               account_name_type,
                                                               &grant_object::owner>>>,
                              allocator<grant_object>>
    grant_index;

}
}

FC_REFLECT( deip::chain::grant_object,
             (id)(owner)(target_discipline)(created)(balance)(per_block)(start_block)(end_block)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::grant_object, deip::chain::grant_index )

