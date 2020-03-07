#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void inline validate_research_group_permitted_details(
  const uint32_t& kind, 
  const std::vector<research_group_details>& details)
{
  bool is_validated = false;

  // TODO: change op.details type to set
  std::set<research_group_details> distinct;
  distinct.insert(details.begin(), details.end());
  FC_ASSERT(distinct.size() == details.size(), "Duplicated research group details specified");

  const research_group_type type = static_cast<research_group_type>(kind);
  FC_ASSERT(type != research_group_type::unknown, "Research group type is required");
  FC_ASSERT(type >= research_group_type::FIRST && type <= research_group_type::LAST, 
    "Provided enum value is outside of the range: val = ${enum_val}, first = ${first}, last = ${last}",
    ("enum_val", type)("first", research_group_type::FIRST)("last", research_group_type::LAST));

  if (type == research_group_type::default_research_group)
  {
    const std::set<int>& permitted_details = 
    {
      research_group_details::tag<dao_voting_research_group_management_model_v1_0_0_type>::value,
      research_group_details::tag<dao_multisig_research_group_management_model_v1_0_0_type>::value,
      research_group_details::tag<centralized_research_group_management_model_v1_0_0_type>::value,
      research_group_details::tag<organization_division_contract_v1_0_0_type>::value
    };

    const bool is_permitted = std::count_if(details.begin(), details.end(),
      [&](const research_group_details& detail) {
          return permitted_details.count(detail.which()) != 0;
      }) == details.size();

    FC_ASSERT(is_permitted, 
      "Provided research group details types are not permitted for ${type} type",
      ("type", type));

    is_validated = true;
  }

  else if (type == research_group_type::grant_application_review_committee)
  {
      FC_ASSERT(false, "Grant application review committee is not implemented yet");
      is_validated = true;
  }

  FC_ASSERT(is_validated, "Research group details are not validated");
}

void inline validate_research_group_management_model(
  const research_group_details management_model, 
  const account_name_type& creator, 
  const std::set<invitee_type>& invitees)
{  
  bool is_validated = false;

  if (management_model.which() == research_group_details::tag<dao_voting_research_group_management_model_v1_0_0_type>::value) 
  {
    const auto dao_voting_model = management_model.get<dao_voting_research_group_management_model_v1_0_0_type>();
    FC_ASSERT(dao_voting_model.default_quorum >= DEIP_1_PERCENT && dao_voting_model.default_quorum <= DEIP_100_PERCENT, "Default quorum is required for DAO voting model and needs to be in range of 1-100%");

    for (auto& quorum : dao_voting_model.action_quorums)
    {
      FC_ASSERT(quorum.first >= FIRST_ACTION_QUORUM_TYPE && quorum.first <= LAST_ACTION_QUORUM_TYPE,
        "Unknown research group proposal type");
      FC_ASSERT(quorum.second >= DEIP_1_PERCENT && quorum.second <= DEIP_100_PERCENT,
        "Default quorum is required for DAO voting model and needs to be in range of 1-100%");
    }

    percent_type total_invitee_influence_share = percent_type(0);
    const percent_type min_invitee_influence_share = percent_type(DEIP_1_PERCENT);
    const percent_type max_invitee_influence_share = percent_type(95 * DEIP_1_PERCENT); // we need to preserve some influence for the creator

    for (auto& invitee : invitees) 
    {
      validate_account_name(invitee.account);
      FC_ASSERT(invitee.rgt >= min_invitee_influence_share && invitee.rgt <= max_invitee_influence_share, "Invitee should have influence share in range of 1-100%");
      total_invitee_influence_share += invitee.rgt;
    }
    FC_ASSERT(total_invitee_influence_share <= max_invitee_influence_share);
    is_validated = true;
  }

  else if (management_model.which() == research_group_details::tag<dao_multisig_research_group_management_model_v1_0_0_type>::value) 
  {
    const auto dao_multisig_model = management_model.get<dao_multisig_research_group_management_model_v1_0_0_type>();
    // TODO: define requirements
    FC_ASSERT(false, "Multisignature group management model is not supported currently");
    is_validated = true;
  }

  else if (management_model.which() == research_group_details::tag<centralized_research_group_management_model_v1_0_0_type>::value) 
  {
    const auto centralized_model = management_model.get<centralized_research_group_management_model_v1_0_0_type>();
    FC_ASSERT(centralized_model.heads.size() != 0, "Centralized group management model requires at least 1 decision maker");
    FC_ASSERT(centralized_model.heads.count(creator) != 0,
      "Research group heads should include the group creator ${c}", 
      ("c", creator));

    for (auto& head : centralized_model.heads)
    {
      validate_account_name(head);
    }

    for (auto& invitee : invitees) 
    {
      validate_account_name(invitee.account);
    }

    is_validated = true;
  }

  FC_ASSERT(is_validated, "Research group management model is not validated");
}

fc::optional<research_group_details> create_research_group_operation::get_management_model() const
{
  fc::optional<research_group_details> management_model;
  auto itr = std::find_if(details.begin(), details.end(),
    [&](const research_group_details& detail) {
      return 
        detail.which() == research_group_details::tag<dao_voting_research_group_management_model_v1_0_0_type>::value || 
        detail.which() == research_group_details::tag<dao_multisig_research_group_management_model_v1_0_0_type>::value ||
        detail.which() == research_group_details::tag<centralized_research_group_management_model_v1_0_0_type>::value;      
  });

  if (itr != details.end())
  {
    management_model = *itr;
  }

  return management_model;
}

void inline validate_organizational_contract(
  const research_group_details organizational_contract, 
  const bool& is_created_by_organization,
  const account_name_type& creator,
  const std::set<invitee_type>& invitees)
{
    bool is_validated = false;
    if (organizational_contract.which() == research_group_details::tag<organization_division_contract_v1_0_0_type>::value)
    {
        FC_ASSERT(is_created_by_organization, "Division can be created by organization only");
        const auto division_contract = organizational_contract.get<organization_division_contract_v1_0_0_type>();
        FC_ASSERT(division_contract.organization_agents.size() != 0, "Division requires at least 1 agent from organization");

        for (auto& agent : division_contract.organization_agents)
        {
            validate_account_name(agent.account);
        }

        std::set<account_name_type> organization_agents = std::accumulate(
          division_contract.organization_agents.begin(), division_contract.organization_agents.end(), std::set<account_name_type>(),
          [&](std::set<account_name_type> acc, const invitee_type& agent) {
              acc.insert(agent.account);
              return acc;
          });

        std::set<account_name_type> invites = std::accumulate(
          invitees.begin(), invitees.end(), std::set<account_name_type>(),
          [&](std::set<account_name_type> acc, const invitee_type& invitee) {
              acc.insert(invitee.account);
              return acc;
        });

        FC_ASSERT(organization_agents.count(creator) != 0, 
          "Research group creator ${c} should be specified in organization agents list.",
          ("c", creator));

        FC_ASSERT(std::none_of(organization_agents.begin(), organization_agents.end(), 
          [&](const account_name_type& agent) {
            return invites.count(agent) != 0;
        }), "Organization agents should not be specified in invitees list.");

        is_validated = true;
    }

    FC_ASSERT(is_validated, "Research group organizational contract is not validated");
}

fc::optional<research_group_details> create_research_group_operation::get_organizational_contract() const
{
    fc::optional<research_group_details> organizational_contract;

    auto itr = std::find_if(details.begin(), details.end(),
      [&](const research_group_details& detail) {
        return detail.which() == research_group_details::tag<organization_division_contract_v1_0_0_type>::value;    
    });

    if (itr != details.end()) 
    {
        organizational_contract = *itr;
    }

    return organizational_contract;
}

bool create_research_group_operation::is_organization_division() const
{
    return std::count_if(details.begin(), details.end(), 
      [&](const research_group_details& detail) {
        return detail.which() == research_group_details::tag<organization_division_contract_v1_0_0_type>::value;
      }) != 0;
}

research_group_details create_research_group_operation::get_organization_division_contract() const
{
     auto itr = std::find_if(details.begin(), details.end(), 
      [&](const research_group_details& detail) {
        return detail.which() == research_group_details::tag<organization_division_contract_v1_0_0_type>::value;
      });

     FC_ASSERT(itr != details.end(), "Division contract is not found");
     return *itr;
}

void create_research_group_operation::validate() const
{
  validate_account_name(creator);
  validate_permlink(permlink);
  FC_ASSERT(name.size() > 0, "Research group name is required");
  FC_ASSERT(fc::is_utf8(name), "Research group name is not valid UTF-8 string");
  FC_ASSERT(fc::is_utf8(description), "Research group description is not valid UTF-8 string");
  FC_ASSERT(details.size() != 0, "Research group details are required");
  validate_research_group_permitted_details(type, details);

  FC_ASSERT(std::none_of(invitees.begin(), invitees.end(),
    [&](const invitee_type& invitee) { return invitee.account == creator; }), 
    "Research group creator should not be specified in invitees list.",
    ("c", creator));

  const auto management_model_opt = get_management_model();
  FC_ASSERT(management_model_opt.valid(), "Research group management model is required");
  validate_research_group_management_model(*management_model_opt, creator, invitees);
  
  if (is_created_by_organization)
  {
      FC_ASSERT(is_organization_division(), "Division contract is required");
  }

  const auto organizational_contract_opt = get_organizational_contract();
  if (organizational_contract_opt.valid())
  {
      validate_organizational_contract(*organizational_contract_opt, is_created_by_organization, creator, invitees);
  }
}


} /* deip::protocol */
} /* protocol */

namespace fc {

  std::string research_group_detail_name_from_type(const std::string& type_name)
  {
    auto start = type_name.find_last_of(':') + 1;
    auto end = type_name.find_last_of('_');
    auto result = type_name.substr(start, end - start);
    return result;
  }

}


DEFINE_RESEARCH_GROUP_DETAILS_TYPE(deip::protocol::research_group_details)