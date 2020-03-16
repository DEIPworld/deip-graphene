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
        share_type common_tokens_amount;
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
        discipline_id_type id;
        std::string name;
        share_type votes_in_last_ten_weeks;
        discipline_id_type parent_id;
    };

    struct expert_token_type
    {
        std::string account_name;
        discipline_id_type discipline_id;
        uint32_t amount;
    };

    struct research_group_type
    {
        research_group_id_type id;
        std::string name;
        std::string description;
        std::string permlink;
        fc::optional<uint32_t> default_quorum;
        std::vector<std::string> members;
        int32_t management_model_v;
        fc::optional<research_group_id_type> organization_id;
        fc::optional<std::vector<account_name_type>> organization_agents;
        std::vector<research_group_type> subgroups;
    };

    struct research_type
    {
        research_id_type id;
        research_group_id_type research_group_id;
        std::string title;
        std::string abstract;
        std::string permlink;
        bool is_finished;
        uint16_t review_share_in_percent;
        uint16_t dropout_compensation_in_percent;
        std::vector<int64_t> disciplines;
        bool is_private;
    };

    struct research_content_type
    {
        research_content_id_type id;
        research_id_type research_id;
        uint16_t type;
        std::string title;
        std::string content;
        std::string permlink;
        std::vector<std::string> authors;
        std::vector<int64_t> references;
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
           (id)
           (name)
           (votes_in_last_ten_weeks)
           (parent_id))

FC_REFLECT(deip::chain::genesis_state_type::expert_token_type,
           (account_name)
           (discipline_id)
           (amount))

FC_REFLECT(deip::chain::genesis_state_type::research_group_type,
           (id)
           (name)
           (description)
           (permlink)
           (default_quorum)
           (members)
           (management_model_v)
           (organization_id)
           (organization_agents)
           (subgroups))


FC_REFLECT(deip::chain::genesis_state_type::research_type,
           (id)
           (research_group_id)
           (title)
           (abstract)
           (permlink)
           (is_finished)
           (review_share_in_percent)
           (dropout_compensation_in_percent)
           (disciplines)
           (is_private))


FC_REFLECT(deip::chain::genesis_state_type::research_content_type,
           (id)
           (research_id)
           (type)
           (title)
           (permlink)
           (content)
           (authors)
           (references))

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