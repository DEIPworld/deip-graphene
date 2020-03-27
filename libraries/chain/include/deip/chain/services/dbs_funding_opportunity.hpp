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

    const funding_opportunity_object& create_funding_opportunity_announcement(  const research_group_id_type& organization_id,
                                                                                const research_group_id_type& review_committee_id,
                                                                                const account_name_type& grantor,
                                                                                const string& funding_opportunity_number,
                                                                                const flat_map<string, string>& additional_info,
                                                                                const std::set<discipline_id_type>& target_disciplines,
                                                                                const asset& amount,
                                                                                const asset& award_ceiling, 
                                                                                const asset& award_floor,
                                                                                const uint16_t& expected_number_of_awards,
                                                                                const std::set<account_name_type>& officers,
                                                                                const fc::time_point_sec& open_date,
                                                                                const fc::time_point_sec& close_date);

    const funding_opportunity_object& get_funding_opportunity_announcement(const funding_opportunity_id_type& id) const;
    
    const funding_opportunity_object& get_funding_opportunity_announcement(const string& funding_opportunity_number) const;

    const fc::optional<std::reference_wrapper<const funding_opportunity_object>> get_funding_opportunity_announcement_if_exists(const funding_opportunity_id_type& id) const;

    const fc::optional<std::reference_wrapper<const funding_opportunity_object>> get_funding_opportunity_announcement_if_exists(const string& opportunity_number) const;

    const bool funding_opportunity_announcement_exists(const funding_opportunity_id_type& id) const;

    const bool funding_opportunity_announcement_exists(const string& number) const;

    funding_opportunity_refs_type get_funding_opportunity_announcements_by_grantor(const account_name_type& owner) const;
    
    funding_opportunity_refs_type get_funding_opportunity_announcements_by_organization(const research_group_id_type& organization_id) const;

    funding_opportunity_refs_type get_funding_opportunity_announcements_listing(const uint16_t& page, const uint16_t& limit) const;
    
    void remove_funding_opportunity_announcement(const funding_opportunity_object& funding_opportunity);

    void adjust_fundind_opportunity_supply(const funding_opportunity_id_type& funding_opportunity_id, const asset& delta);
};
} // namespace chain
} // namespace deip