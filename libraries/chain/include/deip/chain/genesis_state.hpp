#pragma once

#include <vector>
#include <string>

#include <deip/protocol/types.hpp>
#include <deip/protocol/asset.hpp>
#include <deip/chain/deip_object_types.hpp>

#include <fc/reflect/reflect.hpp>

namespace deip {
namespace chain {

namespace sp = deip::protocol;
namespace dc = deip::chain;

struct genesis_state_type
{
    struct account_type
    {
        std::string name;
        std::string recovery_account;
        sp::public_key_type public_key;
        sp::share_type deip_amount;
        sp::share_type sp_amount;
    };

    struct witness_type
    {
        std::string owner_name;
        sp::public_key_type block_signing_key;
    };

    struct discipline_type
    {
        dc::discipline_id_type id;
        std::string name;
        sp::share_type votes_in_last_ten_weeks;
        dc::discipline_id_type parent_id;
    };

    struct expert_token_type
    {
        std::string account_name;
        dc::discipline_id_type discipline_id;
        uint32_t amount;
    };

    genesis_state_type()
        : init_supply(0)
    {
    }

    genesis_state_type(const sp::share_type& supply)
        : init_supply(supply)
    {
    }

    sp::share_type init_supply;
    sp::asset init_rewards_supply;
    time_point_sec initial_timestamp;
    std::vector<account_type> accounts;
    std::vector<witness_type> witness_candidates;
    std::vector<discipline_type> disciplines;
    std::vector<expert_token_type> expert_tokens;


    sp::chain_id_type initial_chain_id;
};

namespace utils {

void generate_default_genesis_state(genesis_state_type& genesis);

} // namespace utils
} // namespace chain
} // namespace deip

// clang-format off
FC_REFLECT(deip::chain::genesis_state_type::account_type,
           (name)
           (recovery_account)
           (public_key)
           (deip_amount)
           (sp_amount))

FC_REFLECT(deip::chain::genesis_state_type::witness_type,
           (owner_name)
           (block_signing_key))

FC_REFLECT(deip::chain::genesis_state_type::discipline_type,
           (id)
           (name)
           (votes_in_last_ten_weeks)
           (parent_id))

FC_REFLECT(deip::chain::genesis_state_type::expert_token_type,
           (account_name)
           (discipline_id)
           (amount))

FC_REFLECT(deip::chain::genesis_state_type,
           (init_supply)
           (init_rewards_supply)
           (initial_timestamp)
           (accounts)
           (witness_candidates)
           (disciplines)
           (expert_tokens)
           (initial_chain_id))
// clang-format on
