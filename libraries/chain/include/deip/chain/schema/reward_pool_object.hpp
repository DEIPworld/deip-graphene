#pragma once
#include <deip/protocol/asset.hpp>

#include "deip_object_types.hpp"
#include "shared_authority.hpp"

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

namespace deip{
namespace chain{

using deip::protocol::asset;

class reward_pool_object : public object<reward_pool_object_type, reward_pool_object>
{

    reward_pool_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    reward_pool_object(Constructor &&c, allocator<Allocator> a)
    {
        c(*this);
    }

    reward_pool_id_type id;
    research_content_id_type research_content_id;
    discipline_id_type discipline_id;

    asset balance = asset(0, DEIP_SYMBOL);
    share_type expertise;
};

struct by_research_content;
struct by_content_and_discipline;

typedef multi_index_container<reward_pool_object,
                indexed_by<ordered_unique<tag<by_id>,
                            member<reward_pool_object,
                                    reward_pool_id_type,
                                    &reward_pool_object::id>>,
                           ordered_non_unique<tag<by_research_content>,
                            member<reward_pool_object,
                                    research_content_id_type,
                                    &reward_pool_object::research_content_id>>,
                           ordered_unique<tag<by_content_and_discipline>,
                           composite_key<reward_pool_object,
                                  member<reward_pool_object,
                                   research_content_id_type,
                                    &reward_pool_object::research_content_id>,
                                  member<reward_pool_object,
                                          discipline_id_type,
                                    &reward_pool_object::discipline_id>>>>,
        allocator<reward_pool_object>>
        reward_pool_index;
}
}

FC_REFLECT(deip::chain::reward_pool_object,
                        (id)(research_content_id)(discipline_id)(balance)(expertise)
            )

CHAINBASE_SET_INDEX_TYPE(deip::chain::reward_pool_object, deip::chain::reward_pool_index)