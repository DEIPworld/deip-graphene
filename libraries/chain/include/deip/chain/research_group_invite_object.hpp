#pragma once

#include <deip/protocol/types.hpp>
#include <deip/chain/deip_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <fc/shared_string.hpp>
#include <fc/fixed_string.hpp>


namespace deip {
namespace chain {

class research_group_invite_object : public object<research_group_invite_object_type, research_group_invite_object>
{
public:
    template <typename Constructor, typename Allocator> research_group_invite_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

public:
    research_group_invite_id_type id;

    account_name_type account_name;
    research_group_id_type research_group_id;
    share_type research_group_token_amount;
};


struct by_account_and_research_group_id;

typedef multi_index_container<research_group_invite_object,
                indexed_by<ordered_unique<tag<by_id>,
                            member<research_group_invite_object,
                                   research_group_invite_id_type,
                                   &research_group_invite_object::id>>,
                           ordered_unique<tag<by_account_and_research_group_id>,
                           composite_key<research_group_invite_object,
                                        member<research_group_invite_object,
                                                account_name_type,
                                                &research_group_invite_object::account_name>,
                                        member<research_group_invite_object,
                                                research_group_id_type,
                                                &research_group_invite_object::research_group_id>>>>,

                        allocator<research_group_invite_object>>
        research_group_invite_index;


    } // namespace chain
} // namespace deip



FC_REFLECT(deip::chain::research_group_invite_object, (id)(account_name)(research_group_id)(research_group_token_amount))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_group_invite_object, deip::chain::research_group_invite_index)
