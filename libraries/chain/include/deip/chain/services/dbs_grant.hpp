#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/grant_object.hpp>

namespace deip {
namespace chain {

class dbs_grant : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_grant() = delete;

protected:
    explicit dbs_grant(database &db);

public:
    using grant_refs_type = std::vector<std::reference_wrapper<const grant_object>>;

    const grant_object& create_grant_with_announced_application_window(
      const account_name_type& grantor,
      const asset& amount,
      const std::set<discipline_id_type>& target_disciplines,
      const research_group_id_type& review_committee_id,
      const uint16_t& min_number_of_positive_reviews,
      const uint16_t& min_number_of_applications,
      const uint16_t& max_number_of_research_to_grant,
      const fc::time_point_sec start_date,
      const fc::time_point_sec end_date);

    const grant_object& get_grant_with_announced_application_window(const grant_id_type& id) const;

    const fc::optional<std::reference_wrapper<const grant_object>> get_grant_with_announced_application_window_if_exists(const grant_id_type& id) const;

    const bool grant_with_announced_application_window_exists(const grant_id_type& id) const;

    grant_refs_type get_grants_with_announced_application_window_by_grantor(const account_name_type& owner);
    
    void remove_grant_with_announced_application_window(const grant_object& grant);

};
} // namespace chain
} // namespace deip