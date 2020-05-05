#pragma once

#include <deip/protocol/deip_operations.hpp>
#include <deip/protocol/deip_virtual_operations.hpp>
#include <boost/container/flat_set.hpp>
#include <deip/protocol/authority.hpp>
#include <fc/variant.hpp>
#include <vector>

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

                create_account_operation,
                update_account_operation,

                witness_update_operation,
                account_witness_vote_operation,
                account_witness_proxy_operation,

                set_withdraw_common_tokens_route_operation,

                request_account_recovery_operation,
                recover_account_operation,
                change_recovery_account_operation,

                // DEIP native operations
                placeholder1_operation,
                delete_proposal_operation,
                create_proposal_operation,
                update_proposal_operation,
                make_review_operation,
                contribute_to_token_sale_operation,
                placeholder2_operation,
                placeholder3_operation,
                placeholder4_operation,

                placeholder5_operation,
                placeholder6_operation,

                create_vesting_balance_operation,
                withdraw_vesting_balance_operation,

                transfer_research_share_operation,

                delegate_expertise_operation,
                revoke_expertise_delegation_operation,
                create_expertise_allocation_proposal_operation,
                vote_for_expertise_allocation_proposal_operation,
                placeholder7_operation,
                placeholder8_operation,

                create_grant_operation,
                create_grant_application_operation,
                make_review_for_application_operation,
                approve_grant_application_operation,
                reject_grant_application_operation,
                create_asset_operation,
                issue_asset_operation,
                reserve_asset_operation,

                create_award_operation,
                approve_award_operation,
                reject_award_operation,
                create_award_withdrawal_request_operation,
                certify_award_withdrawal_request_operation,
                approve_award_withdrawal_request_operation,
                reject_award_withdrawal_request_operation,
                pay_award_withdrawal_request_operation,

                join_research_group_membership_operation,
                left_research_group_membership_operation,
                create_research_operation,
                create_research_content_operation,
                create_research_token_sale_operation,
                placeholder9_operation,
                update_research_operation,

                create_nda_contract_operation,
                sign_nda_contract_operation,
                decline_nda_contract_operation,
                close_nda_contract_operation,
                create_request_by_nda_contract_operation,
                fulfill_request_by_nda_contract_operation,

                // virtual operations
                fill_common_tokens_withdraw_operation,
                shutdown_witness_operation,
                hardfork_operation,
                producer_reward_operation,
                token_sale_contribution_to_history_operation,
                research_content_reference_history_operation,
                research_content_eci_history_operation,
                research_eci_history_operation,
                account_eci_history_operation>
                operation;


        bool is_market_operation(const operation& op);

        bool is_virtual_operation(const operation& op);

        void operation_validate(const operation& op);

        void operation_get_required_authorities(const operation& op,
                                                flat_set<account_name_type>& active,
                                                flat_set<account_name_type>& owner,
                                                flat_set<account_name_type>& posting,
                                                vector<authority>& other);

        /**
         * op_wrapper is used to get around the circular definition of operation and proposals that contain them.
         */
        struct op_wrapper
        {
            op_wrapper(const operation& op = operation())
                : op(op)
            {
            }

            operation op;
        };

} //  deip::protocol
} //  deip

namespace fc {

using namespace deip::protocol;

void to_variant(const operation&, fc::variant&);
void from_variant(const fc::variant&, operation&);

} //

FC_REFLECT_TYPENAME(deip::protocol::operation)
FC_REFLECT(deip::protocol::op_wrapper, (op))