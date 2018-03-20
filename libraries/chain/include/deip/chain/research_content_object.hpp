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

enum research_content_type : uint16_t
{
    announcement = 1,
    milestone = 2,
    final_result = 3
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
    {
        c(*this);
    }

    research_content_id_type id;
    research_id_type research_id;
    research_content_type type;
    fc::string content;
    flat_set<account_name_type> authors;
    time_point_sec created_at;
    std::vector<research_id_type> research_references;
    std::vector<string> research_external_references;

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

FC_REFLECT(deip::chain::research_content_object, (id)(research_id)(type)(content)(authors)(research_references)(research_external_references))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_content_object, deip::chain::research_content_index)