#pragma once

#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/evaluator.hpp>

namespace deip {
namespace chain {

using namespace deip::protocol;

DEFINE_EVALUATOR(account_create)
DEFINE_EVALUATOR(account_update)
DEFINE_EVALUATOR(transfer)
DEFINE_EVALUATOR(transfer_to_common_tokens)
DEFINE_EVALUATOR(witness_update)
DEFINE_EVALUATOR(account_witness_vote)
DEFINE_EVALUATOR(account_witness_proxy)
DEFINE_EVALUATOR(withdraw_common_tokens)
DEFINE_EVALUATOR(set_withdraw_common_tokens_route)
DEFINE_EVALUATOR(request_account_recovery)
DEFINE_EVALUATOR(recover_account)
DEFINE_EVALUATOR(change_recovery_account)
DEFINE_EVALUATOR(create_discipline_supply)
DEFINE_EVALUATOR(create_proposal)
DEFINE_EVALUATOR(create_research_group)
DEFINE_EVALUATOR(make_review)
DEFINE_EVALUATOR(contribute_to_token_sale)
DEFINE_EVALUATOR(approve_research_group_invite)
DEFINE_EVALUATOR(reject_research_group_invite)
DEFINE_EVALUATOR(vote_for_review)
DEFINE_EVALUATOR(transfer_research_tokens_to_research_group)
DEFINE_EVALUATOR(set_expertise_tokens)
DEFINE_EVALUATOR(research_update)
DEFINE_EVALUATOR(create_vesting_balance)
DEFINE_EVALUATOR(withdraw_vesting_balance)
DEFINE_EVALUATOR(vote_proposal)
DEFINE_EVALUATOR(transfer_research_tokens)
DEFINE_EVALUATOR(delegate_expertise)
DEFINE_EVALUATOR(revoke_expertise_delegation)
DEFINE_EVALUATOR(create_expertise_allocation_proposal)
DEFINE_EVALUATOR(vote_for_expertise_allocation_proposal)
DEFINE_EVALUATOR(accept_research_token_offer)
DEFINE_EVALUATOR(reject_research_token_offer)
DEFINE_EVALUATOR(create_grant)
DEFINE_EVALUATOR(create_grant_application)
DEFINE_EVALUATOR(make_review_for_application)
DEFINE_EVALUATOR(approve_grant_application)
DEFINE_EVALUATOR(reject_grant_application)
}
} // deip::chain
