#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include "deip_object_types.hpp"
#include "shared_authority.hpp"

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

#include <vector>

namespace deip{
namespace chain{

class review_object : public object<review_object_type, review_object>
{

    review_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    review_object(Constructor &&c, allocator<Allocator> a) : content(a)
            , disciplines(a)
            , references(a)
            , expertise_tokens_amount_by_discipline(a)
    {
        c(*this);
    }

    review_id_type id;
    research_content_id_type research_content_id;
    fc::shared_string content;
    bool is_positive = true;
    account_name_type author;
    time_point_sec created_at;
    discipline_id_type_set disciplines;
    research_content_id_type_set references;

    discipline_id_share_type_map expertise_tokens_amount_by_discipline;
};

struct by_author;
struct by_research_content;
struct by_author_and_research_content;

typedef multi_index_container<review_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<review_object,
                        review_id_type,
                        &review_object::id>>,
                ordered_unique<tag<by_author_and_research_content>,
                        composite_key<review_object,
                                member<review_object,
                                        account_name_type,
                                        &review_object::author>,
                                member<review_object,
                                        research_content_id_type,
                                        &review_object::research_content_id>>>,
                ordered_non_unique<tag<by_author>,
                        member<review_object,
                                account_name_type,
                                &review_object::author>>,
                ordered_non_unique<tag<by_research_content>,
                        member<review_object,
                                research_content_id_type,
                                &review_object::research_content_id>>>,
        allocator<review_object>>
        review_index;
}
}

FC_REFLECT(deip::chain::review_object,
           (id)(research_content_id)(content)(is_positive)(author)(created_at)
           (disciplines)(references)(expertise_tokens_amount_by_discipline)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::review_object, deip::chain::review_index)