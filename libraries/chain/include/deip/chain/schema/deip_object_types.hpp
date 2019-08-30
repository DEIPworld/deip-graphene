#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <fc/shared_string.hpp>
#include <chainbase/chainbase.hpp>

#include <deip/protocol/types.hpp>
#include <deip/protocol/authority.hpp>

namespace deip {
namespace chain {

namespace bip = chainbase::bip;
using namespace boost::multi_index;

using boost::multi_index_container;

using chainbase::allocator;
using chainbase::object;
using chainbase::oid;

using deip::protocol::account_name_type;
using deip::protocol::block_id_type;
using deip::protocol::chain_id_type;
using deip::protocol::share_type;
using deip::protocol::transaction_id_type;
using deip::protocol::discipline_name_type;

struct by_id;

enum object_type
{
    dynamic_global_property_object_type,
    chain_property_object_type,
    account_object_type,
    account_authority_object_type,
    witness_object_type,
    transaction_object_type,
    block_summary_object_type,
    witness_schedule_object_type,
    witness_vote_object_type,
    hardfork_property_object_type,
    operation_object_type,
    withdraw_common_tokens_route_object_type,
    owner_authority_history_object_type,
    account_recovery_request_object_type,
    change_recovery_account_request_object_type,
    block_stats_object_type,
    reward_fund_object_type,
    discipline_supply_object_type,
    discipline_object_type,
    vote_object_type,
    total_votes_object_type,
    research_object_type,
    research_discipline_relation_object_type,
    research_content_object_type,
    proposal_object_type,
    research_group_object_type,
    research_group_token_object_type,
    proposal_vote_object_type,
    expert_token_object_type,
    research_token_object_type,
    research_token_sale_object_type,
    research_token_sale_contribution_object_type,
    research_group_invite_object_type,
    review_object_type,
    review_vote_object_type,
    vesting_balance_object_type,
    reward_pool_object_type,
    expertise_allocation_proposal_object_type,
    expertise_allocation_proposal_vote_object_type,
    expertise_stats_object_type,
    offer_research_tokens_object_type,
    grant_object_type,
    grant_application_object_type,
    contract_object_type,
    contract_file_access_object_type
};

class dynamic_global_property_object;
class chain_property_object;
class account_object;
class account_authority_object;
class witness_object;
class transaction_object;
class block_summary_object;
class witness_schedule_object;
class witness_vote_object;
class hardfork_property_object;
class operation_object;
class withdraw_common_tokens_route_object;

class owner_authority_history_object;
class account_recovery_request_object;
class change_recovery_account_request_object;
class block_stats_object;
class reward_fund_object;
class discipline_supply_object;
class proposal_object;
class research_group_object;
class research_group_token_object;
class discipline_object;
class vote_object;
class total_votes_object;
class research_object;
class research_discipline_relation_object;
class research_content_object;
class proposal_vote_object;
class expert_token_object;
class research_token_object;
class research_token_sale_object;
class research_token_sale_contribution_object;
class research_group_invite_object;
class review_object;
class review_vote_object;
class vesting_balance_object;
class reward_pool_object;
class expertise_allocation_proposal_object;
class expertise_allocation_proposal_vote_object;
class expertise_stats_object;
class offer_research_tokens_object;
class grant_object;
class grant_application_object;
class contract_object;
class contract_file_access_object;

typedef oid<dynamic_global_property_object> dynamic_global_property_id_type;
typedef oid<chain_property_object> chain_property_id_type;
typedef oid<account_object> account_id_type;
typedef oid<account_authority_object> account_authority_id_type;
typedef oid<witness_object> witness_id_type;
typedef oid<transaction_object> transaction_object_id_type;
typedef oid<block_summary_object> block_summary_id_type;
typedef oid<witness_schedule_object> witness_schedule_id_type;
typedef oid<witness_vote_object> witness_vote_id_type;
typedef oid<hardfork_property_object> hardfork_property_id_type;
typedef oid<operation_object> operation_id_type;
typedef oid<withdraw_common_tokens_route_object> withdraw_common_tokens_route_id_type;
typedef oid<owner_authority_history_object> owner_authority_history_id_type;
typedef oid<account_recovery_request_object> account_recovery_request_id_type;
typedef oid<change_recovery_account_request_object> change_recovery_account_request_id_type;
typedef oid<block_stats_object> block_stats_id_type;
typedef oid<reward_fund_object> reward_fund_id_type;
typedef oid<discipline_supply_object> discipline_supply_id_type;
typedef oid<proposal_object> proposal_id_type;
typedef oid<research_group_object> research_group_id_type;
typedef oid<research_group_token_object> research_group_token_id_type;
typedef oid<discipline_object> discipline_id_type;
typedef oid<vote_object> vote_id_type;
typedef oid<total_votes_object> total_votes_id_type;
typedef oid<research_object> research_id_type;
typedef oid<research_discipline_relation_object> research_discipline_relation_id_type;
typedef oid<research_content_object> research_content_id_type;
typedef oid<proposal_vote_object> proposal_vote_id_type;
typedef oid<expert_token_object> expert_token_id_type;
typedef oid<research_token_object> research_token_id_type;
typedef oid<research_token_sale_object> research_token_sale_id_type;
typedef oid<research_token_sale_contribution_object> research_token_sale_contribution_id_type;
typedef oid<research_group_invite_object> research_group_invite_id_type;
typedef oid<review_object> review_id_type;
typedef oid<review_vote_object> review_vote_id_type;
typedef oid<vesting_balance_object> vesting_balance_id_type;
typedef oid<reward_pool_object> reward_pool_id_type;
typedef oid<expertise_allocation_proposal_object> expertise_allocation_proposal_id_type;
typedef oid<expertise_allocation_proposal_vote_object> expertise_allocation_proposal_vote_id_type;
typedef oid<expertise_stats_object> expertise_stats_id_type;
typedef oid<offer_research_tokens_object> offer_research_tokens_id_type;
typedef oid<grant_object> grant_id_type;
typedef oid<grant_application_object> grant_application_id_type;
typedef oid<contract_object> contract_id_type;
typedef oid<contract_file_access_object> contract_file_access_id_type;


typedef allocator<account_name_type> account_name_allocator_type;
typedef bip::set<account_name_type, std::less<account_name_type>, account_name_allocator_type> account_name_type_set;

typedef allocator<research_id_type> research_id_allocator_type;
typedef bip::set<research_id_type, std::less<research_id_type>, research_id_allocator_type> research_id_type_set;

typedef allocator<research_content_id_type> research_content_id_allocator_type;
typedef bip::set<research_content_id_type, std::less<research_content_id_type>, research_content_id_allocator_type> research_content_id_type_set;

typedef allocator<fc::fixed_string_32> fixed_string_32_allocator_type;
typedef bip::set<fc::fixed_string_32, std::less<fc::fixed_string_32>, fixed_string_32_allocator_type> fixed_string_32_type_set;
    
typedef allocator<discipline_id_type> discipline_id_allocator_type;    
typedef bip::set<discipline_id_type, std::less<discipline_id_type>, discipline_id_allocator_type> discipline_id_type_set;

typedef allocator<std::pair<const discipline_id_type, share_type>> discipline_id_share_type_allocator_type;
typedef bip::map<discipline_id_type, share_type, std::less<discipline_id_type>, discipline_id_share_type_allocator_type> discipline_id_share_type_map;

typedef allocator<std::pair<const discipline_id_type, std::vector<account_name_type>>> discipline_id_account_name_allocator_type;
typedef bip::map<discipline_id_type, std::vector<account_name_type>, std::less<discipline_id_type>, discipline_id_account_name_allocator_type> delegated_expertise_type_map;

typedef allocator<std::pair<const uint16_t, share_type>> proposal_share_type_allocator_type;
typedef bip::map<uint16_t, share_type, std::less<uint16_t>, proposal_share_type_allocator_type> proposal_type_share_type_map;

typedef allocator<share_type> share_type_allocator_type;
typedef bip::deque<share_type, share_type_allocator_type> share_type_deque;


enum bandwidth_type
{
    post, ///< Rate limiting posting reward eligibility over time
    forum, ///< Rate limiting for all forum related actions
    market ///< Rate limiting for all other actions
};
} // namespace chain
} // namespace deip

// clang-format off

FC_REFLECT_ENUM( deip::chain::object_type,
                 (dynamic_global_property_object_type)
                 (chain_property_object_type)
                 (account_object_type)
                 (account_authority_object_type)
                 (witness_object_type)
                 (transaction_object_type)
                 (block_summary_object_type)
                 (witness_schedule_object_type)
                 (witness_vote_object_type)
                 (hardfork_property_object_type)
                 (operation_object_type)
                 (withdraw_common_tokens_route_object_type)
                 (owner_authority_history_object_type)
                 (account_recovery_request_object_type)
                 (change_recovery_account_request_object_type)
                 (block_stats_object_type)
                 (reward_fund_object_type)
                 (discipline_supply_object_type)
                 (proposal_object_type)
                 (research_group_object_type)
                 (research_group_token_object_type)
                 (discipline_object_type)
                 (vote_object_type)
                 (total_votes_object_type)
                 (research_object_type)
                 (research_discipline_relation_object_type)
                 (research_content_object_type)
                 (proposal_vote_object_type)
                 (expert_token_object_type)
                 (research_token_object_type)
                 (research_token_sale_object_type)
                 (research_token_sale_contribution_object_type)
                 (research_group_invite_object_type)
                 (review_object_type)
                 (review_vote_object_type)
                 (vesting_balance_object_type)
                 (reward_pool_object_type)
                 (expertise_allocation_proposal_object_type)
                 (expertise_allocation_proposal_vote_object_type)
                 (expertise_stats_object_type)
                 (offer_research_tokens_object_type)
                 (grant_application_object_type)
                 (grant_object_type)
                 (contract_object_type)
                 (contract_file_access_object_type)
                 )

FC_REFLECT_ENUM( deip::chain::bandwidth_type, (post)(forum)(market) )

// clang-format on
