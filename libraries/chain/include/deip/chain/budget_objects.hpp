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

class budget_object : public object<budget_object_type, budget_object>
{
    budget_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    budget_object(Constructor&& c, allocator<Allocator> a): content_permlink(a)
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

typedef multi_index_container<budget_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<budget_object,
                                                               budget_id_type,
                                                               &budget_object::id>>,
                                         ordered_non_unique<tag<by_owner_name>,
                                                        member<budget_object,
                                                               account_name_type,
                                                               &budget_object::owner>>>,
                              allocator<budget_object>>
    budget_index;

}
}

FC_REFLECT( deip::chain::budget_object,
             (id)(owner)(target_research)(target_discipline)(research_specific)(created)(balance)(per_block)(start_block)(end_block)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::budget_object, deip::chain::budget_index )

