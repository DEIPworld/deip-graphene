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


uint128_t evaluate_reward_curve(const uint128_t& rshares, const curve_id& curve = quadratic);

// DEIP: decide who will we approach MIN PAYOUT if we don't have stable coin

}
}
} // deip::chain::util

