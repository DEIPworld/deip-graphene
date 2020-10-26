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
typedef fc::static_variant<create_account_operation, // 0
                           update_account_operation, // 1

                           transfer_operation, // 2
                           transfer_to_common_tokens_operation, // 3
                           withdraw_common_tokens_operation, // 4
                           set_withdraw_common_tokens_route_operation, // 5

                           witness_update_operation, // 6
                           account_witness_vote_operation, // 7
                           account_witness_proxy_operation, // 8

                           request_account_recovery_operation, // 9
                           recover_account_operation, // 10
                           change_recovery_account_operation, // 11

                           join_research_group_membership_operation, // 12
                           leave_research_group_membership_operation, // 13
                           create_research_operation, // 14
                           update_research_operation, // 15
                           create_research_content_operation, // 16

                           create_review_operation, // 17
                           vote_for_review_operation, // 18

                           create_research_token_sale_operation, // 19
                           contribute_to_token_sale_operation, // 20
                           transfer_research_share_operation, // 21

                           create_asset_operation, // 22
                           issue_asset_operation, // 23
                           reserve_asset_operation, // 24

                           create_vesting_balance_operation, //  25
                           withdraw_vesting_balance_operation, // 26

                           create_proposal_operation, // 27
                           update_proposal_operation, // 28
                           delete_proposal_operation, // 29

                           create_expertise_allocation_proposal_operation, // 30
                           vote_for_expertise_allocation_proposal_operation, // 31

                           create_grant_operation, // 32
                           create_grant_application_operation, // 33
                           create_review_for_application_operation, // 34
                           approve_grant_application_operation, // 35
                           reject_grant_application_operation, // 36
                           create_award_operation, // 37
                           approve_award_operation, // 38
                           reject_award_operation, // 39
                           create_award_withdrawal_request_operation, // 40
                           certify_award_withdrawal_request_operation, // 41
                           approve_award_withdrawal_request_operation, // 42
                           reject_award_withdrawal_request_operation, // 43
                           pay_award_withdrawal_request_operation, // 44

                           create_nda_contract_operation, // 45
                           sign_nda_contract_operation, // 46
                           decline_nda_contract_operation, // 47
                           close_nda_contract_operation, // 48
                           create_request_by_nda_contract_operation, // 49
                           fulfill_request_by_nda_contract_operation, // 50

                           create_assessment_operation, // 51
                           create_security_token_operation, // 52
                           transfer_security_token_operation, // 53
                           create_research_license_operation,

                           // virtual operations
                           fill_common_tokens_withdraw_operation,
                           shutdown_witness_operation,
                           hardfork_operation,
                           producer_reward_operation,
                           token_sale_contribution_to_history_operation,
                           research_content_reference_history_operation,
                           research_content_eci_history_operation,
                           research_eci_history_operation,
                           account_eci_history_operation,
                           disciplines_eci_history_operation,
                           account_revenue_income_history_operation>

    operation;

bool is_market_operation(const operation& op);

bool is_virtual_operation(const operation& op);

void operation_validate(const operation& op);

void entity_validate(const operation& op, uint16_t ref_block_num, uint32_t ref_block_prefix);

void operation_get_required_authorities(const operation& op,
                                        flat_set<account_name_type>& active,
                                        flat_set<account_name_type>& owner,
                                        vector<authority>& other);

struct authority_pack
{
    authority owner;
    authority active;
    flat_map<uint16_t, authority> active_overrides;
};

void extract_new_accounts(const vector<operation>& ops, flat_map<account_name_type, authority_pack>& accounts_auth);

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

} // namespace protocol
} // namespace deip

DECLARE_STATIC_VARIANT_TYPE(deip::protocol::operation)
FC_REFLECT_TYPENAME(deip::protocol::operation)
FC_REFLECT(deip::protocol::op_wrapper, (op))