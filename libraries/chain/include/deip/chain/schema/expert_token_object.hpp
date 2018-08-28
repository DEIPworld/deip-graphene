#pragma once

#include <fc/fixed_string.hpp>

#include "../../../../../protocol/include/deip/protocol/authority.hpp"
#include "../../../../../protocol/include/deip/protocol/deip_operations.hpp"

#include "deip_object_types.hpp"
#include "shared_authority.hpp"

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {

using deip::protocol::asset;

class expert_token_object : public object<expert_token_object_type, expert_token_object>
{
    expert_token_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    expert_token_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    expert_token_id_type id;
    account_name_type account_name;
    discipline_id_type discipline_id;
    share_type amount = 0;
    uint16_t voting_power = DEIP_100_PERCENT; ///< current voting power of this token, it falls after every vote
    time_point_sec last_vote_time; ///< used to increase the voting power of this token the longer it goes without voting.
};

struct by_account_name;
struct by_discipline_id;
struct by_account_and_discipline;

typedef multi_index_container<expert_token_object,
            indexed_by<ordered_unique<tag<by_id>,
                    member<expert_token_object,
                           expert_token_id_type,
                           &expert_token_object::id>>,
            ordered_non_unique<tag<by_account_name>,
                    member<expert_token_object,
                           account_name_type,
                           &expert_token_object::account_name>>,
            ordered_unique<tag<by_account_and_discipline>,
                    composite_key<expert_token_object,
                            member<expert_token_object,
                                   account_name_type,
                                   &expert_token_object::account_name>,
                            member<expert_token_object,
                                   discipline_id_type,
                                   &expert_token_object::discipline_id>>>,
            ordered_non_unique<tag<by_discipline_id>,
                    member<expert_token_object,
                           discipline_id_type,
                           &expert_token_object::discipline_id>>>,
        allocator<expert_token_object>>
        expert_token_index;
    }
}

FC_REFLECT( deip::chain::expert_token_object,
            (id)(account_name)(discipline_id)(amount)(voting_power)(last_vote_time)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::expert_token_object, deip::chain::expert_token_index )