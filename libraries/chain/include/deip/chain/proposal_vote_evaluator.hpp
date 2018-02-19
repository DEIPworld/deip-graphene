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
#include <deip/chain/dbs_research_content.hpp>
#include <deip/chain/dbs_research_token_sale.hpp>
#include <deip/chain/dbs_dynamic_global_properties.hpp>

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
        typename ResearchContentService,
        typename ResearchTokenSaleService,
        typename DynamicGlobalPropertiesService,
        typename OperationType = deip::protocol::operation>
class proposal_vote_evaluator_t : public evaluator<OperationType>
// clang-format on
{
public:
    typedef vote_proposal_operation operation_type;
    typedef proposal_vote_evaluator_t<AccountService,
            ProposalService,
            ResearchGroupService,
            ResearchService,
            ResearchTokenService,
            ResearchContentService,
            ResearchTokenSaleService,
            DynamicGlobalPropertiesService,
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
                              ResearchTokenService& research_token_service,
                              ResearchContentService& research_content_service,
                              ResearchTokenSaleService& research_token_sale_service,
                              DynamicGlobalPropertiesService& dynamic_global_properties_service
                              )
            : _account_service(account_service)
            , _proposal_service(proposal_service)
            , _research_group_service(research_group_service)
            , _research_service(research_service)
            , _research_token_service(research_token_service)
            , _research_content_service(research_content_service)
            , _research_token_sale_service(research_token_sale_service)
            , _dynamic_global_properties_service(dynamic_global_properties_service)
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
        evaluators.set(proposal_action_type::send_funds,
                       std::bind(&EvaluatorType::send_funds_evaluator, this, std::placeholders::_1));
        evaluators.set(proposal_action_type::rebalance_research_group_tokens,
                       std::bind(&EvaluatorType::rebalance_research_group_tokens_evaluator, this, std::placeholders::_1));
        evaluators.set(proposal_action_type::create_research_material,
                       std::bind(&EvaluatorType::create_research_material_evaluator, this, std::placeholders::_1));
        evaluators.set(proposal_action_type::start_research_token_sale,
                       std::bind(&EvaluatorType::start_research_token_sale_evaluator, this, std::placeholders::_1));
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
        _research_group_service.increase_research_group_total_tokens_amount(data.research_group_id, data.research_group_token_amount);
    }

    void dropout_evaluator(const proposal_object& proposal)
    {
        member_proposal_data_type data = get_data<member_proposal_data_type>(proposal);
        _account_service.check_account_existence(data.name);
        _research_group_service.check_research_group_token_existence(data.name, data.research_group_id);
        _proposal_service.remove_proposal_votes(data.name, data.research_group_id);
        auto& token = _research_group_service.get_research_group_token_by_account_and_research_group_id(data.name, data.research_group_id);
        auto tokens_amount = token.amount;
        _research_group_service.remove_token(data.name, data.research_group_id);
        _research_group_service.decrease_research_group_total_tokens_amount(data.research_group_id, tokens_amount);
    }

    void change_research_review_share_evaluator(const proposal_object& proposal)
    {
        change_research_review_share_percent_data_type data = get_data<change_research_review_share_percent_data_type>(proposal);
        auto& research = _research_service.get_research(data.research_id);

        int64_t time_period_from_last_update
            = (_dynamic_global_properties_service.get_dynamic_global_properties().time - research.review_share_in_percent_last_update).to_seconds();
        FC_ASSERT((time_period_from_last_update >= DAYS_TO_SECONDS(90)),
                  "Cannot update review_share (time period from last update < 90)");

        _research_service.change_research_review_share_percent(data.research_id, data.review_share_in_percent);
    }

    void change_quorum_evaluator(const proposal_object& proposal)
    {
        change_quorum_proposal_data_type data = get_data<change_quorum_proposal_data_type>(proposal);
        _research_group_service.change_quorum(data.quorum_percent, data.research_group_id);
    }

    void start_research_evaluator(const proposal_object& proposal)
    {
        start_research_proposal_data_type data = get_data<start_research_proposal_data_type>(proposal);
        _research_group_service.check_research_group_existence(data.research_group_id);
        _research_service.create(data.name, data.abstract, data.permlink, data.research_group_id, data.review_share_in_percent);
    }

    void transfer_research_tokens_evaluator(const proposal_object& proposal)
    {
        transfer_research_tokens_data_type data = get_data<transfer_research_tokens_data_type>(proposal);
        _research_service.check_research_existence(data.research_id);
        _account_service.check_account_existence(data.account_name);
        auto& account = _account_service.get_account(data.account_name);
        auto& research = _research_service.get_research(data.research_id);

        FC_ASSERT((account.balance.amount - data.total_price > 0), "Account balance is less that total price (result amount < 0)");
        FC_ASSERT((research.owned_tokens - data.amount > 0), "Research balance is less than amount (result amount < 0)");
        _account_service.decrease_balance(account, asset(data.total_price));
        _research_group_service.increase_research_group_funds(proposal.research_group_id, data.total_price);
        _research_service.decrease_owned_tokens(research, data.amount);
        _research_token_service.create_research_token(account.name, data.amount, data.research_id);
    }

    void send_funds_evaluator(const proposal_object& proposal)
    {
        send_funds_data_type data = get_data<send_funds_data_type>(proposal);
        _research_group_service.check_research_group_existence(data.research_group_id);
        _account_service.check_account_existence(data.account_name);

        auto& account = _account_service.get_account(data.account_name);
        auto& research_group = _research_group_service.get_research_group(data.research_group_id);
        FC_ASSERT((research_group.funds - data.funds > 0), "Research balance is less than amount (result amount < 0)");

        _account_service.increase_balance(account, data.funds);
        _research_group_service.decrease_research_group_funds(proposal.research_group_id, data.funds);
    }

    void rebalance_research_group_tokens_evaluator(const proposal_object& proposal)
    {
        rebalance_research_group_tokens_data_type data = get_data<rebalance_research_group_tokens_data_type>(proposal);
        _research_group_service.check_research_group_existence(data.research_group_id);

        int size = data.accounts.size();
        for (int i = 0; i < size; ++i)
        {
            _account_service.check_account_existence(data.accounts[i].account_name);
            _research_group_service.increase_research_group_token_amount(data.research_group_id, 
                                                                         data.accounts[i].account_name, 
                                                                         data.accounts[i].amount);
        }
    }

    void create_research_material_evaluator(const proposal_object& proposal)
    {
        create_research_content_data_type data = get_data<create_research_content_data_type>(proposal);

        _research_service.check_research_existence(data.research_id);
        auto &research = _research_service.get_research(data.research_id);
          
        _research_content_service.create(data.research_id, data.type, data.content, data.authors, research.review_share_in_percent, data.research_references, data.research_external_references);
    }

    void start_research_token_sale_evaluator(const proposal_object& proposal) {
        start_research_token_sale_data_type data = get_data<start_research_token_sale_data_type>(proposal);

        _research_service.check_research_existence(data.research_id);
        auto &research = _research_service.get_research(data.research_id);

        FC_ASSERT((research.owned_tokens - data.amount_for_sale >= 0), "Tokens for sale is more than research balance");

        _research_service.decrease_owned_tokens(research, data.amount_for_sale);
        _research_token_sale_service.start_research_token_sale(data.research_id, data.start_time, data.end_time,
                                                               data.amount_for_sale, data.soft_cap, data.hard_cap);
    }

    AccountService& _account_service;
    ProposalService& _proposal_service;
    ResearchGroupService& _research_group_service;
    ResearchService& _research_service;
    ResearchTokenService& _research_token_service;
    ResearchContentService& _research_content_service;
    ResearchTokenSaleService& _research_token_sale_service;
    DynamicGlobalPropertiesService& _dynamic_global_properties_service;

private:
    proposal_evaluators_register evaluators;

    template <typename DataType> DataType get_data(const proposal_object& proposal)
    {
        auto data = fc::json::from_string(proposal.data).as<DataType>();
        data.validate();
        return data;
    }
};

typedef proposal_vote_evaluator_t<dbs_account,
                                  dbs_proposal,
                                  dbs_research_group,
                                  dbs_research,
                                  dbs_research_token,
                                  dbs_research_content,
                                  dbs_research_token_sale,
                                  dbs_dynamic_global_properties>
    proposal_vote_evaluator;
typedef proposal_vote_evaluator_t<dbs_account, dbs_proposal, dbs_research_group, dbs_research, dbs_research_token, dbs_research_content, dbs_research_token_sale, dbs_dynamic_global_properties>
        proposal_vote_evaluator;

} // namespace chain
} // namespace deip
