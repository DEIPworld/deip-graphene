#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/shared_authority.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip{
namespace chain{

using deip::protocol::asset;

class research_object : public object<research_object_type, research_object>
{

    research_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    research_object(Constructor &&c, allocator<Allocator> a)
    {
        c(*this);
    }

    id_type id;
    id_type discipline_ids;
    vector <id_type> disciplines_ids;
    fc::shared_string name;
    fc::shared_string abstract;

    time_point_sec created;

    bool is_finished;
    string permlink;
    share_type owned_tokens;
};

struct by_permlink;
struct by_discipline_id;

typedef multi_index_container<research_object,
                indexed_by<ordered_unique<tag<by_id>,
                            member<research_object,
                                    research_id,
                                    &research_object::id>>
                            ordered_unique<tag<by_permlink>
                            member<research_object,
                                    string,
                                    &research_object::permlink>>
                            ordered_unique<tag<by_discipline_id>,
                            member<research_object,
                                    discipline_id,
                                    &research_object::discipline_ids>>>,
                                    
                                    allocator<research_object>>
                                    research_index;
}
}

FC_REFLECT(deip::chain::research_object,
                        (id)(permlink)(discipline_ids)
            )

CHAINBASE_SET_INDEX_TYPE(deip::chain::reearch_object, deip::chain::research_index)