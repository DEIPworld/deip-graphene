#pragma once

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include "deip_object_types.hpp"

#include <boost/multi_index/composite_key.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace deip {
namespace chain {

using deip::protocol::asset;
using deip::protocol::asset_symbol_type;
using deip::protocol::price;

typedef fc::fixed_string_16 reward_fund_name_type;

/**
 * @breif a route to send withdrawn common tokens.
 */
class withdraw_common_tokens_route_object : public object<withdraw_common_tokens_route_object_type, withdraw_common_tokens_route_object>
{
public:
    template <typename Constructor, typename Allocator>
    withdraw_common_tokens_route_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    withdraw_common_tokens_route_object()
    {
    }

    id_type id;

    account_id_type from_account;
    account_id_type to_account;
    uint16_t percent = 0;
    bool auto_common_token = false;
};

enum curve_id
{
    quadratic,
    linear,
    square_root,
    power1dot5
};

// clang-format off
struct by_withdraw_route;
struct by_destination;
typedef multi_index_container<withdraw_common_tokens_route_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<withdraw_common_tokens_route_object,
                                                               withdraw_common_tokens_route_id_type,
                                                               &withdraw_common_tokens_route_object::id>>,
                                         ordered_unique<tag<by_withdraw_route>,
                                                        composite_key<withdraw_common_tokens_route_object,
                                                                      member<withdraw_common_tokens_route_object,
                                                                             account_id_type,
                                                                             &withdraw_common_tokens_route_object::
                                                                                 from_account>,
                                                                      member<withdraw_common_tokens_route_object,
                                                                             account_id_type,
                                                                             &withdraw_common_tokens_route_object::
                                                                                 to_account>>,
                                                        composite_key_compare<std::less<account_id_type>,
                                                                              std::less<account_id_type>>>,
                                         ordered_unique<tag<by_destination>,
                                                        composite_key<withdraw_common_tokens_route_object,
                                                                      member<withdraw_common_tokens_route_object,
                                                                             account_id_type,
                                                                             &withdraw_common_tokens_route_object::
                                                                                 to_account>,
                                                                      member<withdraw_common_tokens_route_object,
                                                                             withdraw_common_tokens_route_id_type,
                                                                             &withdraw_common_tokens_route_object::id>>>>,
                              allocator<withdraw_common_tokens_route_object>>
    withdraw_common_tokens_route_index;

// clang-format on

} // namespace chain
} // namespace deip

#include "account_object.hpp"

// clang-format off

FC_REFLECT_ENUM( deip::chain::curve_id,
                  (quadratic)(linear)(square_root)(power1dot5))

FC_REFLECT( deip::chain::withdraw_common_tokens_route_object,
             (id)(from_account)(to_account)(percent)(auto_common_token) )
CHAINBASE_SET_INDEX_TYPE( deip::chain::withdraw_common_tokens_route_object, deip::chain::withdraw_common_tokens_route_index )

// clang-format on
