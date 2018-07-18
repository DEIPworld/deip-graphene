#pragma once
#include <deip/protocol/authority.hpp>

#include <deip/chain/deip_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

namespace deip{
namespace chain{

class research_content_reward_pool_object : public object<research_content_reward_pool_object_type, research_content_reward_pool_object>
{

    research_content_reward_pool_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    research_content_reward_pool_object(Constructor &&c, allocator<Allocator> a)
    {
        c(*this);
    }

    research_content_reward_pool_id_type id;
    research_content_id_type research_content_id;
    discipline_id_type discipline_id;

    share_type reward_share;
    share_type expertise_share;
};

struct by_content_and_discipline;

typedef multi_index_container<research_content_reward_pool_object,
                indexed_by<ordered_unique<tag<by_id>,
                            member<research_content_reward_pool_object,
                                    research_content_reward_pool_id_type,
                                    &research_content_reward_pool_object::id>>,
                           ordered_unique<tag<by_content_and_discipline>,
                           composite_key<research_content_reward_pool_object,
                                  member<research_content_reward_pool_object,
                                   research_content_id_type,
                                    &research_content_reward_pool_object::research_content_id>,
                                  member<research_content_reward_pool_object,
                                          discipline_id_type,
                                    &research_content_reward_pool_object::discipline_id>>>>,
        allocator<research_content_reward_pool_object>>
        research_content_reward_pool_index;
}
}

FC_REFLECT(deip::chain::research_content_reward_pool_object,
                        (id)(research_content_id)(discipline_id)(reward_share)(expertise_share)
            )

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_content_reward_pool_object, deip::chain::research_content_reward_pool_index)