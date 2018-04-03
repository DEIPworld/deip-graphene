#pragma once

#include <deip/protocol/types.hpp>
#include <deip/chain/deip_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <fc/shared_string.hpp>
#include <fc/fixed_string.hpp>


namespace deip {
namespace chain {

class research_group_join_request_object : public object<research_group_join_request_object_type, research_group_join_request_object>
{
public:
    template <typename Constructor, typename Allocator> research_group_join_request_object(Constructor&& c, allocator<Allocator> a) : motivation_letter(a   )
    {
        c(*this);
    }

public:
    research_group_join_request_id_type id;

    account_name_type account_name;
    research_group_id_type research_group_id;
    fc::shared_string motivation_letter;
    fc::time_point_sec expiration_time;
};

struct by_account_name;
struct by_research_group_id;
struct by_expiration_time;
struct by_account_and_research_group_id;

typedef multi_index_container<research_group_join_request_object,
                indexed_by<ordered_unique<tag<by_id>,
                            member<research_group_join_request_object,
                                    research_group_join_request_id_type,
                                   &research_group_join_request_object::id>>,
                           ordered_non_unique<tag<by_account_name>,
                                        member<research_group_join_request_object,
                                               account_name_type,
                                               &research_group_join_request_object::account_name>>,
                           ordered_non_unique<tag<by_research_group_id>,
                                         member<research_group_join_request_object,
                                                research_group_id_type,
                                                &research_group_join_request_object::research_group_id>>,
                           ordered_non_unique<tag<by_expiration_time>,
                                         member<research_group_join_request_object,
                                                fc::time_point_sec,
                                                &research_group_join_request_object::expiration_time>>,
                           ordered_unique<tag<by_account_and_research_group_id>,
                           composite_key<research_group_join_request_object,
                                        member<research_group_join_request_object,
                                                account_name_type,
                                                &research_group_join_request_object::account_name>,
                                        member<research_group_join_request_object,
                                                research_group_id_type,
                                                &research_group_join_request_object::research_group_id>>>>,

                        allocator<research_group_invite_object>>
        research_group_join_request_index;


    } // namespace chain
} // namespace deip



FC_REFLECT(deip::chain::research_group_join_request_object, (id)(account_name)(research_group_id)(motivation_letter)(expiration_time))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_group_join_request_object, deip::chain::research_group_join_request_index)