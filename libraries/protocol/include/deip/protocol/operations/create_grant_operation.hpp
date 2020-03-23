#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <deip/protocol/asset.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

using deip::protocol::asset;

struct base_grant_contract
{
  std::string version;

  base_grant_contract(string v = "1.0.0")
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

struct announced_application_window_contract_v1_0_0_type : base_grant_contract
{
  announced_application_window_contract_v1_0_0_type(string v = "1.0.0")
    : base_grant_contract(v)
  {
  }

  int64_t review_committee_id;

  uint16_t min_number_of_positive_reviews;
  uint16_t min_number_of_applications;
  uint16_t max_number_of_research_to_grant;
  
  fc::time_point_sec start_date;
  fc::time_point_sec end_date;
};

struct funding_opportunity_announcement_contract_v1_0_0_type : base_grant_contract
{
  funding_opportunity_announcement_contract_v1_0_0_type(string v = "1.0.0")
    : base_grant_contract(v)
  {
  }

    int64_t organization_id;
    int64_t review_committee_id;
    
    string funding_opportunity_number;

    asset award_ceiling;
    asset award_floor;

    uint16_t expected_number_of_awards;

    fc::time_point_sec open_date;
    fc::time_point_sec close_date;

    std::set<account_name_type> officers;

    flat_map<string, string> additional_info;
};

enum class grant_contract_type : uint16_t
{
  unknown = 0,
  announced_application_window = 1,
  funding_opportunity_announcement = 2,

  FIRST = announced_application_window,
  LAST = funding_opportunity_announcement
};


typedef fc::static_variant<
  announced_application_window_contract_v1_0_0_type,
  funding_opportunity_announcement_contract_v1_0_0_type
  >
  grant_contract_details;


struct create_grant_operation : public base_operation
{
    account_name_type grantor;
    asset amount;
    uint16_t type;
    std::set<int64_t> target_disciplines;
    std::vector<grant_contract_details> details;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(grantor);
    }

    fc::optional<grant_contract_details> get_grant_contract() const;
};


}
}

#define DECLARE_GRANT_CONTRACT_DETAILS_TYPE(GrantContractDetailsType)                                                  \
    namespace fc {                                                                                                     \
                                                                                                                       \
    void to_variant(const GrantContractDetailsType&, fc::variant&);                                                    \
    void from_variant(const fc::variant&, GrantContractDetailsType&);                                                  \
                                                                                                                       \
    } /* fc */


namespace fc {
using namespace deip::protocol;

std::string grant_contract_detail_name_from_type(const std::string& type_name);

struct from_grant_contract_details_type
{
    variant& var;
    from_grant_contract_details_type(variant& dv)
        : var(dv)
    {
    }

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
        auto name = grant_contract_detail_name_from_type(fc::get_typename<T>::name());
        var = variant(std::make_pair(name, v));
    }
};

struct get_grant_contract_details_type
{
  string& name;
  get_grant_contract_details_type(string& dv)
    : name(dv)
  {
  }

  typedef void result_type;
  template <typename T> void operator()(const T& v) const
  {
    name = grant_contract_detail_name_from_type(fc::get_typename<T>::name());
  }
};
} // namespace fc



#define DEFINE_GRANT_CONTRACT_DETAILS_TYPE(GrantContractDetailsType)                                                   \
    namespace fc {                                                                                                     \
                                                                                                                       \
    void to_variant(const GrantContractDetailsType& var, fc::variant& vo)                                              \
    {                                                                                                                  \
        var.visit(from_grant_contract_details_type(vo));                                                               \
    }                                                                                                                  \
                                                                                                                       \
    void from_variant(const fc::variant& var, GrantContractDetailsType& vo)                                            \
    {                                                                                                                  \
        static std::map<string, uint32_t> to_tag = []() {                                                              \
            std::map<string, uint32_t> name_map;                                                                       \
            for (int i = 0; i < GrantContractDetailsType::count(); ++i)                                                \
            {                                                                                                          \
                GrantContractDetailsType tmp;                                                                          \
                tmp.set_which(i);                                                                                      \
                string n;                                                                                              \
                tmp.visit(get_grant_contract_details_type(n));                                                         \
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



FC_REFLECT( deip::protocol::create_grant_operation, (grantor)(amount)(type)(target_disciplines)(details) )
FC_REFLECT_ENUM( deip::protocol::grant_contract_type, (unknown)(announced_application_window)(funding_opportunity_announcement) )

FC_REFLECT( deip::protocol::base_grant_contract, (version) )
FC_REFLECT_DERIVED( deip::protocol::announced_application_window_contract_v1_0_0_type, (deip::protocol::base_grant_contract), (review_committee_id)(min_number_of_positive_reviews)(min_number_of_applications)(max_number_of_research_to_grant)(start_date)(end_date) )
FC_REFLECT_DERIVED( deip::protocol::funding_opportunity_announcement_contract_v1_0_0_type, (deip::protocol::base_grant_contract), (organization_id)(review_committee_id)(funding_opportunity_number)(award_ceiling)(award_floor)(expected_number_of_awards)(open_date)(close_date)(officers)(additional_info) )


DECLARE_GRANT_CONTRACT_DETAILS_TYPE(deip::protocol::grant_contract_details)
FC_REFLECT_TYPENAME(deip::protocol::grant_contract_details)