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
                vote_operation,

                transfer_operation,
                transfer_to_vesting_operation,
                withdraw_vesting_operation,

                account_create_operation,
                account_update_operation,

                witness_update_operation,
                account_witness_vote_operation,
                account_witness_proxy_operation,

                set_withdraw_vesting_route_operation,

                request_account_recovery_operation,
                recover_account_operation,
                change_recovery_account_operation,

                delegate_vesting_shares_operation,
                account_create_with_delegation_operation,

                // DEIP native operations
                create_grant_operation,
                create_research_group_operation,
                create_proposal_operation,
                vote_proposal_operation,
                make_research_review_operation,
                contribute_to_token_sale_operation,
                approve_research_group_invite_operation,
                reject_research_group_invite_operation,
                create_research_group_join_request_operation,
                reject_research_group_join_request_operation,
                transfer_research_tokens_to_research_group_operation,
                add_expertise_tokens_operation,
                research_update_operation,

                // virtual operations
                fill_vesting_withdraw_operation,
                shutdown_witness_operation,
                hardfork_operation,
                return_vesting_delegation_operation,
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
