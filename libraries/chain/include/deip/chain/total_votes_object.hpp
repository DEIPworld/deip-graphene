#pragma once

#include <fc/fixed_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/shared_authority.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {

using deip::protocol::asset;


class total_votes_object : public object<total_votes_object_type, total_votes_object>
{
    total_votes_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    total_votes_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    total_votes_id_type id;
    discipline_id_type discipline_id;
    research_id_type research_id;
    research_content_id_type research_content_id;

    share_type total_votes_amount;
    share_type total_active_votes_amount;

    share_type total_research_reward_votes_amount;
    share_type total_active_research_reward_votes_amount;

    share_type total_curators_reward_votes_amount;
    share_type total_active_curators_reward_votes_amount;
};

struct by_discipline_id;
struct by_research_id;
struct by_research_content_id;
struct by_research_and_discipline;
struct by_content_and_discipline;

typedef multi_index_container<total_votes_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<total_votes_object,
                        total_votes_id_type,
                        &total_votes_object::id>>,
                ordered_non_unique<tag<by_research_id>,
                        member<total_votes_object,
                                research_id_type,
                                &total_votes_object::research_id>>,
                ordered_unique<tag<by_research_and_discipline>,
                        composite_key<total_votes_object,
                                member<total_votes_object,
                                       research_id_type,
                                       &total_votes_object::research_id>,
                                member<total_votes_object,
                                       discipline_id_type,
                                       &total_votes_object::discipline_id>>>,
                ordered_unique<tag<by_content_and_discipline>,
                        composite_key<total_votes_object,
                                member<total_votes_object,
                                       research_content_id_type,
                                       &total_votes_object::research_content_id>,
                                member<total_votes_object,
                                       discipline_id_type,
                                       &total_votes_object::discipline_id>>>,
                ordered_non_unique<tag<by_research_content_id>,
                        member<total_votes_object,
                                research_content_id_type,
                                &total_votes_object::research_content_id>>>,
        allocator<total_votes_object>>
        total_votes_index;
}
}

FC_REFLECT( deip::chain::total_votes_object,(id)(discipline_id)(research_id)(research_content_id)(total_votes_amount)
        (total_active_votes_amount)(total_research_reward_votes_amount)(total_active_research_reward_votes_amount)
        (total_curators_reward_votes_amount)(total_active_curators_reward_votes_amount))

CHAINBASE_SET_INDEX_TYPE( deip::chain::total_votes_object, deip::chain::total_votes_index )

