#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include "deip_object_types.hpp"

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

#include <vector>

namespace deip{
namespace chain{

using protocol::external_id_type;

class review_object : public object<review_object_type, review_object>
{

    review_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    review_object(Constructor &&c, allocator<Allocator> a) 
      : content(a)
    {
        c(*this);
    }

    review_id_type id;

    external_id_type external_id;
    external_id_type research_external_id;
    external_id_type research_content_external_id;

    research_content_id_type research_content_id;
    fc::shared_string content;
    bool is_positive = true;
    account_name_type author;
    time_point_sec created_at;
    flat_set<discipline_id_type> disciplines;
    flat_set<external_id_type> disciplines_external_ids;
    flat_map<discipline_id_type, share_type> expertise_tokens_amount_by_discipline;

    int32_t assessment_model_v;
    flat_map<uint16_t, assessment_criteria_value> assessment_criterias;
};

struct by_external_id;
struct by_research_external_id;
struct by_research_content_external_id;

struct by_author;
struct by_research_content;
struct by_author_and_research_content;

typedef multi_index_container<review_object,
        indexed_by<
        
                ordered_unique<tag<by_id>,
                        member<review_object,
                              review_id_type,
                              &review_object::id>>,

                ordered_unique<tag<by_external_id>,
                        member<review_object,
                              external_id_type,
                              &review_object::external_id>>,
                
                ordered_non_unique<tag<by_research_external_id>,
                        member<review_object,
                                external_id_type,
                                &review_object::research_external_id>>,

                ordered_non_unique<tag<by_research_content_external_id>,
                        member<review_object,
                                external_id_type,
                                &review_object::research_content_external_id>>,
                        
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
           (id)
           (external_id)
           (research_external_id)
           (research_content_external_id)
           (research_content_id)
           (content)
           (is_positive)
           (author)
           (created_at)
           (disciplines)
           (disciplines_external_ids)
           (expertise_tokens_amount_by_discipline)
           (assessment_model_v)
           (assessment_criterias)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::review_object, deip::chain::review_index)