#include <deip/chain/dbs_proposal_execution.hpp>
#include <deip/chain/database.hpp>
#include <deip/chain/dbs_research_group_invite.hpp>

namespace deip {
namespace chain {

dbs_proposal_execution::dbs_proposal_execution(database &db)
    : _base_type(db)
{
    executions.set(proposal_action_type::invite_member,
                   std::bind(&dbs_proposal_execution::invite, this, std::placeholders::_1));
//    evaluators.set(proposal_action_type::dropout_member,
//                   std::bind(&EvaluatorType::dropout_evaluator, this, std::placeholders::_1));
//    evaluators.set(proposal_action_type::change_quorum,
//                   std::bind(&EvaluatorType::change_quorum_evaluator, this, std::placeholders::_1));
//    evaluators.set(proposal_action_type::change_research_review_share_percent,
//                   std::bind(&EvaluatorType::change_research_review_share_evaluator, this, std::placeholders::_1));
//    evaluators.set(proposal_action_type::start_research,
//                   std::bind(&EvaluatorType::start_research_evaluator, this, std::placeholders::_1));
//    evaluators.set(proposal_action_type::send_funds,
//                   std::bind(&EvaluatorType::send_funds_evaluator, this, std::placeholders::_1));
//    evaluators.set(proposal_action_type::rebalance_research_group_tokens,
//                   std::bind(&EvaluatorType::rebalance_research_group_tokens_evaluator, this, std::placeholders::_1));
//    evaluators.set(proposal_action_type::create_research_material,
//                   std::bind(&EvaluatorType::create_research_material_evaluator, this, std::placeholders::_1));
//    evaluators.set(proposal_action_type::start_research_token_sale,
//                   std::bind(&EvaluatorType::start_research_token_sale_evaluator, this, std::placeholders::_1));
}

void dbs_proposal_execution::invite(const proposal_object &proposal)
{
    invite_member_proposal_data_type data = get_data<invite_member_proposal_data_type>(proposal);
    auto& research_group_invite_service = db_impl().obtain_service<dbs_research_group_invite>();
    research_group_invite_service.create(data.name, data.research_group_id, data.research_group_token_amount_in_percent);
}

} //namespace chain
} //namespace deip