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

class escrow_object : public object<escrow_object_type, escrow_object>
{
public:
    template <typename Constructor, typename Allocator> escrow_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    escrow_object()
    {
    }

    id_type id;

    uint32_t escrow_id = 20;
    account_name_type from;
    account_name_type to;
    account_name_type agent;
    time_point_sec ratification_deadline;
    time_point_sec escrow_expiration;
    asset deip_balance;
    asset pending_fee;
    bool to_approved = false;
    bool agent_approved = false;
    bool disputed = false;

    bool is_approved() const
    {
        return to_approved && agent_approved;
    }
};

/**
 * @breif a route to send withdrawn vesting shares.
 */
class withdraw_vesting_route_object : public object<withdraw_vesting_route_object_type, withdraw_vesting_route_object>
{
public:
    template <typename Constructor, typename Allocator>
    withdraw_vesting_route_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    withdraw_vesting_route_object()
    {
    }

    id_type id;

    account_id_type from_account;
    account_id_type to_account;
    uint16_t percent = 0;
    bool auto_vest = false;
};

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
struct by_withdraw_route;
struct by_destination;
typedef multi_index_container<withdraw_vesting_route_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<withdraw_vesting_route_object,
                                                               withdraw_vesting_route_id_type,
                                                               &withdraw_vesting_route_object::id>>,
                                         ordered_unique<tag<by_withdraw_route>,
                                                        composite_key<withdraw_vesting_route_object,
                                                                      member<withdraw_vesting_route_object,
                                                                             account_id_type,
                                                                             &withdraw_vesting_route_object::
                                                                                 from_account>,
                                                                      member<withdraw_vesting_route_object,
                                                                             account_id_type,
                                                                             &withdraw_vesting_route_object::
                                                                                 to_account>>,
                                                        composite_key_compare<std::less<account_id_type>,
                                                                              std::less<account_id_type>>>,
                                         ordered_unique<tag<by_destination>,
                                                        composite_key<withdraw_vesting_route_object,
                                                                      member<withdraw_vesting_route_object,
                                                                             account_id_type,
                                                                             &withdraw_vesting_route_object::
                                                                                 to_account>,
                                                                      member<withdraw_vesting_route_object,
                                                                             withdraw_vesting_route_id_type,
                                                                             &withdraw_vesting_route_object::id>>>>,
                              allocator<withdraw_vesting_route_object>>
    withdraw_vesting_route_index;

struct by_from_id;
struct by_to;
struct by_agent;
struct by_ratification_deadline;
typedef multi_index_container<escrow_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<escrow_object, escrow_id_type, &escrow_object::id>>,
                                         ordered_unique<tag<by_from_id>,
                                                        composite_key<escrow_object,
                                                                      member<escrow_object,
                                                                             account_name_type,
                                                                             &escrow_object::from>,
                                                                      member<escrow_object,
                                                                             uint32_t,
                                                                             &escrow_object::escrow_id>>>,
                                         ordered_unique<tag<by_to>,
                                                        composite_key<escrow_object,
                                                                      member<escrow_object,
                                                                             account_name_type,
                                                                             &escrow_object::to>,
                                                                      member<escrow_object,
                                                                             escrow_id_type,
                                                                             &escrow_object::id>>>,
                                         ordered_unique<tag<by_agent>,
                                                        composite_key<escrow_object,
                                                                      member<escrow_object,
                                                                             account_name_type,
                                                                             &escrow_object::agent>,
                                                                      member<escrow_object,
                                                                             escrow_id_type,
                                                                             &escrow_object::id>>>,
                                         ordered_unique<tag<by_ratification_deadline>,
                                                        composite_key<escrow_object,
                                                                      const_mem_fun<escrow_object,
                                                                                    bool,
                                                                                    &escrow_object::is_approved>,
                                                                      member<escrow_object,
                                                                             time_point_sec,
                                                                             &escrow_object::ratification_deadline>,
                                                                      member<escrow_object,
                                                                             escrow_id_type,
                                                                             &escrow_object::id>>,
                                                        composite_key_compare<std::less<bool>,
                                                                              std::less<time_point_sec>,
                                                                              std::less<escrow_id_type>>>>,
                              allocator<escrow_object>>
    escrow_index;


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

FC_REFLECT( deip::chain::withdraw_vesting_route_object,
             (id)(from_account)(to_account)(percent)(auto_vest) )
CHAINBASE_SET_INDEX_TYPE( deip::chain::withdraw_vesting_route_object, deip::chain::withdraw_vesting_route_index )

FC_REFLECT( deip::chain::escrow_object,
             (id)(escrow_id)(from)(to)(agent)
             (ratification_deadline)(escrow_expiration)
             (deip_balance)(pending_fee)
             (to_approved)(agent_approved)(disputed) )
CHAINBASE_SET_INDEX_TYPE( deip::chain::escrow_object, deip::chain::escrow_index )

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
