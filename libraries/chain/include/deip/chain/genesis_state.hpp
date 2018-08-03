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

    struct research_group_type
    {
        dc::research_group_id_type id;
        std::string name;
        std::string description;
        std::string permlink;
        uint16_t quorum_percent;
        std::vector<std::string> members;
        bool is_personal;
    };

    struct research_type
    {
        dc::research_id_type id;
        dc::research_group_id_type research_group_id;
        std::string title;
        std::string abstract;
        std::string permlink;
        bool is_finished;
        uint16_t review_share_in_percent;
        uint16_t dropout_compensation_in_percent;
        std::vector<int64_t> disciplines;
    };

    struct research_content_type
    {
        dc::research_id_type research_id;
        uint16_t type;
        std::string title;
        std::string content;
        std::string permlink;
        std::vector<std::string> authors;
        std::vector<int64_t> references;
    };

    struct vesting_contract_type
    {
        dc::vesting_contract_id_type id;
        std::string sender;
        std::string receiver;
        uint16_t balance;
        uint32_t withdrawal_periods;
        uint16_t contract_duration;
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
    std::vector<research_group_type> research_groups;
    std::vector<research_type> researches;
    std::vector<research_content_type> research_contents;
    std::vector<vesting_contract_type> vesting_contracts;

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

FC_REFLECT(deip::chain::genesis_state_type::research_group_type,
           (id)
           (name)
           (description)
           (permlink)
           (quorum_percent)
           (members)
           (is_personal))


FC_REFLECT(deip::chain::genesis_state_type::research_type,
           (id)
           (research_group_id)
           (title)
           (abstract)
           (permlink)
           (is_finished)
           (review_share_in_percent)
           (dropout_compensation_in_percent)
           (disciplines))


FC_REFLECT(deip::chain::genesis_state_type::research_content_type,
           (research_id)
           (type)
           (title)
           (permlink)
           (content)
           (authors)
           (references))

FC_REFLECT(deip::chain::genesis_state_type::vesting_contract_type,
           (id)
           (sender)
           (receiver)
           (balance)
           (withdrawal_periods)
           (contract_duration))

FC_REFLECT(deip::chain::genesis_state_type,
           (init_supply)
           (init_rewards_supply)
           (initial_timestamp)
           (accounts)
           (witness_candidates)
           (expert_tokens)
           (disciplines)
           (research_groups)
           (researches)
           (research_contents)
           (initial_chain_id)
           (vesting_contracts))
// clang-format on