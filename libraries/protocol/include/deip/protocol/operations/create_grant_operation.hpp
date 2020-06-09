#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/version.hpp>
#include <deip/protocol/asset.hpp>

namespace deip {
namespace protocol {

using deip::protocol::asset;

struct announced_application_window_contract_type
{
    external_id_type review_committee_id;
    uint16_t min_number_of_positive_reviews;
    uint16_t min_number_of_applications;
    uint16_t max_number_of_research_to_grant;
    fc::time_point_sec open_date;
    fc::time_point_sec close_date;
    flat_map<string, string> additional_info;

    extensions_type extensions;
};

struct funding_opportunity_announcement_contract_type
{
    external_id_type organization_id;
    external_id_type review_committee_id;
    external_id_type treasury_id;
    asset award_ceiling;
    asset award_floor;
    uint16_t expected_number_of_awards;
    fc::time_point_sec open_date;
    fc::time_point_sec close_date;
    std::set<account_name_type> officers;
    flat_map<string, string> additional_info;

    extensions_type extensions;
};

struct discipline_supply_announcement_contract_type
{
    fc::time_point_sec start_time;
    fc::time_point_sec end_time;
    bool is_extendable;
    string content_hash;
    flat_map<string, string> additional_info;

    extensions_type extensions;
};

typedef fc::static_variant<
  announced_application_window_contract_type,
  funding_opportunity_announcement_contract_type,
  discipline_supply_announcement_contract_type
  >
  grant_distribution_models;


struct create_grant_operation : public base_operation
{
    external_id_type external_id;
    account_name_type grantor;
    asset amount;
    flat_set<external_id_type> target_disciplines;
    grant_distribution_models distribution_model;
    
    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(grantor);
    }
};


}
}

#define DECLARE_GRANT_DISTRIBUTION_MODELS_TYPE(GrantDistributionModelsType)                                            \
    namespace fc {                                                                                                     \
                                                                                                                       \
    void to_variant(const GrantDistributionModelsType&, fc::variant&);                                                 \
    void from_variant(const fc::variant&, GrantDistributionModelsType&);                                               \
                                                                                                                       \
    } /* fc */

    namespace fc {
using namespace deip::protocol;

std::string grant_distribution_model_name_from_type(const std::string& type_name);

struct from_grant_distribution_models_type
{
    variant& var;
    from_grant_distribution_models_type(variant& dv)
        : var(dv)
    {
    }

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
        auto name = grant_distribution_model_name_from_type(fc::get_typename<T>::name());
        var = variant(std::make_pair(name, v));
    }
};

struct get_grant_distribution_models_type
{
  string& name;
  get_grant_distribution_models_type(string& dv)
    : name(dv)
  {
  }

  typedef void result_type;
  template <typename T> void operator()(const T& v) const
  {
      name = grant_distribution_model_name_from_type(fc::get_typename<T>::name());
  }
};
} // namespace fc

#define DEFINE_GRANT_DISTRIBUTION_MODELS_TYPE(GrantDistributionModelsType)                                             \
    namespace fc {                                                                                                     \
                                                                                                                       \
    void to_variant(const GrantDistributionModelsType& var, fc::variant& vo)                                           \
    {                                                                                                                  \
        var.visit(from_grant_distribution_models_type(vo));                                                            \
    }                                                                                                                  \
                                                                                                                       \
    void from_variant(const fc::variant& var, GrantDistributionModelsType& vo)                                         \
    {                                                                                                                  \
        static std::map<string, uint32_t> to_tag = []() {                                                              \
            std::map<string, uint32_t> name_map;                                                                       \
            for (int i = 0; i < GrantDistributionModelsType::count(); ++i)                                             \
            {                                                                                                          \
                GrantDistributionModelsType tmp;                                                                       \
                tmp.set_which(i);                                                                                      \
                string n;                                                                                              \
                tmp.visit(get_grant_distribution_models_type(n));                                                      \
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
            FC_ASSERT(itr != to_tag.end(), "Invalid account trait type: ${n}", ("n", ar[0]));                          \
            vo.set_which(to_tag[ar[0].as_string()]);                                                                   \
        }                                                                                                              \
        vo.visit(fc::to_static_variant(ar[1]));                                                                        \
    }                                                                                                                  \
    } /* fc */

FC_REFLECT( deip::protocol::create_grant_operation, (external_id)(grantor)(amount)(target_disciplines)(distribution_model)(extensions) )

FC_REFLECT( deip::protocol::announced_application_window_contract_type, (review_committee_id)(min_number_of_positive_reviews)(min_number_of_applications)(max_number_of_research_to_grant)(open_date)(close_date)(additional_info)(extensions) )
FC_REFLECT( deip::protocol::funding_opportunity_announcement_contract_type, (organization_id)(review_committee_id)(treasury_id)(award_ceiling)(award_floor)(expected_number_of_awards)(open_date)(close_date)(officers)(additional_info)(extensions) )
FC_REFLECT( deip::protocol::discipline_supply_announcement_contract_type, (start_time)(end_time)(is_extendable)(content_hash)(additional_info)(extensions) )

DECLARE_GRANT_DISTRIBUTION_MODELS_TYPE(deip::protocol::grant_distribution_models)
FC_REFLECT_TYPENAME(deip::protocol::grant_distribution_models)