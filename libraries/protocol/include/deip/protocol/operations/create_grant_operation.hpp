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


FC_REFLECT( deip::protocol::create_grant_operation, (external_id)(grantor)(amount)(target_disciplines)(distribution_model)(extensions) )

FC_REFLECT( deip::protocol::announced_application_window_contract_type, (review_committee_id)(min_number_of_positive_reviews)(min_number_of_applications)(max_number_of_research_to_grant)(open_date)(close_date)(additional_info)(extensions) )
FC_REFLECT( deip::protocol::funding_opportunity_announcement_contract_type, (organization_id)(review_committee_id)(treasury_id)(award_ceiling)(award_floor)(expected_number_of_awards)(open_date)(close_date)(officers)(additional_info)(extensions) )
FC_REFLECT( deip::protocol::discipline_supply_announcement_contract_type, (start_time)(end_time)(is_extendable)(content_hash)(additional_info)(extensions) )

DECLARE_STATIC_VARIANT_TYPE(deip::protocol::grant_distribution_models)
FC_REFLECT_TYPENAME(deip::protocol::grant_distribution_models)