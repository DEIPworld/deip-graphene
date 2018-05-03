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

namespace deip{
namespace chain{

using fc::shared_string;

class research_object : public object<research_object_type, research_object>
{ 

    research_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    research_object(Constructor &&c, allocator<Allocator> a) : title(a), abstract(a), permlink(a)
    {
        c(*this);
    }

    research_id_type id;
    research_group_id_type research_group_id;
    shared_string title;
    shared_string abstract;
    shared_string permlink;
    time_point_sec created_at;
    time_point_sec review_share_last_update;
    time_point_sec last_update_time;
    
    bool is_finished;
    share_type owned_tokens;
    uint16_t review_share;
    uint16_t dropout_compensation;
};

struct by_permlink;
struct is_finished;
struct by_research_group;

typedef multi_index_container<research_object,
                indexed_by<ordered_unique<tag<by_id>,
                            member<research_object,
                                    research_id_type,
                                    &research_object::id>>,

                        ordered_unique<tag<by_permlink>,
                                        member<research_object,
                                                shared_string,
                                                &research_object::permlink>>,
                       
                        ordered_non_unique<tag<is_finished>,
                                member<research_object,
                                        bool,
                                        &research_object::is_finished>>,
                        
                        ordered_non_unique<tag<by_research_group>,
                        member<research_object,
                                research_group_id_type,
                                &research_object::research_group_id>>>,

                        allocator<research_object>>
                        research_index;
}
}

FC_REFLECT(deip::chain::research_object,
                        (id)(title)(research_group_id)(permlink)(abstract)(created_at)(review_share_last_update)
                        (last_update_time)(is_finished)(owned_tokens)(review_share)(dropout_compensation)
            )

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_object, deip::chain::research_index)