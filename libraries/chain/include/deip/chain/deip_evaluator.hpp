#pragma once

#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/evaluator.hpp>

namespace deip {
namespace chain {

using namespace deip::protocol;

DEFINE_EVALUATOR(account_create)
DEFINE_EVALUATOR(account_create_with_delegation)
DEFINE_EVALUATOR(account_update)
DEFINE_EVALUATOR(transfer)
DEFINE_EVALUATOR(transfer_to_vesting)
DEFINE_EVALUATOR(witness_update)
DEFINE_EVALUATOR(account_witness_vote)
DEFINE_EVALUATOR(account_witness_proxy)
DEFINE_EVALUATOR(withdraw_vesting)
DEFINE_EVALUATOR(set_withdraw_vesting_route)
DEFINE_EVALUATOR(vote)
DEFINE_EVALUATOR(request_account_recovery)
DEFINE_EVALUATOR(recover_account)
DEFINE_EVALUATOR(change_recovery_account)
DEFINE_EVALUATOR(delegate_vesting_shares)
DEFINE_EVALUATOR(create_grant)
DEFINE_EVALUATOR(create_proposal)
DEFINE_EVALUATOR(create_research_group)
DEFINE_EVALUATOR(make_research_review)
DEFINE_EVALUATOR(contribute_to_token_sale)
DEFINE_EVALUATOR(approve_research_group_invite)
DEFINE_EVALUATOR(reject_research_group_invite)
DEFINE_EVALUATOR(create_research_group_join_request)
DEFINE_EVALUATOR(reject_research_group_join_request)
DEFINE_EVALUATOR(transfer_research_tokens_to_research_group)

}
} // deip::chain
