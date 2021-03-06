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

namespace deip {
namespace chain {

using fc::shared_string;
using protocol::external_id_type;
using protocol::asset_symbol_type;
using protocol::percent;

class research_object : public object<research_object_type, research_object>
{ 
    research_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    research_object(Constructor&& c, allocator<Allocator> a)
        : description(a)
    {
        c(*this);
    }

    research_id_type id;
    external_id_type external_id;
    account_id_type research_group_id;
    external_id_type research_group;
    shared_string description;
    time_point_sec created_at;
    time_point_sec review_share_last_update;
    time_point_sec last_update_time;
    
    bool is_finished;

    flat_map<discipline_id_type, share_type> eci_per_discipline;
    flat_set<asset> security_tokens; // TODO: move to a separate 'security_token_terms' object

    uint16_t number_of_positive_reviews = 0;
    uint16_t number_of_negative_reviews = 0;

    uint16_t number_of_research_contents = 0;

    bool is_private;
    bool is_default;
};

struct is_finished;
struct is_default;
struct by_research_group_id;
struct by_research_group;
struct by_external_id;

typedef multi_index_container<
    research_object,
    indexed_by<

        ordered_unique<tag<by_id>, 
          member<research_object, 
                research_id_type, 
                &research_object::id>>,

        ordered_unique<tag<by_external_id>, 
            member<research_object,
                external_id_type, 
                &research_object::external_id>>,

        ordered_non_unique<tag<is_finished>, 
            member<research_object, 
                bool, 
                &research_object::is_finished>>,

        ordered_non_unique<tag<is_default>, 
            member<research_object, 
                bool, 
                &research_object::is_default>>,

        ordered_non_unique<tag<by_research_group>,
            member<research_object, 
                account_name_type, 
                &research_object::research_group>>,

        ordered_non_unique<tag<by_research_group_id>,
            member<research_object, 
                  account_id_type, 
                  &research_object::research_group_id>>>,

    allocator<research_object>>
    research_index;
}
}

FC_REFLECT(deip::chain::research_object,
  (id)
  (external_id)
  (description)
  (research_group_id)
  (research_group)
  (created_at)
  (review_share_last_update)
  (last_update_time)
  (is_finished)
  (eci_per_discipline)
  (security_tokens)
  (number_of_positive_reviews)
  (number_of_negative_reviews)
  (number_of_research_contents)
  (is_private)
  (is_default)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_object, deip::chain::research_index)