#pragma once

#include <fc/fixed_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include "deip_object_types.hpp"
#include "shared_authority.hpp"
#include "research_content_object.hpp"

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
    research_content_type content_type;

    share_type total_weight;
};

struct by_discipline_id;
struct by_research_id;
struct by_research_content_id;
struct by_research_and_discipline;
struct by_content_and_discipline;
struct by_content_type;
struct by_discipline_and_content_type;

typedef multi_index_container<total_votes_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<total_votes_object,
                        total_votes_id_type,
                        &total_votes_object::id>>,
                ordered_non_unique<tag<by_research_id>,
                        member<total_votes_object,
                                research_id_type,
                                &total_votes_object::research_id>>,
                ordered_non_unique<tag<by_discipline_id>,
                        member<total_votes_object,
                                discipline_id_type,
                                &total_votes_object::discipline_id>>,
                ordered_non_unique<tag<by_content_type>,
                        member<total_votes_object,
                                research_content_type,
                                &total_votes_object::content_type>>,
                ordered_non_unique<tag<by_research_and_discipline>,
                        composite_key<total_votes_object,
                                member<total_votes_object,
                                       research_id_type,
                                       &total_votes_object::research_id>,
                                member<total_votes_object,
                                       discipline_id_type,
                                       &total_votes_object::discipline_id>>>,
                ordered_non_unique<tag<by_discipline_and_content_type>,
                        composite_key<total_votes_object,
                                member<total_votes_object,
                                        discipline_id_type,
                                        &total_votes_object::discipline_id>,
                                member<total_votes_object,
                                        research_content_type,
                                        &total_votes_object::content_type>>>,
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

FC_REFLECT( deip::chain::total_votes_object,(id)(discipline_id)(research_id)(research_content_id)(total_weight)(content_type))

CHAINBASE_SET_INDEX_TYPE( deip::chain::total_votes_object, deip::chain::total_votes_index )

