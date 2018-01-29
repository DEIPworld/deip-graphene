#pragma once

#include <functional>

#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/global_property_object.hpp>

#include <deip/chain/evaluator.hpp>
#include <deip/chain/dbservice.hpp>

#include <deip/chain/dbs_account.hpp>
#include <deip/chain/dbs_proposal.hpp>
#include <deip/chain/dbs_research_group.hpp>
#include <deip/chain/dbs_research_token.hpp>
#include <deip/chain/dbs_research.hpp>

#include <deip/chain/proposal_object.hpp>
#include <deip/chain/proposal_data_types.hpp>

namespace deip {
namespace chain {

using namespace deip::protocol;

// clang-format off
template <typename AccountService,
        typename ProposalService,
        typename ResearchGroupService,
        typename ResearchService,
        typename ResearchTokenService,
        typename OperationType = deip::protocol::operation>
class proposal_vote_evaluator_t : public evaluator<OperationType>
// clang-format on
{
public:
    typedef proposal_vote_operation operation_type;
    typedef proposal_vote_evaluator_t<AccountService,
            ProposalService,
            ResearchGroupService,
            ResearchService,
            ResearchTokenService,
            OperationType>
            EvaluatorType;

    using evaluator_callback = std::function<void(const proposal_object&)>;

    class proposal_evaluators_register
    {
    public:
        void set(proposal_action_type action, evaluator_callback callback)
        {
            _register.insert(std::make_pair(action, callback));
        }

        void execute(const proposal_object& proposal)
        {
            if (_register.count(proposal.action) == 0)
            {
                FC_ASSERT("Invalid proposal action type");
            }
            else
            {
                _register[proposal.action](proposal);
            }
        }

    private:
        fc::flat_map<proposal_action_type, evaluator_callback> _register;
    };

    proposal_vote_evaluator_t(AccountService& account_service,
                              ProposalService& proposal_service,
                              ResearchGroupService& research_group_service,
                              ResearchService& research_service,
                              ResearchTokenService& research_token_service
                              )
            : _account_service(account_service)
            , _proposal_service(proposal_service)
            , _research_group_service(research_group_service)
            , _research_service(research_service)
            , _research_token_service(research_token_service)
    {
        evaluators.set(proposal_action_type::invite_member,
                       std::bind(&EvaluatorType::invite_evaluator, this, std::placeholders::_1));
        evaluators.set(proposal_action_type::dropout_member,
                       std::bind(&EvaluatorType::dropout_evaluator, this, std::placeholders::_1));
        evaluators.set(proposal_action_type::change_quorum,
                       std::bind(&EvaluatorType::change_quorum_evaluator, this, std::placeholders::_1));
        evaluators.set(proposal_action_type::change_research_review_share_percent,
                       std::bind(&EvaluatorType::change_research_review_share_evaluator, this, std::placeholders::_1));
        evaluators.set(proposal_action_type::start_research,
                       std::bind(&EvaluatorType::start_research_evaluator, this, std::placeholders::_1));
        evaluators.set(proposal_action_type::transfer_research_tokens,
                       std::bind(&EvaluatorType::transfer_research_tokens_evaluator, this, std::placeholders::_1));
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

    void do_apply(const proposal_vote_operation& op)
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
            evaluators.execute(proposal);
            _proposal_service.remove(proposal);
        }
    }

    bool is_quorum(const proposal_object& proposal)
    {
        const research_group_object& research_group = _research_group_service.get_research_group(proposal.research_group_id);
        auto& total_tokens = research_group.total_tokens_amount;

        float total_voted_weight = 0;
        auto& votes = _proposal_service.get_votes_for(proposal.id);
        for (const proposal_vote_object& vote : votes) {
            total_voted_weight += vote.weight.value;
        }
        return (total_voted_weight * DEIP_1_PERCENT) / total_tokens  >= research_group.quorum_percent;
    }

    void invite_evaluator(const proposal_object& proposal)
    {
        invite_member_proposal_data_type data = get_data<invite_member_proposal_data_type>(proposal);
        _account_service.check_account_existence(data.name);
        _research_group_service.check_research_group_existence(data.research_group_id);
        _research_group_service.create_research_group_token(data.research_group_id, data.research_group_token_amount, data.name);
        _research_group_service.adjust_research_group_token_amount(data.research_group_id, data.research_group_token_amount);
    }

    void dropout_evaluator(const proposal_object& proposal)
    {
        member_proposal_data_type data = get_data<member_proposal_data_type>(proposal);
        _account_service.check_account_existence(data.name);
        _research_group_service.check_research_group_token_existence(data.name, data.research_group_id);
        _proposal_service.remove_proposal_votes(data.name, data.research_group_id);
        auto& token = _research_group_service.get_research_group_token_by_account_and_research_id(data.name, data.research_group_id);
        auto tokens_amount = token.amount;
        _research_group_service.remove_token(data.name, data.research_group_id);
        _research_group_service.adjust_research_group_token_amount(data.research_group_id, -tokens_amount);
    }

    void change_research_review_share_evaluator(const proposal_object& proposal)
    {
//        uint64_t quorum = proposal.data.as_uint64();
//        _properties_service.set_dropout_quorum(quorum);
    }

    void change_quorum_evaluator(const proposal_object& proposal)
    {
        change_quorum_proposal_data_type data = get_data<change_quorum_proposal_data_type>(proposal);
        _research_group_service.check_research_group_existence(data.research_group_id);
        _research_group_service.change_quorum(data.quorum_percent, data.research_group_id);
    }

    void start_research_evaluator(const proposal_object& proposal)
    {
        start_research_proposal_data_type data = get_data<start_research_proposal_data_type>(proposal);
        _research_group_service.check_research_group_existence(data.research_group_id);
        _research_service.create(data.name, data.abstract, data.permlink, data.research_group_id, data.percent_for_review);
    }

    void transfer_research_tokens_evaluator(const proposal_object& proposal)
    {
        transfer_research_tokens_data_type data = get_data<transfer_research_tokens_data_type>(proposal);
        _research_service.check_research_existence(data.research_id);
        _account_service.check_account_existence(data.account_name);
        auto& account = _account_service.get_account(data.account_name);
        auto& research = _research_service.get_research(data.research_id);

        FC_ASSERT((account.balance.amount - data.total_price.amount > 0), "Account balance is less that total price (result amount < 0)");
        FC_ASSERT((research.owned_tokens - data.amount > 0), "Research balance is less than amount (result amount < 0)");
        _account_service.decrease_balance(account, data.total_price);
        //_research_group_service.adjust_research_group_token_amount(proposal.research_group_id, data.amount);
        _research_service.decrease_owned_tokens(research, data.amount);
        _research_token_service.create_research_token(account.name, data.amount, data.research_id);
    }
    AccountService& _account_service;
    ProposalService& _proposal_service;
    ResearchGroupService& _research_group_service;
    ResearchService& _research_service;
    ResearchTokenService& _research_token_service;

private:
    proposal_evaluators_register evaluators;

    template <typename DataType> DataType get_data(const proposal_object& proposal)
    {
        auto data = fc::json::from_string(proposal.data).as<DataType>();
        data.validate();
        return data;
    }
};

typedef proposal_vote_evaluator_t<dbs_account, dbs_proposal, dbs_research_group, dbs_research, dbs_research_token>
        proposal_vote_evaluator;

} // namespace chain
} // namespace deip
