#pragma once

#include <functional>

#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/global_property_object.hpp>

#include <deip/chain/evaluator.hpp>
#include <deip/chain/dbservice.hpp>

#include <deip/chain/dbs_account.hpp>
#include <deip/chain/dbs_proposal.hpp>
#include <deip/chain/dbs_research_group.hpp>

#include <deip/chain/dbs_proposal_execution.hpp>

#include <deip/chain/proposal_object.hpp>
#include <deip/chain/proposal_data_types.hpp>

namespace deip {
namespace chain {

using namespace deip::protocol;

// clang-format off
template <typename AccountService,
        typename ProposalService,
        typename ResearchGroupService,
        typename ProposalExecutionService,
        typename OperationType = deip::protocol::operation>
class proposal_vote_evaluator_t : public evaluator<OperationType>
// clang-format on
{
public:
    typedef vote_proposal_operation operation_type;

    proposal_vote_evaluator_t(AccountService& account_service,
                              ProposalService& proposal_service,
                              ResearchGroupService& research_group_service,
                              ProposalExecutionService& proposal_execution_service)

            : _account_service(account_service)
            , _proposal_service(proposal_service)
            , _research_group_service(research_group_service)
            , _proposal_execution_service(proposal_execution_service)
    {
    }

    virtual void apply(const OperationType& o) final override
    {
        const auto& op = o.template get<operation_type>();
        this->do_apply(op);
    }

    virtual int get_type() const override
    {
        return OperationType::template tag<operation_type>::value;
    }

    void do_apply(const vote_proposal_operation& op)
    {
        _research_group_service.check_research_group_token_existence(op.voter, op.research_group_id);
        _account_service.check_account_existence(op.voter);
        _proposal_service.check_proposal_existence(op.proposal_id);

        const proposal_object& proposal = _proposal_service.get_proposal(op.proposal_id);

        FC_ASSERT(proposal.voted_accounts.find(op.voter) == proposal.voted_accounts.end(),
                  "Account \"${account}\" already voted", ("account", op.voter));

        FC_ASSERT(!_proposal_service.is_expired(proposal), "Proposal '${id}' is expired.", ("id", op.proposal_id));

        _proposal_service.vote_for(op.proposal_id, op.voter);

        execute_proposal(proposal);
    }

protected:
    virtual void execute_proposal(const proposal_object& proposal)
    {
        if (is_quorum(proposal))
        {
            _proposal_execution_service.execute_proposal(proposal);
            _proposal_service.remove(proposal);
        }
    }

    bool is_quorum(const proposal_object& proposal)
    {
        const research_group_object& research_group = _research_group_service.get_research_group(proposal.research_group_id);

        float total_voted_weight = 0;
        auto& votes = _proposal_service.get_votes_for(proposal.id);
        for (const proposal_vote_object& vote : votes) {
            total_voted_weight += vote.weight.value;
        }
        return total_voted_weight  >= research_group.quorum_percent;
    }

    AccountService& _account_service;
    ProposalService& _proposal_service;
    ResearchGroupService& _research_group_service;
    ProposalExecutionService& _proposal_execution_service;

};

typedef proposal_vote_evaluator_t<dbs_account,
                                  dbs_proposal,
                                  dbs_research_group,
                                  dbs_proposal_execution>
    proposal_vote_evaluator;

} // namespace chain
} // namespace deip
