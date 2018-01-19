#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/shared_authority.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

#include <vector>

namespace deip {
namespace chain {

enum research_content_type
{
    announcement,
    milestone,
    comment,
    final_result,
    review
};

class research_content_object : public object<research_content_object_type, research_content_object>
{

    research_content_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    research_content_object(Constructor &&c, allocator<Allocator> a) : name(a)
    {
        c(*this);
    }

    research_content_id_type id;
    research_id_type research_id;
    research_content_type content_type;

};

struct by_research_id;
struct by_research_id_and_content_type;

typedef multi_index_container<research_content_object,
                indexed_by<ordered_unique<tag<by_id>,
                            member<research_content_object,
                                    research_content_id_type,
                                    &research_content_object::id>>,
                ordered_non_unique<tag<by_research_id>,
                    member<research_content_object,
                           research_id_type,
                           &research_content_object::research_id>>>,
                ordered_non_unique<tag<by_research_id_and_content_type>,
                        composite_key<research_content_object,
                                member<research_content_object,
                                        research_id_type,
                                        &research_content_object::research_id>,
                                member<research_content_object,
                                        research_content_type,
                                        &research_content_object::content_type>>>>,
                        allocator<research_content_object>>
                        research_content_index;
}
}

FC_REFLECT(deip::chain::research_content_object,
                        (id)(research_id)(content_type)
            )

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_content_object, deip::chain::research_content_index)