#pragma once

#include <vector>
#include <string>

#include <deip/protocol/types.hpp>
#include <deip/protocol/asset.hpp>
#include <deip/chain/schema/deip_object_types.hpp>

#include <fc/reflect/reflect.hpp>

namespace deip {
namespace chain {

struct genesis_state_type
{
    struct registrar_account_type
    {
        std::string name;
        std::string recovery_account;
        protocol::public_key_type public_key;
        share_type common_tokens_amount = 0;
    };

    struct account_type
    {
        std::string name;
        std::string recovery_account;
        protocol::public_key_type public_key;
    };

    struct asset_type
    {
        std::string symbol;
        uint8_t precision;
        share_type current_supply;
    };

    struct account_balance_type
    {
        std::string owner;
        share_type amount;
        std::string symbol = protocol::asset(0, DEIP_SYMBOL).symbol_name();
    };

    struct witness_type
    {
        std::string owner_name;
        protocol::public_key_type block_signing_key;
    };

    struct discipline_type
    {
        std::string name;
        protocol::external_id_type external_id;
        protocol::external_id_type parent_external_id;
    };

    struct expert_token_type
    {
        std::string account;
        int64_t amount;
        protocol::external_id_type discipline_external_id;
    };

    struct research_group_type
    {
        account_name_type account;
        protocol::public_key_type public_key;
        account_name_type creator;
        std::string description;
        std::set<account_name_type> members;
    };

    struct research_type
    {
        protocol::external_id_type external_id;
        account_name_type account;
        std::string description;
        bool is_finished;
        bool is_private;
        std::set<account_name_type> members;
        std::set<protocol::external_id_type> disciplines;
    };

    struct research_content_type
    {
        protocol::external_id_type external_id;
        protocol::external_id_type research_external_id;
        std::string description;
        std::string content;
        uint16_t type;
        flat_set<account_name_type> authors;
        flat_set<protocol::external_id_type> references;
    };

    struct vesting_balance_type
    {
        vesting_balance_id_type id;
        std::string owner;
        uint32_t balance;
        uint32_t vesting_duration_seconds;
        uint32_t vesting_cliff_seconds;
        uint32_t period_duration_seconds;
    };
    
    genesis_state_type()
        : init_supply(0)
    {
    }

    genesis_state_type(const share_type& supply)
        : init_supply(supply)
    {
    }

    share_type init_supply;
    protocol::asset init_rewards_supply;
    time_point_sec initial_timestamp;
    registrar_account_type registrar_account;
    std::vector<account_type> accounts;
    std::vector<asset_type> assets;
    std::vector<account_balance_type> account_balances;
    std::vector<witness_type> witness_candidates;
    std::vector<discipline_type> disciplines;
    std::vector<expert_token_type> expert_tokens;
    std::vector<research_group_type> research_groups;
    std::vector<research_type> researches;
    std::vector<research_content_type> research_contents;
    std::vector<vesting_balance_type> vesting_balances;

    chain_id_type initial_chain_id;
};

namespace utils {

void generate_default_genesis_state(genesis_state_type& genesis);

} // namespace utils
} // namespace chain
} // namespace deip

// clang-format off
FC_REFLECT(deip::chain::genesis_state_type::registrar_account_type,
           (name)
           (recovery_account)
           (public_key)
           (common_tokens_amount))

FC_REFLECT(deip::chain::genesis_state_type::account_type,
           (name)
           (recovery_account)
           (public_key))

FC_REFLECT(deip::chain::genesis_state_type::asset_type,
           (symbol)
           (precision)
           (current_supply))

FC_REFLECT(deip::chain::genesis_state_type::account_balance_type,
           (owner)
           (amount)
           (symbol))

FC_REFLECT(deip::chain::genesis_state_type::witness_type,
           (owner_name)
           (block_signing_key))

FC_REFLECT(deip::chain::genesis_state_type::discipline_type,
           (name)
           (external_id)
           (parent_external_id))

FC_REFLECT(deip::chain::genesis_state_type::expert_token_type,
          (account)
          (amount)
          (discipline_external_id))

FC_REFLECT(deip::chain::genesis_state_type::research_group_type,
          (account)
          (public_key)
          (creator)
          (description)
          (members)
)

FC_REFLECT(deip::chain::genesis_state_type::research_type,
          (external_id)
          (account)
          (description)
          (is_finished)
          (is_private)
          (members)
          (disciplines)
)

FC_REFLECT(deip::chain::genesis_state_type::research_content_type,
          (external_id)
          (research_external_id)
          (description)
          (content)
          (type)
          (authors)
          (references)
)

FC_REFLECT(deip::chain::genesis_state_type::vesting_balance_type,
           (id)
           (owner)
           (balance)
           (vesting_duration_seconds)
           (vesting_cliff_seconds)
           (period_duration_seconds))

FC_REFLECT(deip::chain::genesis_state_type,
           (init_supply)
           (init_rewards_supply)
           (initial_timestamp)
           (registrar_account)
           (accounts)
           (assets)
           (account_balances)
           (witness_candidates)
           (expert_tokens)
           (disciplines)
           (research_groups)
           (researches)
           (research_contents)
           (initial_chain_id)
           (vesting_balances))
// clang-format on