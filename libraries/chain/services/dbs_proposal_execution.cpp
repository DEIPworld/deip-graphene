#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_asset.hpp>
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
    executions.set(research_group_quorum_action::invite_member,
                   std::bind(&dbs_proposal_execution::invite, this, std::placeholders::_1));
    executions.set(research_group_quorum_action::dropout_member,
                   std::bind(&dbs_proposal_execution::dropout, this, std::placeholders::_1));
    executions.set(research_group_quorum_action::change_research_review_share_percent,
                   std::bind(&dbs_proposal_execution::change_research_review_share, this, std::placeholders::_1));
    executions.set(research_group_quorum_action::change_quorum,
                   std::bind(&dbs_proposal_execution::change_proposal_quorum, this, std::placeholders::_1));
    executions.set(research_group_quorum_action::start_research,
                   std::bind(&dbs_proposal_execution::start_research, this, std::placeholders::_1));
    executions.set(research_group_quorum_action::send_funds,
                   std::bind(&dbs_proposal_execution::send_funds, this, std::placeholders::_1));
    executions.set(research_group_quorum_action::rebalance_research_group_tokens,
                   std::bind(&dbs_proposal_execution::rebalance_research_group_tokens, this, std::placeholders::_1));
    executions.set(research_group_quorum_action::create_research_material,
                   std::bind(&dbs_proposal_execution::create_research_material, this, std::placeholders::_1));
    executions.set(research_group_quorum_action::start_research_token_sale,
                   std::bind(&dbs_proposal_execution::start_research_token_sale, this, std::placeholders::_1));
    executions.set(research_group_quorum_action::offer_research_tokens,
                   std::bind(&dbs_proposal_execution::offer_research_tokens, this, std::placeholders::_1));
    executions.set(research_group_quorum_action::change_research_group_meta,
                   std::bind(&dbs_proposal_execution::change_research_group_meta, this, std::placeholders::_1));
    executions.set(research_group_quorum_action::change_research_meta,
                   std::bind(&dbs_proposal_execution::change_research_meta, this, std::placeholders::_1));
}

const proposal_object& dbs_proposal_execution::execute_proposal(const proposal_object& proposal)
{
  executions.execute(proposal);
  db_impl().modify(proposal, [&](proposal_object& p) {
    p.is_completed = true;
  });
  return proposal;
}

void dbs_proposal_execution::invite(const proposal_object &proposal)
{
    auto& research_group_invite_service = db_impl().obtain_service<dbs_research_group_invite>();
    const auto& research_group_service = db_impl().obtain_service<dbs_research_group>();
    const research_group_object& research_group = research_group_service.get_research_group(proposal.research_group_id);
    const invite_member_proposal_data_type data = get_data<invite_member_proposal_data_type>(proposal);

    if (data.is_head) 
    {
        FC_ASSERT(research_group.is_centralized, 
          "Heads are allowed only in centralized research groups");
    }

    research_group_invite_service.create(data.name, proposal.research_group_id, data.research_group_token_amount_in_percent, data.cover_letter, account_name_type(), data.is_head);
}

void dbs_proposal_execution::dropout(const proposal_object& proposal)
{
    auto& account_service = db_impl().obtain_service<dbs_account>();
    auto& research_group_service = db_impl().obtain_service<dbs_research_group>();
    auto& proposal_service = db_impl().obtain_service<dbs_proposal>();
    auto& research_service = db_impl().obtain_service<dbs_research>();
    auto& research_token_service = db_impl().obtain_service<dbs_research_token>();

    const dropout_member_proposal_data_type data = get_data<dropout_member_proposal_data_type>(proposal);
    const research_group_object& research_group = research_group_service.get_research_group(proposal.research_group_id);

    account_service.check_account_existence(data.name);
    FC_ASSERT(research_group_service.is_research_group_member(data.name, proposal.research_group_id), 
      "${a} is not a member of ${rg} research group",
      ("${a}", data.name)("rg", proposal.research_group_id));

    if (data.name == research_group.creator)
    {
        FC_ASSERT(proposal.creator == research_group.creator || proposal.voted_accounts.count(research_group.creator) != 0,
          "Research group Creator ${c} can not be excluded by member ${m} without his own approval", 
          ("c", research_group.creator)("m", proposal.creator));
    }
    
    if (research_group.heads.count(data.name) != 0) // centralized
    {
        FC_ASSERT(proposal.creator == data.name || proposal.creator == research_group.creator,
          "Research group Head ${h} can not be excluded by member ${m} without his own or research group creator ${c} approval", 
          ("h", data.name)("m", proposal.creator)("c", research_group.creator));
    }

    if (research_group_service.is_organization_division(proposal.research_group_id))
    {
        const auto& division_contract = research_group_service.get_division_contract_by_research_group(proposal.research_group_id);
        if (division_contract.organization_agents.count(data.name) != 0) 
        {
            FC_ASSERT(
              proposal.creator == data.name 
              || proposal.voted_accounts.count(data.name) != 0
              || proposal.creator == research_group.creator 
              || proposal.voted_accounts.count(research_group.creator) != 0,
              "Organization agent ${a} can not be excluded without organization approval", 
              ("a", data.name));

            research_group_service.remove_organization_agent_from_division_contract(division_contract, data.name);
        }
    }

    proposal_service.remove_proposal_votes(data.name, proposal.research_group_id);

    auto& token = research_group_service.get_research_group_token_by_account_and_research_group(data.name, proposal.research_group_id);
    auto researches = research_service.get_researches_by_research_group(proposal.research_group_id);
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
            research_token_service.create_research_token(data.name, tokens_amount_after_dropout_compensation, research.id, true);
        }
    }
    
    const auto& members = research_group_service.remove_member_from_research_group(data.name, proposal.research_group_id);
    FC_ASSERT(members.size() != 0, "The last research group member can not be excluded"); // let's forbid it for now
}

void dbs_proposal_execution::change_research_review_share(const proposal_object& proposal)
{
    auto& research_service = db_impl().obtain_service<dbs_research>();
    auto& dynamic_global_properties_service = db_impl().obtain_service<dbs_dynamic_global_properties>();

    change_research_review_reward_percent_data_type data = get_data<change_research_review_reward_percent_data_type>(proposal);
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
    change_action_quorum_proposal_data_type data = get_data<change_action_quorum_proposal_data_type>(proposal);
    research_group_service.change_quorum(data.quorum, static_cast<deip::protocol::research_group_quorum_action>(data.action), proposal.research_group_id);
}

void dbs_proposal_execution::start_research(const proposal_object& proposal)
{
    auto& research_group_service = db_impl().obtain_service<dbs_research_group>();
    auto& research_service = db_impl().obtain_service<dbs_research>();
    auto& discipline_service = db_impl().obtain_service<dbs_discipline>();
    auto& research_discipline_relation_service = db_impl().obtain_service<dbs_research_discipline_relation>();


    start_research_proposal_data_type data = get_data<start_research_proposal_data_type>(proposal);
    research_group_service.check_research_group_existence(proposal.research_group_id);
    auto& research = research_service.create(data.title, data.abstract, data.permlink, proposal.research_group_id, data.review_share_in_percent, data.dropout_compensation_in_percent, data.is_private);
    for (auto& discipline_id : data.disciplines)
    {
        discipline_service.check_discipline_existence(discipline_id);
        research_discipline_relation_service.create(research.id, discipline_id);
    }
}

void dbs_proposal_execution::send_funds(const proposal_object &proposal)
{
    auto& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    auto& account_service = db_impl().obtain_service<dbs_account>();
    auto& research_group_service = db_impl().obtain_service<dbs_research_group>();

    send_funds_data_type data = get_data<send_funds_data_type>(proposal);
    research_group_service.check_research_group_existence(proposal.research_group_id);
    account_service.check_account_existence(data.recipient);

    auto& account = account_service.get_account(data.recipient);
    auto& research_group = research_group_service.get_research_group(proposal.research_group_id);
    FC_ASSERT((research_group.balance - data.funds >= asset(0, DEIP_SYMBOL)), "Research balance is less than amount (result amount < 0)");

    account_balance_service.adjust_balance(account.name, data.funds);
    research_group_service.decrease_research_group_balance(proposal.research_group_id, data.funds);
}

void dbs_proposal_execution::rebalance_research_group_tokens(const proposal_object& proposal)
{
    auto& research_group_service = db_impl().obtain_service<dbs_research_group>();
    rebalance_research_group_tokens_data_type data = get_data<rebalance_research_group_tokens_data_type>(proposal);
    research_group_service.check_research_group_existence(proposal.research_group_id);

    const std::map<account_name_type, share_type> shares = std::accumulate(
      data.accounts.begin(), data.accounts.end(), std::map<account_name_type, share_type>(),
      [&](std::map<account_name_type, share_type> acc, rebalance_info item) {
          acc[item.account_name] = item.new_amount_in_percent;
          return acc;
      });

    research_group_service.rebalance_research_group_tokens(proposal.research_group_id, shares);
}

void dbs_proposal_execution::create_research_material(const proposal_object& proposal)
{
    auto& research_service = db_impl().obtain_service<dbs_research>();
    auto& research_content_service = db_impl().obtain_service<dbs_research_content>();

    const create_research_content_data_type data = get_data<create_research_content_data_type>(proposal);
    research_service.check_research_existence(data.research_id);
    
    const auto& research = research_service.get_research(data.research_id);
    FC_ASSERT(!research.is_finished, "The research ${r} has been finished already", ("r", research.title));

    const auto& research_content = research_content_service.create_research_content(
      data.research_id, 
      data.type, 
      data.title, 
      data.content, 
      data.permlink, 
      data.authors, 
      data.references, 
      data.external_references);
    
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

    for (auto& reference : data.references)
    {
        auto& _content = research_content_service.get(reference);
        db_impl().push_virtual_operation(research_content_reference_history_operation(research_content.id._id,
                                                                             research_content.research_id._id,
                                                                             fc::to_string(research_content.content),
                                                                             _content.id._id,
                                                                             _content.research_id._id,
                                                                             fc::to_string(_content.content)));
    }

    const auto& old_research_eci = research_service.get_eci_evaluation(research_content.research_id);

    research_content_service.update_eci_evaluation(research_content.id);
    research_service.update_eci_evaluation(research.id);

    const auto& new_research_eci = research_service.get_eci_evaluation(research_content.research_id);
    const auto& new_research_content_eci = research_content_service.get_eci_evaluation(research_content.id);

    for (auto& pair : new_research_eci)
    {
        db_impl().push_virtual_operation(research_eci_history_operation(
            research_content.research_id._id, pair.first._id, pair.second, pair.second - old_research_eci.at(pair.first), 
            3, research_content.id._id, db_impl().head_block_time().sec_since_epoch()));
    }

    for (auto& pair : new_research_content_eci)
    {
        db_impl().push_virtual_operation(research_content_eci_history_operation(
            research_content.id._id, pair.first._id, pair.second, pair.second, 
            3, research_content.id._id, db_impl().head_block_time().sec_since_epoch()));
    }
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

void dbs_proposal_execution::change_research_group_meta(const deip::chain::proposal_object &proposal)
{
    auto& research_group_service = db_impl().obtain_service<dbs_research_group>();

    change_research_group_metadata_type data = get_data<change_research_group_metadata_type>(proposal);

    research_group_service.check_research_group_existence(proposal.research_group_id);

    auto& research_group = research_group_service.get_research_group(proposal.research_group_id);

    db_impl().modify(research_group, [&](research_group_object& rg_o) {
        fc::from_string(rg_o.name, data.research_group_name);
        fc::from_string(rg_o.description, data.research_group_description);
    });
}

void dbs_proposal_execution::change_research_meta(const deip::chain::proposal_object &proposal)
{
    auto& research_group_service = db_impl().obtain_service<dbs_research_group>();
    auto& research_service = db_impl().obtain_service<dbs_research>();

    change_research_metadata_type data = get_data<change_research_metadata_type>(proposal);

    research_group_service.check_research_group_existence(proposal.research_group_id);
    research_service.check_research_existence(data.research_id);

    auto& research = research_service.get_research(data.research_id);

    db_impl().modify(research, [&](research_object& r_o) {
        fc::from_string(r_o.title, data.research_title);
        fc::from_string(r_o.abstract, data.research_abstract);
        r_o.is_private = data.is_private;
    });
}

} //namespace chain
} //namespace deip