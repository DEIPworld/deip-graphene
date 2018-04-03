#pragma once

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/deip_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace deip {
namespace chain {

using deip::protocol::asset;
using deip::protocol::asset_symbol_type;
using deip::protocol::price;

typedef fc::fixed_string_16 reward_fund_name_type;

enum curve_id
{
    quadratic,
    linear,
    square_root,
    power1dot5
};

class reward_fund_object : public object<reward_fund_object_type, reward_fund_object>
{
public:
    template <typename Constructor, typename Allocator> reward_fund_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    reward_fund_object()
    {
    }

    id_type id;

    reward_fund_name_type name;
    asset reward_balance = asset(0, DEIP_SYMBOL);
    fc::uint128_t recent_claims = 0;
    time_point_sec last_update;
    uint16_t percent_curation_rewards = 0;
    uint16_t percent_content_rewards = 0;
    curve_id author_reward_curve;
    curve_id curation_reward_curve;
};

// clang-format off
struct by_name;
typedef multi_index_container<reward_fund_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<reward_fund_object,
                                                               reward_fund_id_type,
                                                               &reward_fund_object::id>>,
                                         ordered_unique<tag<by_name>,
                                                        member<reward_fund_object,
                                                               reward_fund_name_type,
                                                               &reward_fund_object::name>>>,
                              allocator<reward_fund_object>>
    reward_fund_index;

// clang-format on

} // namespace chain
} // namespace deip

#include <deip/chain/account_object.hpp>

// clang-format off

FC_REFLECT_ENUM( deip::chain::curve_id,
                  (quadratic)(linear)(square_root)(power1dot5))

FC_REFLECT( deip::chain::reward_fund_object,
            (id)
            (name)
            (reward_balance)
            (recent_claims)
            (last_update)
            (percent_curation_rewards)
            (percent_content_rewards)
            (author_reward_curve)
            (curation_reward_curve)
         )
CHAINBASE_SET_INDEX_TYPE( deip::chain::reward_fund_object, deip::chain::reward_fund_index )

// clang-format on
