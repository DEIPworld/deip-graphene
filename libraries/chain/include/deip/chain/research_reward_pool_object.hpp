#pragma once
#include <deip/protocol/authority.hpp>

#include <deip/chain/deip_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

namespace deip{
namespace chain{

class research_reward_pool_object : public object<research_reward_pool_object_type, research_reward_pool_object>
{

    research_reward_pool_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    research_reward_pool_object(Constructor &&c, allocator<Allocator> a)
    {
        c(*this);
    }

    research_reward_pool_id_type id;
    research_id_type research_id;

    share_type reward_amount;
};

struct by_research_id;
struct is_finished;
struct by_research_group;

typedef multi_index_container<research_reward_pool_object,
                indexed_by<ordered_unique<tag<by_id>,
                            member<research_reward_pool_object,
                                    research_reward_pool_id_type,
                                    &research_reward_pool_object::id>>,
                           ordered_unique<tag<by_research_id>,
                            member<research_reward_pool_object,
                                    research_id_type,
                                    &research_reward_pool_object::research_id>>>,
        allocator<research_reward_pool_object>>
        research_reward_pool_index;
}
}

FC_REFLECT(deip::chain::research_reward_pool_object,
                        (id)(research_id)(reward_amount)
            )

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_reward_pool_object, deip::chain::research_reward_pool_index)