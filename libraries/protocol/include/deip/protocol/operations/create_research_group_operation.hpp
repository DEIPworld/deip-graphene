#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct invitee_type
{
  invitee_type()
  {
  }

  invitee_type(const account_name_type& a, const percent_type& rgt, const std::string& c)
    : account(a)
    , rgt(rgt)
    , notes(c)
  {
  }

  account_name_type account;
  percent_type rgt;
  std::string notes;

  bool operator<(const invitee_type& other) const
  { 
    return (this->account < other.account); 
  }
};

struct base_research_group_management_model
{
  std::string version;

  base_research_group_management_model(string v = "1.0.0")
  {
    version = v;
  }

  deip::protocol::version get_version() const
  {
    fc::variant v_str(version);
    deip::protocol::version v;
    fc::from_variant(v_str, v);
    return v;
  }
};

struct dao_voting_research_group_management_model_v1_0_0_type : base_research_group_management_model
{
  dao_voting_research_group_management_model_v1_0_0_type(string v = "1.0.0")
    : base_research_group_management_model(v)
  {
  }

  percent_type default_quorum;
  std::map<uint16_t, percent_type> action_quorums;
};

struct dao_multisig_research_group_management_model_v1_0_0_type : base_research_group_management_model
{
  dao_multisig_research_group_management_model_v1_0_0_type(string v = "1.0.0")
    : base_research_group_management_model(v)
  {
  }

  // not implemented yet
  percent_type default_threshold;
  std::map<uint16_t, percent_type> action_thresholds;
};

struct centralized_research_group_management_model_v1_0_0_type : base_research_group_management_model
{
  centralized_research_group_management_model_v1_0_0_type(string v = "1.0.0")
    : base_research_group_management_model(v)
  {
  }

  std::set<account_name_type> heads;
};

struct base_organizational_contract
{
  std::string version;

  base_organizational_contract(string v = "1.0.0")
  {
    version = v;
  }

  deip::protocol::version get_version() const
  {
    fc::variant v_str(version);
    deip::protocol::version v;
    fc::from_variant(v_str, v);
    return v;
  }
};

struct organization_division_contract_v1_0_0_type : base_organizational_contract
{
  organization_division_contract_v1_0_0_type(string v = "1.0.0")
    : base_organizational_contract(v)
  {
  }

  int64_t organization_id;
  bool unilateral_termination_allowed;
  std::set<invitee_type> organization_agents;
  std::string notes;
};

enum class research_group_type : uint32_t
{
  unknown = 0,
  default_research_group = 1,
  grant_application_review_committee = 2,

  FIRST = default_research_group,
  LAST = grant_application_review_committee
};

typedef fc::static_variant<
  dao_voting_research_group_management_model_v1_0_0_type,
  dao_multisig_research_group_management_model_v1_0_0_type,
  centralized_research_group_management_model_v1_0_0_type,
  organization_division_contract_v1_0_0_type
  >
  research_group_details;

struct create_research_group_operation : public base_operation
{
  account_name_type creator;
  std::string name;
  std::string permlink;
  std::string description;
  uint32_t type; // legacy uint32 field is being reused here
  std::vector<research_group_details> details;
  bool is_created_by_organization;
  std::set<invitee_type> invitees;

  void validate() const;

  void get_required_active_authorities(flat_set<account_name_type>& a) const
  {
    a.insert(creator);
  }

  fc::optional<research_group_details> get_management_model() const;

  fc::optional<research_group_details> get_organizational_contract() const;

  bool is_organization_division() const;

  research_group_details get_organization_division_contract() const;
};


}
}

#define DECLARE_RESEARCH_GROUP_DETAILS_TYPE(ResearchGroupDetailsType)                                                  \
    namespace fc {                                                                                                     \
                                                                                                                       \
    void to_variant(const ResearchGroupDetailsType&, fc::variant&);                                                    \
    void from_variant(const fc::variant&, ResearchGroupDetailsType&);                                                  \
                                                                                                                       \
    } /* fc */



namespace fc {
using namespace deip::protocol;

std::string research_group_detail_name_from_type(const std::string& type_name);

struct from_research_group_details_type
{
    variant& var;
    from_research_group_details_type(variant& dv)
        : var(dv)
    {
    }

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
        auto name = research_group_detail_name_from_type(fc::get_typename<T>::name());
        var = variant(std::make_pair(name, v));
    }
};

struct get_research_group_details_type
{
  string& name;
  get_research_group_details_type(string& dv)
    : name(dv)
  {
  }

  typedef void result_type;
  template <typename T> void operator()(const T& v) const
  {
    name = research_group_detail_name_from_type(fc::get_typename<T>::name());
  }
};
} // namespace fc



#define DEFINE_RESEARCH_GROUP_DETAILS_TYPE(ResearchGroupDetailsType)                                                   \
    namespace fc {                                                                                                     \
                                                                                                                       \
    void to_variant(const ResearchGroupDetailsType& var, fc::variant& vo)                                              \
    {                                                                                                                  \
        var.visit(from_research_group_details_type(vo));                                                               \
    }                                                                                                                  \
                                                                                                                       \
    void from_variant(const fc::variant& var, ResearchGroupDetailsType& vo)                                            \
    {                                                                                                                  \
        static std::map<string, uint32_t> to_tag = []() {                                                              \
            std::map<string, uint32_t> name_map;                                                                       \
            for (int i = 0; i < ResearchGroupDetailsType::count(); ++i)                                                \
            {                                                                                                          \
                ResearchGroupDetailsType tmp;                                                                          \
                tmp.set_which(i);                                                                                      \
                string n;                                                                                              \
                tmp.visit(get_research_group_details_type(n));                                                         \
                name_map[n] = i;                                                                                       \
            }                                                                                                          \
            return name_map;                                                                                           \
        }();                                                                                                           \
                                                                                                                       \
        auto ar = var.get_array();                                                                                     \
        if (ar.size() < 2)                                                                                             \
            return;                                                                                                    \
        if (ar[0].is_uint64())                                                                                         \
            vo.set_which(ar[0].as_uint64());                                                                           \
        else                                                                                                           \
        {                                                                                                              \
            auto itr = to_tag.find(ar[0].as_string());                                                                 \
            FC_ASSERT(itr != to_tag.end(), "Invalid research group management type: ${n}", ("n", ar[0]));              \
            vo.set_which(to_tag[ar[0].as_string()]);                                                                   \
        }                                                                                                              \
        vo.visit(fc::to_static_variant(ar[1]));                                                                        \
    }                                                                                                                  \
    } /* fc */

FC_REFLECT( deip::protocol::create_research_group_operation, (creator)(name)(permlink)(description)(type)(details)(is_created_by_organization)(invitees) )
FC_REFLECT( deip::protocol::invitee_type, (account)(rgt)(notes) )
FC_REFLECT_ENUM( deip::protocol::research_group_type, (unknown)(default_research_group)(grant_application_review_committee) )

FC_REFLECT( deip::protocol::base_research_group_management_model, (version) )
FC_REFLECT_DERIVED( deip::protocol::dao_voting_research_group_management_model_v1_0_0_type, (deip::protocol::base_research_group_management_model), (default_quorum)(action_quorums) )
FC_REFLECT_DERIVED( deip::protocol::dao_multisig_research_group_management_model_v1_0_0_type, (deip::protocol::base_research_group_management_model), (default_threshold)(action_thresholds) )
FC_REFLECT_DERIVED( deip::protocol::centralized_research_group_management_model_v1_0_0_type, (deip::protocol::base_research_group_management_model), (heads) )

FC_REFLECT( deip::protocol::base_organizational_contract, (version) )
FC_REFLECT_DERIVED( deip::protocol::organization_division_contract_v1_0_0_type, (deip::protocol::base_organizational_contract), (organization_id)(unilateral_termination_allowed)(organization_agents)(notes) )

DECLARE_RESEARCH_GROUP_DETAILS_TYPE(deip::protocol::research_group_details)
FC_REFLECT_TYPENAME(deip::protocol::research_group_details)