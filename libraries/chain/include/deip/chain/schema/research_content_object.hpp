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

namespace deip {
namespace chain {

using chainbase::allocator;
using fc::shared_string;

enum research_content_type : uint16_t
{
    announcement = 1,
    final_result = 2,
    milestone_article = 3,
    milestone_book = 4,
    milestone_chapter = 5,
    milestone_code = 6,
    milestone_conference_paper = 7,
    milestone_cover_page = 8,
    milestone_data = 9,
    milestone_experiment_findings = 10,
    milestone_method = 11,
    milestone_negative_results = 12,
    milestone_patent = 13,
    milestone_poster = 14,
    milestone_preprint = 15,
    milestone_presentation = 16,
    milestone_raw_data = 17,
    milestone_research_proposal = 18,
    milestone_technical_report = 19,
    milestone_thesis = 20
};

enum research_content_activity_state : uint16_t
{
    active = 1,
    pending = 2,
    closed = 3
};

class research_content_object : public object<research_content_object_type, research_content_object>
{
    
    research_content_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    research_content_object(Constructor &&c, allocator<Allocator> a) 
        : title(a), content(a), permlink(a), authors(a), references(a), external_references(a)
    {
        c(*this);
    }

    research_content_id_type id;
    research_id_type research_id;
    
    research_content_type type;

    shared_string title;
    shared_string content;
    shared_string permlink;
    
    account_name_type_set authors;
    time_point_sec created_at;

    research_content_id_type_set references;
    fixed_string_32_type_set external_references;

    uint16_t activity_round;
    research_content_activity_state activity_state;
    time_point_sec activity_window_start;
    time_point_sec activity_window_end;
};

struct by_research_id;
struct by_research_id_and_content_type;
struct by_activity_state;
struct by_activity_window_start;
struct by_activity_window_end;
struct by_permlink;

typedef multi_index_container<research_content_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<research_content_object,
                        research_content_id_type,
                        &research_content_object::id>>,

                ordered_non_unique<tag<by_research_id>,
                        member<research_content_object,
                                research_id_type,
                                &research_content_object::research_id>>,

                ordered_non_unique<tag<by_research_id_and_content_type>,
                        composite_key<research_content_object,
                                member<research_content_object,
                                        research_id_type,
                                        &research_content_object::research_id>,
                                member<research_content_object,
                                        research_content_type,
                                        &research_content_object::type>>>,

                ordered_unique<tag<by_permlink>,
                        composite_key<research_content_object,
                                member<research_content_object,
                                        research_id_type,
                                        &research_content_object::research_id>,
                                member<research_content_object,
                                        shared_string,
                                        &research_content_object::permlink>>,
                        composite_key_compare<std::less<research_id_type>,
                                              fc::strcmp_less>>,

                ordered_non_unique<tag<by_activity_window_start>,
                                member<research_content_object,
                                        time_point_sec,
                                        &research_content_object::activity_window_start>>,

                ordered_non_unique<tag<by_activity_window_end>,
                                member<research_content_object,
                                        time_point_sec,
                                        &research_content_object::activity_window_end>>,

                ordered_non_unique<tag<by_activity_state>,
                                member<research_content_object,
                                        research_content_activity_state,
                                        &research_content_object::activity_state>>>,
        allocator<research_content_object>>
        research_content_index;
}
}

FC_REFLECT_ENUM(deip::chain::research_content_type, (announcement)(milestone)(final_result))
FC_REFLECT_ENUM(deip::chain::research_content_activity_state, (active)(pending)(closed) )
FC_REFLECT(deip::chain::research_content_object, (id)(research_id)(type)(title)(content)(permlink)(authors)(created_at)(references)(external_references)(activity_round)(activity_state)(activity_window_start)(activity_window_end))
CHAINBASE_SET_INDEX_TYPE(deip::chain::research_content_object, deip::chain::research_content_index)