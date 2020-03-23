#pragma once

#include "dbs_base_impl.hpp"

#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/deip_object_types.hpp>
#include <deip/chain/schema/research_group_object.hpp>

#include <fc/shared_string.hpp>
#include <fc/fixed_string.hpp>

namespace deip {
namespace chain {

using deip::protocol::research_group_quorum_action;
using deip::protocol::percent_type;

class dbs_research_group : public dbs_base
{

  friend class dbservice_dbs_factory;
  dbs_research_group() = delete;

  protected:
    explicit dbs_research_group(database& db);

  public:
    using research_group_refs_type = std::vector<std::reference_wrapper<const research_group_object>>;
    using research_group_token_refs_type = std::vector<std::reference_wrapper<const research_group_token_object>>;
    using research_group_organization_contract_refs_type = std::vector<std::reference_wrapper<const research_group_organization_contract_object>>;

    /** Get research group by id
     */
    const research_group_object& get_research_group(
      const research_group_id_type& id) const;

    const research_group_object& get_research_group_by_permlink(
      const fc::string& permlink) const;

    research_group_refs_type get_all_research_groups(
      const bool& is_personal_need) const;

    const research_group_object& create_personal_research_group(
      const account_name_type& creator);

    const research_group_object& create_dao_voting_research_group(
      const account_name_type& creator,
      const std::string& name,
      const string& permlink,
      const string& description,
      const int& management_model_v,
      const bool& is_created_by_organization,
      const bool& has_organization,
      const percent_type& default_quorum,
      const std::map<research_group_quorum_action, percent_type>& action_quorums);

    const research_group_object& create_centralized_research_group(
      const account_name_type& creator,
      const std::string& name,
      const string& permlink,
      const string& description,
      const int& management_model_v,
      const bool& is_created_by_organization,
      const bool& has_organization,
      const std::set<account_name_type>& heads);

    void change_quorum(
      const percent_type quorum,
      const research_group_quorum_action quorum_action,
      const research_group_id_type& research_group_id);

    void check_research_group_existence(
      const research_group_id_type& research_group_id) const;

    const bool research_group_exists(const research_group_id_type& research_group_id) const;

    const research_group_token_object& get_research_group_token_by_id(
      const research_group_token_id_type& id) const;

    research_group_token_refs_type get_tokens_by_account(
      const account_name_type &account_name) const;

    research_group_token_refs_type get_research_group_tokens(
      const research_group_id_type& research_group_id) const;

    const research_group_token_object& get_research_group_token_by_account_and_research_group(
      const account_name_type &account,
      const research_group_id_type &research_group_id) const;

    const bool is_research_group_member(
      const account_name_type& account,
      const research_group_id_type& research_group_id) const;

    const research_group_object& increase_research_group_balance(
      const research_group_id_type &research_group_id, 
      const asset &deips);

    const research_group_object& decrease_research_group_balance(
      const research_group_id_type &research_group_id, 
      const asset &deips);

    const research_group_token_object& add_member_to_research_group(
      const account_name_type& account,
      const research_group_id_type& research_group_id,
      const share_type& share,
      const account_name_type& inviter);

    research_group_token_refs_type remove_member_from_research_group(
      const account_name_type& account,
      const research_group_id_type& research_group_id);

    research_group_token_refs_type rebalance_research_group_tokens(
      const research_group_id_type& research_group_id,
      const std::map<account_name_type, share_type> shares);

    const std::set<account_name_type> get_research_group_members(
      const research_group_id_type& id) const;

    const research_group_object& add_research_group_head(
      const account_name_type& head,
      const research_group_object& research_group);

    const research_group_object& remove_research_group_head(
      const account_name_type& head,
      const research_group_object& research_group);

    const research_group_organization_contract_object& get_organizational_contract(
      const research_group_organization_contract_id_type& id) const;

    research_group_organization_contract_refs_type get_organizational_contracts_by_organization(
      const research_group_id_type organization_id) const;

    const research_group_organization_contract_object& get_organizational_contract(
      const research_group_id_type& organization_id,
      const research_group_id_type& research_group_id,
      const research_group_organization_contract_type& type) const;

    const research_group_organization_contract_object& create_organizational_contract(
      const research_group_id_type& organization_id,
      const research_group_id_type& research_group_id,
      const std::set<account_name_type>& organization_agents,
      research_group_organization_contract_type type,
      const bool& unilateral_termination_allowed,
      const std::string& notes);

    const research_group_organization_contract_object& remove_organization_agent_from_division_contract(const research_group_organization_contract_object& contract, const account_name_type& account);

    const research_group_organization_contract_object& get_division_contract_by_research_group(const research_group_id_type& research_group_id) const;

    const bool is_organization_division(const research_group_id_type& research_group_id) const;
};

} // namespace chain
} // namespace deip
