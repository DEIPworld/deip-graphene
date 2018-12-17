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

    const grant_object& create(const discipline_id_type& target_discipline,
                               const asset& amount,
                               const int16_t& min_number_of_positive_reviews,
                               const int16_t& min_number_of_applications,
                               const int16_t& researches_to_grant,
                               fc::time_point_sec start_time,
                               fc::time_point_sec end_time,
                               const account_name_type& owner);

    const grant_object& get(const grant_id_type& id) const;

    void check_grant_existence(const grant_id_type& id) const;

    grant_refs_type get_by_target_discipline(const discipline_id_type& discipline_id);

    grant_refs_type get_by_owner(const account_name_type& owner);
    
    std::set<string> lookup_grant_owners(const string &lower_bound_owner_name, uint32_t limit) const;

    void delete_grant(const grant_object& grant);

};
} // namespace chain
} // namespace deip