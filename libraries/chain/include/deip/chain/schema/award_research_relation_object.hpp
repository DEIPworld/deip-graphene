#pragma once

#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace chain {

using deip::protocol::asset;

class award_research_relation_object : public object<award_research_relation_object_type, award_research_relation_object>
{
    award_research_relation_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    award_research_relation_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    award_research_relation_id_type id;
    award_id_type award_id;
    research_id_type research_id;
    research_group_id_type research_group_id;
    account_name_type awardee;

    asset total_amount;
    asset total_expenses;

    research_group_id_type university_id;
    share_type university_overhead;
};

struct by_award_and_research;
struct by_award;

typedef multi_index_container<award_research_relation_object,
            indexed_by<ordered_unique<tag<by_id>,
                    member<award_research_relation_object,
                            award_research_relation_id_type,
                           &award_research_relation_object::id>>,
            ordered_unique<tag<by_award_and_research>,
                        composite_key<award_research_relation_object,
                                member<award_research_relation_object,
                                award_id_type,
                                        &award_research_relation_object::award_id>,
                                member<award_research_relation_object,
                                        research_id_type,
                                        &award_research_relation_object::research_id>>>,
            ordered_non_unique<tag<by_award>,
                    member<award_research_relation_object,
                            award_id_type,
                           &award_research_relation_object::award_id>>>,
        allocator<award_research_relation_object>>
        award_research_relation_index;

}
}

FC_REFLECT( deip::chain::award_research_relation_object, (id)
                                                         (award_id)
                                                         (research_id)
                                                         (research_group_id)
                                                         (awardee)
                                                         (total_amount)
                                                         (total_expenses)
                                                         (university_id)
                                                         (university_overhead)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::award_research_relation_object, deip::chain::award_research_relation_index )
