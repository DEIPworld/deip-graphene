#pragma once

#include <deip/protocol/types.hpp>
#include <deip/chain/deip_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <fc/shared_string.hpp>
#include <fc/fixed_string.hpp>


namespace deip {
namespace chain {

using fc::shared_string;

class research_group_object : public object<research_group_object_type, research_group_object>
{

public:
    template <typename Constructor, typename Allocator> research_group_object(Constructor&& c, allocator<Allocator> a) 
        : name(a), description(a), permlink(a)
    {
        c(*this);
    }

public:
    research_group_id_type id;

    shared_string name;
    shared_string description;
    shared_string permlink;
    asset balance;
    share_type quorum_percent;
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
    research_group_id_type research_group_id;
    share_type amount;
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
                                                    >,
                                                    ordered_unique<
                                                        tag<by_permlink>, 
                                                                member<research_group_object, 
                                                                        shared_string,
                                                                        &research_group_object::permlink>
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
                                                                &research_group_token_object::research_group_id>
                                                >,
                                                ordered_unique<
                                                            tag<by_owner>,
                                                                composite_key<research_group_token_object,
                                                                      member<research_group_token_object,
                                                                             account_name_type,
                                                                             &research_group_token_object::owner>,
                                                                      member<research_group_token_object,
                                                                             research_group_id_type,
                                                                             &research_group_token_object::research_group_id>>
                                                >
                                            >,
                                            allocator<research_group_token_object>>
    research_group_token_index;

} // namespace chain
} // namespace deip

  

FC_REFLECT(deip::chain::research_group_object, (id)(permlink)(description)(balance)(quorum_percent))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_group_object, deip::chain::research_group_index)

FC_REFLECT(deip::chain::research_group_token_object, (id)(research_group_id)(amount)(owner))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_group_token_object, deip::chain::research_group_token_index)