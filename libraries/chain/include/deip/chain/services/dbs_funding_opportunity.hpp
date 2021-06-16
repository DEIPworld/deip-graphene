#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/funding_opportunity_object.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace chain {

class dbs_funding_opportunity : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_funding_opportunity() = delete;

protected:
    explicit dbs_funding_opportunity(database &db);

public:
    using funding_opportunity_refs_type = std::vector<std::reference_wrapper<const funding_opportunity_object>>;

    const funding_opportunity_object& create_grant_with_officer_evaluation_distribution(const account_id_type& organization_id,
                                                                                        const external_id_type& organization_external_id,
                                                                                        const account_id_type& review_committee_id,
                                                                                        const external_id_type& review_committee_external_id,
                                                                                        const account_id_type& treasury_id,
                                                                                        const external_id_type& treasury_external_id,
                                                                                        const account_name_type& grantor,
                                                                                        const external_id_type& funding_opportunity_number,
                                                                                        const flat_map<string, string>& additional_info,
                                                                                        const std::set<discipline_id_type>& target_disciplines,
                                                                                        const asset& amount,
                                                                                        const asset& award_ceiling,
                                                                                        const asset& award_floor,
                                                                                        const uint16_t& expected_number_of_awards,
                                                                                        const std::set<account_name_type>& officers,
                                                                                        const fc::time_point_sec& open_date,
                                                                                        const fc::time_point_sec& close_date);

    const funding_opportunity_object& create_grant_with_eci_evaluation_distribution(const account_name_type& grantor,
                                                                                    const asset& amount,
                                                                                    const std::set<discipline_id_type>& target_disciplines,
                                                                                    const external_id_type& funding_opportunity_number,
                                                                                    const flat_map<string, string>& additional_info,
                                                                                    const account_id_type& review_committee_id,
                                                                                    const external_id_type& review_committee_external_id,
                                                                                    const uint16_t& min_number_of_positive_reviews,
                                                                                    const uint16_t& min_number_of_applications,
                                                                                    const uint16_t& max_number_of_research_to_grant,
                                                                                    const fc::time_point_sec& open_date,
                                                                                    const fc::time_point_sec& close_date);

    const funding_opportunity_object& get_funding_opportunity(const funding_opportunity_id_type& id) const;
    
    const funding_opportunity_object& get_funding_opportunity(const external_id_type& funding_opportunity_number) const;

    const fc::optional<std::reference_wrapper<const funding_opportunity_object>> get_funding_opportunity_announcement_if_exists(const funding_opportunity_id_type& id) const;

    const fc::optional<std::reference_wrapper<const funding_opportunity_object>> get_funding_opportunity_announcement_if_exists(const external_id_type& opportunity_number) const;

    const bool funding_opportunity_exists(const funding_opportunity_id_type& id) const;

    const bool funding_opportunity_exists(const external_id_type& number) const;

    funding_opportunity_refs_type get_funding_opportunity_announcements_by_grantor(const account_name_type& owner) const;
    
    funding_opportunity_refs_type get_funding_opportunity_announcements_by_organization(const account_id_type& organization_id) const;

    funding_opportunity_refs_type get_funding_opportunity_announcements_listing(const uint16_t& page, const uint16_t& limit) const;
    
    void remove_funding_opportunity(const funding_opportunity_object& funding_opportunity);

    void adjust_funding_opportunity_supply(const funding_opportunity_id_type& funding_opportunity_id, const asset& delta);

    void distribute_funding_opportunity(const funding_opportunity_object &funding_opportunity);

    void process_funding_opportunities();

};
} // namespace chain
} // namespace deip