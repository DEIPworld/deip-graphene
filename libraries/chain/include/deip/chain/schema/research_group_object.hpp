#pragma once

#include <deip/protocol/types.hpp>
#include "deip_object_types.hpp"
#include <boost/multi_index/composite_key.hpp>
#include <fc/shared_string.hpp>
#include <fc/fixed_string.hpp>


namespace deip {
namespace chain {

using fc::shared_string;
using deip::protocol::percent_type;

class research_group_object : public object<research_group_object_type, research_group_object>
{
  public:
    template <typename Constructor, typename Allocator>
    research_group_object(Constructor&& c, allocator<Allocator> a)
      : name(a)
      , description(a)
      , permlink(a)
      , heads(a)
    {
      c(*this);
    }

  public:
    research_group_id_type id;

    account_name_type account; // research group account_object
    account_name_type creator; // creator account_object

    shared_string name;
    shared_string description;
    shared_string permlink;
    asset balance = asset(0, DEIP_SYMBOL);
    int32_t management_model_v;

    bool is_personal;

    bool is_dao;
    percent_type default_quorum;

    bool is_centralized;
    account_name_type_set heads;

    bool is_created_by_organization;
    bool has_organization;
};


struct by_permlink;
struct by_account;

typedef multi_index_container<research_group_object,
  indexed_by<
    ordered_unique<
      tag<by_id>, 
        member<
          research_group_object, 
          research_group_id_type, 
          &research_group_object::id>
    >,
    ordered_unique<
      tag<by_account>, 
        member<
          research_group_object, 
          account_name_type, 
          &research_group_object::account>
    >,
    ordered_unique<
      tag<by_permlink>, 
        member<
          research_group_object, 
          shared_string,
          &research_group_object::permlink>
    >
  >,
  allocator<research_group_object>>
  research_group_index;


class research_group_token_object : public object<research_group_token_object_type, research_group_token_object>
{
  public:
    template <typename Constructor, typename Allocator> 
    research_group_token_object(Constructor&& c, allocator<Allocator> a)
    {
      c(*this);
    }

  public:
    research_group_token_id_type id;
    research_group_id_type research_group_id;
    share_type amount;
    account_name_type owner;
};


struct by_research_group;
struct by_owner;
typedef multi_index_container<research_group_token_object,
  indexed_by<
    ordered_unique<
      tag<by_id>, 
        member<
          research_group_token_object, 
          research_group_token_id_type, 
          &research_group_token_object::id
        >
    >,
    ordered_non_unique<
      tag<by_research_group>, 
        member<
          research_group_token_object, 
          research_group_id_type, 
          &research_group_token_object::research_group_id>
    >,
    ordered_unique<
      tag<by_owner>,
        composite_key<research_group_token_object,
          member<
            research_group_token_object,
            account_name_type,
            &research_group_token_object::owner>,
          member<
            research_group_token_object,
            research_group_id_type,
            &research_group_token_object::research_group_id>
        >
    >
  >,
  allocator<research_group_token_object>>
  research_group_token_index;

enum class research_group_organization_contract_type : uint16_t
{
    unknown = 0,
    division = 1,

    FIRST = division,
    LAST = division
};

class research_group_organization_contract_object : public object<research_group_organization_contract_object_type, research_group_organization_contract_object>
{
  public:
    template <typename Constructor, typename Allocator> 
    research_group_organization_contract_object(Constructor&& c, allocator<Allocator> a)
      : organization_agents(a)
      , notes(a)
    {
      c(*this);
    }

  public:
    research_group_organization_contract_id_type id;

    research_group_id_type organization_id;
    research_group_id_type research_group_id;

    account_name_type_set organization_agents;

    uint16_t type;

    bool unilateral_termination_allowed;
    shared_string notes;
};


struct contracts_by_organization;
struct contracts_by_research_group;
struct contracts_by_research_group_and_type;
struct contract_by_organization_and_research_group_and_type;
typedef multi_index_container<research_group_organization_contract_object,
  indexed_by<
    ordered_unique<
      tag<by_id>, 
        member<
          research_group_organization_contract_object, 
          research_group_organization_contract_id_type, 
          &research_group_organization_contract_object::id
        >
    >,
    ordered_non_unique<
      tag<contracts_by_organization>, 
        member<
          research_group_organization_contract_object, 
          research_group_id_type, 
          &research_group_organization_contract_object::organization_id>
    >,
    ordered_non_unique<
      tag<contracts_by_research_group>, 
        member<
          research_group_organization_contract_object, 
          research_group_id_type, 
          &research_group_organization_contract_object::research_group_id>
    >,
    ordered_unique<
      tag<contracts_by_research_group_and_type>,
        composite_key<research_group_organization_contract_object,
          member<
            research_group_organization_contract_object,
            research_group_id_type,
            &research_group_organization_contract_object::research_group_id>,
          member<
            research_group_organization_contract_object,
            uint16_t,
            &research_group_organization_contract_object::type>
        >
    >,
    ordered_unique<
      tag<contract_by_organization_and_research_group_and_type>,
        composite_key<research_group_organization_contract_object,
          member<
            research_group_organization_contract_object,
            research_group_id_type,
            &research_group_organization_contract_object::organization_id>,
          member<
            research_group_organization_contract_object,
            research_group_id_type,
            &research_group_organization_contract_object::research_group_id>,
          member<
            research_group_organization_contract_object,
            uint16_t,
            &research_group_organization_contract_object::type>
        >
    >
  >,
  allocator<research_group_organization_contract_object>>
  research_group_organization_contract_index;

} // namespace chain
} // namespace deip

FC_REFLECT(deip::chain::research_group_object,
  (id)
  (account)
  (creator)
  (name)
  (permlink)
  (description)
  (balance)
  (management_model_v)
  (is_personal)
  (is_dao)
  (default_quorum)
  (is_centralized)
  (heads)
  (is_created_by_organization)
  (has_organization)
)
CHAINBASE_SET_INDEX_TYPE(deip::chain::research_group_object, deip::chain::research_group_index)


FC_REFLECT(deip::chain::research_group_token_object, 
  (id)
  (research_group_id)
  (amount)
  (owner)
)
CHAINBASE_SET_INDEX_TYPE(deip::chain::research_group_token_object, deip::chain::research_group_token_index)

FC_REFLECT_ENUM(deip::chain::research_group_organization_contract_type,
  (unknown)
  (division)
)

FC_REFLECT(deip::chain::research_group_organization_contract_object,
  (id)
  (organization_id)
  (research_group_id)
  (organization_agents)
  (type)
  (unilateral_termination_allowed)
  (notes)
)
CHAINBASE_SET_INDEX_TYPE(deip::chain::research_group_organization_contract_object, deip::chain::research_group_organization_contract_index)

