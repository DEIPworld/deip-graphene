#include <deip/chain/services/dbs_proposal_execution.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_research_group_invite.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_offer_research_tokens.hpp>

namespace deip {
namespace chain {

dbs_proposal_execution::dbs_proposal_execution(database &db)
    : _base_type(db)
{
    executions.set(proposal_action_type::invite_member,
                   std::bind(&dbs_proposal_execution::invite, this, std::placeholders::_1));
    executions.set(proposal_action_type::dropout_member,
                   std::bind(&dbs_proposal_execution::dropout, this, std::placeholders::_1));
    executions.set(proposal_action_type::change_research_review_share_percent,
                   std::bind(&dbs_proposal_execution::change_research_review_share, this, std::placeholders::_1));
    executions.set(proposal_action_type::change_quorum,
                   std::bind(&dbs_proposal_execution::change_proposal_quorum, this, std::placeholders::_1));
    executions.set(proposal_action_type::start_research,
                   std::bind(&dbs_proposal_execution::start_research, this, std::placeholders::_1));
    executions.set(proposal_action_type::send_funds,
                   std::bind(&dbs_proposal_execution::send_funds, this, std::placeholders::_1));
    executions.set(proposal_action_type::rebalance_research_group_tokens,
                   std::bind(&dbs_proposal_execution::rebalance_research_group_tokens, this, std::placeholders::_1));
    executions.set(proposal_action_type::create_research_material,
                   std::bind(&dbs_proposal_execution::create_research_material, this, std::placeholders::_1));
    executions.set(proposal_action_type::start_research_token_sale,
                   std::bind(&dbs_proposal_execution::start_research_token_sale, this, std::placeholders::_1));
    executions.set(proposal_action_type::offer_research_tokens,
                   std::bind(&dbs_proposal_execution::offer_research_tokens, this, std::placeholders::_1));
}

void dbs_proposal_execution::invite(const proposal_object &proposal)
{
    auto& research_group_invite_service = db_impl().obtain_service<dbs_research_group_invite>();

    invite_member_proposal_data_type data = get_data<invite_member_proposal_data_type>(proposal);
    research_group_invite_service.create(data.name, data.research_group_id, data.research_group_token_amount_in_percent, data.cover_letter, account_name_type());
}

void dbs_proposal_execution::dropout(const proposal_object& proposal)
{
    auto& account_service = db_impl().obtain_service<dbs_account>();
    auto& research_group_service = db_impl().obtain_service<dbs_research_group>();
    auto& proposal_service = db_impl().obtain_service<dbs_proposal>();
    auto& research_service = db_impl().obtain_service<dbs_research>();
    auto& research_token_service = db_impl().obtain_service<dbs_research_token>();

    dropout_member_proposal_data_type data = get_data<dropout_member_proposal_data_type>(proposal);

    account_service.check_account_existence(data.name);
    research_group_service.check_research_group_token_existence(data.name, data.research_group_id);

    proposal_service.remove_proposal_votes(data.name, data.research_group_id);

    auto& token = research_group_service.get_token_by_account_and_research_group(data.name, data.research_group_id);

    auto researches = research_service.get_researches_by_research_group(data.research_group_id);
    for (auto& r : researches)
    {
        auto& research = r.get();

        auto tokens_amount_in_percent_after_dropout_compensation
                = token.amount * research.dropout_compensation_in_percent / DEIP_100_PERCENT;
        auto tokens_amount_after_dropout_compensation
                = research.owned_tokens * tokens_amount_in_percent_after_dropout_compensation / DEIP_100_PERCENT;

        research_service.decrease_owned_tokens(research, tokens_amount_after_dropout_compensation);

        if (research_token_service.exists_by_owner_and_research(data.name, research.id))
        {
            auto& research_token = research_token_service.get_by_owner_and_research(data.name, research.id);
            research_token_service.increase_research_token_amount(research_token, tokens_amount_after_dropout_compensation);
        }
        else
        {
            research_token_service.create_research_token(data.name, tokens_amount_after_dropout_compensation, research.id);
        }
    }
    share_type token_amount = token.amount;
    research_group_service.remove_token(data.name, data.research_group_id);
    research_group_service.increase_research_group_tokens_amount(data.research_group_id, token_amount);
}

void dbs_proposal_execution::change_research_review_share(const proposal_object& proposal)
{
    auto& research_service = db_impl().obtain_service<dbs_research>();
    auto& dynamic_global_properties_service = db_impl().obtain_service<dbs_dynamic_global_properties>();

    change_research_review_share_percent_data_type data = get_data<change_research_review_share_percent_data_type>(proposal);
    auto& research = research_service.get_research(data.research_id);

    int64_t time_period_from_last_update
            = (dynamic_global_properties_service.get_dynamic_global_properties().time - research.review_share_in_percent_last_update).to_seconds();
    FC_ASSERT((time_period_from_last_update >= DAYS_TO_SECONDS(90)),
              "Cannot update review_share (time period from last update < 90)");

    research_service.change_research_review_share_percent(data.research_id, data.review_share_in_percent);
}

void dbs_proposal_execution::change_proposal_quorum(const proposal_object &proposal)
{
    auto& research_group_service = db_impl().obtain_service<dbs_research_group>();

    change_quorum_proposal_data_type data = get_data<change_quorum_proposal_data_type>(proposal);
    research_group_service.change_quorum(data.quorum_percent, static_cast<deip::protocol::proposal_action_type>(data.proposal_type), data.research_group_id);
}

void dbs_proposal_execution::start_research(const proposal_object& proposal)
{
    auto& research_group_service = db_impl().obtain_service<dbs_research_group>();
    auto& research_service = db_impl().obtain_service<dbs_research>();
    auto& discipline_service = db_impl().obtain_service<dbs_discipline>();
    auto& research_discipline_relation_service = db_impl().obtain_service<dbs_research_discipline_relation>();


    start_research_proposal_data_type data = get_data<start_research_proposal_data_type>(proposal);
    research_group_service.check_research_group_existence(data.research_group_id);
    auto& research = research_service.create(data.title, data.abstract, data.permlink, data.research_group_id, data.review_share_in_percent, data.dropout_compensation_in_percent);
    for (auto& discipline_id : data.disciplines)
    {
        discipline_service.check_discipline_existence(discipline_id);
        research_discipline_relation_service.create(research.id, discipline_id);
    }
}

void dbs_proposal_execution::send_funds(const proposal_object &proposal)
{
    auto& account_service = db_impl().obtain_service<dbs_account>();
    auto& research_group_service = db_impl().obtain_service<dbs_research_group>();

    send_funds_data_type data = get_data<send_funds_data_type>(proposal);
    research_group_service.check_research_group_existence(data.research_group_id);
    account_service.check_account_existence(data.recipient);

    auto& account = account_service.get_account(data.recipient);
    auto& research_group = research_group_service.get_research_group(data.research_group_id);
    FC_ASSERT((research_group.balance.amount - data.funds.amount >= 0), "Research balance is less than amount (result amount < 0)");

    account_service.adjust_balance(account, data.funds);
    research_group_service.decrease_balance(proposal.research_group_id, data.funds);
}

void dbs_proposal_execution::rebalance_research_group_tokens(const proposal_object& proposal)
{
    auto& research_group_service = db_impl().obtain_service<dbs_research_group>();

    rebalance_research_group_tokens_data_type data = get_data<rebalance_research_group_tokens_data_type>(proposal);
    research_group_service.check_research_group_existence(data.research_group_id);

    FC_ASSERT(data.accounts.size() == research_group_service.get_research_group_tokens(data.research_group_id).size(),
              "New amount of tokens should be provided for every research group member");
    for (auto account : data.accounts)
        research_group_service.check_research_group_token_existence(account.account_name, data.research_group_id);

    for (auto account : data.accounts)
        research_group_service.set_new_research_group_token_amount(data.research_group_id,
                                                                   account.account_name,
                                                                   account.new_amount_in_percent);
}

void dbs_proposal_execution::create_research_material(const proposal_object& proposal)
{
    auto& research_service = db_impl().obtain_service<dbs_research>();
    auto& research_content_service = db_impl().obtain_service<dbs_research_content>();

    const create_research_content_data_type data = get_data<create_research_content_data_type>(proposal);
    research_service.check_research_existence(data.research_id);
    
    const auto& research = research_service.get_research(data.research_id);
    FC_ASSERT(!research.is_finished, "The research ${r} has been finished already", ("r", research.id));

    const auto& research_content = research_content_service.create(data.research_id, data.type, data.title, data.content, data.permlink, data.authors, data.references, data.external_references);

    for (auto& reference : data.references)
    {
        auto& _content = research_content_service.get(reference);
        db_impl().push_virtual_operation(content_reference_history_operation(research_content.id._id,
                                                                             research_content.research_id._id,
                                                                             fc::to_string(research_content.content),
                                                                             _content.id._id,
                                                                             _content.research_id._id,
                                                                             fc::to_string(_content.content)));
    }

    db_impl().modify(research, [&](research_object& r_o) {
        for (auto author : data.authors)
        {
            r_o.members.insert(author);
        }

        if (research_content.type == research_content_type::final_result)
        {
            r_o.is_finished = true;
        }
    });

    research_content_service.update_eci_evaluation(research_content.id);
    research_service.update_eci_evaluation(research.id);
}

void dbs_proposal_execution::start_research_token_sale(const proposal_object& proposal)
{
    auto& research_service = db_impl().obtain_service<dbs_research>();
    auto& research_token_sale_service = db_impl().obtain_service<dbs_research_token_sale>();

    start_research_token_sale_data_type data = get_data<start_research_token_sale_data_type>(proposal);

    research_service.check_research_existence(data.research_id);
    auto &research = research_service.get_research(data.research_id);

    auto research_token_sales = research_token_sale_service.get_by_research_id_and_status(data.research_id, research_token_sale_status::token_sale_active);

    FC_ASSERT(research_token_sales.size() == 0, "Another token sale in progress.");
    FC_ASSERT(data.start_time >= db_impl().head_block_time());
    FC_ASSERT((research.owned_tokens - data.amount_for_sale >= 0), "Tokens for sale is more than research balance");

    research_service.decrease_owned_tokens(research, data.amount_for_sale);
    research_token_sale_service.start(data.research_id, data.start_time, data.end_time,
                                      data.amount_for_sale, data.soft_cap, data.hard_cap);
}

void dbs_proposal_execution::offer_research_tokens(const deip::chain::proposal_object &proposal)
{
    auto& research_service = db_impl().obtain_service<dbs_research>();
    auto& research_token_service = db_impl().obtain_service<dbs_research_token>();
    auto& offer_research_tokens_service = db_impl().obtain_service<dbs_offer_research_tokens>();

    offer_research_tokens_data_type data = get_data<offer_research_tokens_data_type>(proposal);

    research_service.check_research_existence(data.research_id);
    FC_ASSERT(research_token_service.exists_by_owner_and_research(data.receiver, data.research_id) == false, "You cannot offer research tokens to your groupmate");

    auto &research = research_service.get_research(data.research_id);

    FC_ASSERT(research.owned_tokens >= data.amount, "Research group doesn't have enough owned tokens");

    offer_research_tokens_service.create(data.sender,
                                         data.receiver,
                                         data.research_id,
                                         data.amount,
                                         data.price);
}

} //namespace chain
} //namespace deip