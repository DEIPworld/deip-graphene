#pragma once

#include <deip/chain/util/asset.hpp>
#include <deip/chain/deip_objects.hpp>

#include <deip/protocol/asset.hpp>
#include <deip/protocol/config.hpp>
#include <deip/protocol/types.hpp>

#include <fc/reflect/reflect.hpp>

#include <fc/uint128.hpp>

namespace deip {
namespace chain {
namespace util {

using deip::protocol::asset;
using deip::protocol::price;
using deip::protocol::share_type;

using fc::uint128_t;

struct comment_reward_context
{
    share_type rshares;
    uint16_t reward_weight = 0;
    asset max_scr;
    uint128_t total_reward_shares2;
    asset total_reward_fund_deip;
    curve_id reward_curve = quadratic;
};

uint64_t get_rshare_reward(const comment_reward_context& ctx);

uint128_t evaluate_reward_curve(const uint128_t& rshares, const curve_id& curve = quadratic);

// DEIP: decide who will we approach MIN PAYOUT if we don't have stable coin
inline bool is_comment_payout_dust(uint64_t deip_payout)
{
    // DEIP: original logic of steem
    // return to_sbd( p, asset( deip_payout, DEIP_SYMBOL ) ) < DEIP_MIN_PAYOUT_SBD;

    // DEIP: it make sense to move DEIP_MIN_PAYOUT to global properties and let witnesses vote on it
    return asset(deip_payout) < DEIP_MIN_PAYOUT;

    // DEIP: no payout limits
    // return deip_payout <= 0;
}
}
}
} // deip::chain::util

FC_REFLECT(deip::chain::util::comment_reward_context,
    (rshares)(reward_weight)(max_scr)(total_reward_shares2)(total_reward_fund_deip)(reward_curve))
