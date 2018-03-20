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

class review_object : public object<review_object_type, review_object>
{

    review_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    review_object(Constructor &&c, allocator<Allocator> a)
    {
        c(*this);
    }

    review_id_type id;
    research_id_type research_id;
    fc::string content;
    bool is_positive = true;
    account_name_type author;
    time_point_sec created_at;
    std::map<discipline_id_type, share_type> reward_weights_per_discipline;
};

struct by_author;
struct by_research;

typedef multi_index_container<review_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<review_object,
                        review_id_type,
                        &review_object::id>>,
                ordered_non_unique<tag<by_author>,
                        member<review_object,
                                account_name_type,
                                &review_object::author>>,
                ordered_non_unique<tag<by_research>,
                        member<review_object,
                                research_id_type,
                                &review_object::research_id>>>,

        allocator<review_object>>
        review_index;
}
}

FC_REFLECT(deip::chain::review_object,
           (id)(research_id)(content)(is_positive)(author)(created_at)
)

CHAINBASE_SET_INDEX_TYPE(deip::chain::review_object, deip::chain::review_index)