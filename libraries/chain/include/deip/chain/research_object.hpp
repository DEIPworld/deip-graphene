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

using deip::protocol::asset;

class research_object : public object<research_object_type, research_object>
{

    research_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    research_object(Constructor &&c, allocator<Allocator> a)
            : name(a)
            ,  abstract(a)
            ,  permlink(a)
    {
        c(*this);
    }

    research_id_type id;
    id_type research_group_id;
    fc::shared_string name;
    fc::shared_string abstract;
    fc::shared_string permlink;

    time_point_sec created;

    bool is_finished;

    share_type owned_tokens;
    share_type percent_for_review;
};

struct by_permlink;
struct is_finished;

typedef multi_index_container<research_object,
                indexed_by<ordered_unique<tag<by_id>,
                            member<research_object,
                                    research_id_type,
                                    &research_object::id>>,
                        ordered_unique<tag<by_permlink>,
                                composite_key<research_object,
                                        member<research_object,
                                                fc::shared_string,
                                                &research_object::permlink>>,
                                composite_key_compare<fc::strcmp_less>>,
                        ordered_non_unique<tag<is_finished>,
                                member<research_object,
                                        bool,
                                        &research_object::is_finished>>>,

                                    allocator<research_object>>
                                    research_index;
}
}

FC_REFLECT(deip::chain::research_object,
                        (id)(name)(research_group_id)(permlink)(abstract)(created)(is_finished)(owned_tokens)(percent_for_review)
            )

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_object, deip::chain::research_index)