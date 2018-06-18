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
DEFINE_EVALUATOR(vote)
DEFINE_EVALUATOR(request_account_recovery)
DEFINE_EVALUATOR(recover_account)
DEFINE_EVALUATOR(change_recovery_account)
DEFINE_EVALUATOR(create_grant)
DEFINE_EVALUATOR(create_proposal)
DEFINE_EVALUATOR(create_research_group)
DEFINE_EVALUATOR(make_review)
DEFINE_EVALUATOR(contribute_to_token_sale)
DEFINE_EVALUATOR(approve_research_group_invite)
DEFINE_EVALUATOR(reject_research_group_invite)
DEFINE_EVALUATOR(vote_for_review)
DEFINE_EVALUATOR(transfer_research_tokens_to_research_group)
DEFINE_EVALUATOR(add_expertise_tokens)
DEFINE_EVALUATOR(research_update)
DEFINE_EVALUATOR(deposit_to_vesting_contract)
DEFINE_EVALUATOR(withdraw_from_vesting_contract)
DEFINE_EVALUATOR(vote_proposal)    

}
} // deip::chain
