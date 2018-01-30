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

class research_object : public object<research_object_type, research_object>
{ 

    research_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    research_object(Constructor &&c, allocator<Allocator> a)
    {
        c(*this);
    }

    research_id_type id;
    research_group_id_type research_group_id;
    fc::string name;
    fc::string abstract;
    fc::string permlink;
    time_point_sec created_at;

    bool is_finished;
    share_type owned_tokens;
    double review_share_in_percent;
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
                                                fc::string,
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
                        (id)(name)(research_group_id)(permlink)(abstract)(created_at)(is_finished)(owned_tokens)(review_share_in_percent)
            )

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_object, deip::chain::research_index)