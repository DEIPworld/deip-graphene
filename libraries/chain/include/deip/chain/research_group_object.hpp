#pragma once

#include <deip/protocol/types.hpp>
#include <deip/chain/deip_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <fc/shared_string.hpp>

namespace deip {
namespace chain {

class research_group_object : public object<research_group_object_type, research_group_object>
{

public:
    template <typename Constructor, typename Allocator> research_group_object(Constructor&& c, allocator<Allocator> a)
    : permlink(a), desciption(a)
    {
        c(*this);
    }

public:
    research_group_id_type id;

    fc::shared_string permlink;
    fc::shared_string desciption;

    uint32_t quorum_percent;
    uint32_t total_tokens_amount;
    flat_set<research_group_token_object> research_group_tokens;
};

class research_group_token_object : public object<research_group_token_object_type, research_group_token_object>
{

public:
    template <typename Constructor, typename Allocator> research_group_token_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

public:
    research_group_token_id_type id;
    research_group_id_type research_group;
    uint32_t amount;
    account_name_type owner;
};

struct by_permlink;

typedef multi_index_container<research_group_object,
                                                indexed_by<
                                                    ordered_unique<
                                                        tag<by_id>, 
                                                                member<research_group_object, 
                                                                        research_group_id_type, 
                                                                        &research_group_object::id>
                                                    >
                                                >,
                                                allocator<research_group_object>>
    research_group_index;


struct by_research_group;
struct by_owner;

typedef multi_index_container<research_group_token_object,
                                            indexed_by<
                                                ordered_unique<
                                                            tag<by_id>, 
                                                                member<research_group_token_object, 
                                                                        research_group_token_id_type, 
                                                                        &research_group_token_object::id>
                                                >,
                                                ordered_non_unique<
                                                            tag<by_research_group>, 
                                                            member<
                                                                research_group_token_object, 
                                                                research_group_id_type, 
                                                                &research_group_token_object::research_group>
                                                >,
                                                ordered_unique<
                                                            tag<by_owner>,
                                                                composite_key<research_group_token_object,
                                                                      member<research_group_token_object,
                                                                             account_name_type,
                                                                             &research_group_token_object::owner>,
                                                                      member<research_group_token_object,
                                                                             research_group_id_type,
                                                                             &research_group_token_object::research_group>>
                                                >
                                            >,
                                            allocator<research_group_token_object>>
    research_group_token_index;

} // namespace chain
} // namespace deip

  

FC_REFLECT(deip::chain::research_group_object, (id)(permlink)(desciption)(total_tokens_amount))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_group_object, deip::chain::research_group_index)

FC_REFLECT(deip::chain::research_group_token_object, (id)(research_group)(amount)(owner))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_group_token_object, deip::chain::research_group_token_index)