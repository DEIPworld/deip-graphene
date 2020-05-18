#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>

#include <deip/protocol/authority.hpp>

#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>

#include <numeric>

#include <vector>

namespace deip{
namespace chain{

class research_discipline_relation_object
        : public object<research_discipline_relation_object_type, research_discipline_relation_object>
{

    research_discipline_relation_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    research_discipline_relation_object(Constructor &&c, allocator<Allocator> a)
    {
        c(*this);
    }

    research_discipline_relation_id_type id;
    research_id_type research_id;
    discipline_id_type discipline_id;
    uint16_t votes_count;
    share_type research_eci = 0;
};

struct by_research_id;
struct by_discipline_id;
struct by_research_eci;
struct by_research_and_discipline;

typedef multi_index_container<research_discipline_relation_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<research_discipline_relation_object,
                        research_discipline_relation_id_type,
                        &research_discipline_relation_object::id>>,
                ordered_non_unique<tag<by_research_id>,
                        member<research_discipline_relation_object,
                                research_id_type,
                                &research_discipline_relation_object::research_id>>,
                ordered_non_unique<tag<by_discipline_id>,
                        member<research_discipline_relation_object,
                                discipline_id_type,
                                &research_discipline_relation_object::discipline_id>>,
                ordered_non_unique<tag<by_research_eci>,
                        member<research_discipline_relation_object,
                                share_type,
                                &research_discipline_relation_object::research_eci>>,
                ordered_unique<tag<by_research_and_discipline>,
                        composite_key<research_discipline_relation_object,
                                member<research_discipline_relation_object,
                                        research_id_type,
                                                &research_discipline_relation_object::research_id>,
                                member<research_discipline_relation_object,
                                        discipline_id_type,
                                        &research_discipline_relation_object::discipline_id>>>>,
        allocator<research_discipline_relation_object>>
        research_discipline_relation_index;
}
}

FC_REFLECT(deip::chain::research_discipline_relation_object,
           (id)(research_id)(discipline_id)(votes_count)(research_eci)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_discipline_relation_object, deip::chain::research_discipline_relation_index)