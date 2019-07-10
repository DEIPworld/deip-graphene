#pragma once

#include <deip/protocol/operation_util.hpp>
#include <deip/protocol/deip_operations.hpp>
#include <deip/protocol/deip_virtual_operations.hpp>

namespace deip {
    namespace protocol {

/** NOTE: do not change the order of any operations prior to the virtual operations
 * or it will trigger a hardfork.
 */
        typedef fc::static_variant<
                vote_for_review_operation,

                transfer_operation,
                transfer_to_common_tokens_operation,
                withdraw_common_tokens_operation,

                account_create_operation,
                account_update_operation,

                witness_update_operation,
                account_witness_vote_operation,
                account_witness_proxy_operation,

                set_withdraw_common_tokens_route_operation,

                request_account_recovery_operation,
                recover_account_operation,
                change_recovery_account_operation,

                // DEIP native operations
                create_discipline_supply_operation,
                create_research_group_operation,
                create_proposal_operation,
                vote_proposal_operation,
                make_review_operation,
                contribute_to_token_sale_operation,
                approve_research_group_invite_operation,
                reject_research_group_invite_operation,
                transfer_research_tokens_to_research_group_operation,
                set_expertise_tokens_operation,
                research_update_operation,
                create_vesting_balance_operation,
                withdraw_vesting_balance_operation,
                transfer_research_tokens_operation,
                delegate_expertise_operation,
                revoke_expertise_delegation_operation,
                create_expertise_allocation_proposal_operation,
                vote_for_expertise_allocation_proposal_operation,
                accept_research_token_offer_operation,
                reject_research_token_offer_operation,
                create_grant_operation,
                create_grant_application_operation,
                add_member_to_research_operation,
                exclude_member_from_research_operation,

                // virtual operations
                fill_common_tokens_withdraw_operation,
                shutdown_witness_operation,
                hardfork_operation,
                producer_reward_operation>
                operation;

/*void operation_get_required_authorities( const operation& op,
                                         flat_set<string>& active,
                                         flat_set<string>& owner,
                                         flat_set<string>& posting,
                                         vector<authority>&  other );

void operation_validate( const operation& op );*/

        bool is_market_operation(const operation& op);

        bool is_virtual_operation(const operation& op);
    } // namespace protocol
} // namespace deip

/*namespace fc {
    void to_variant( const deip::protocol::operation& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  deip::protocol::operation& vo );
}*/

DECLARE_OPERATION_TYPE(deip::protocol::operation)
FC_REFLECT_TYPENAME(deip::protocol::operation)
