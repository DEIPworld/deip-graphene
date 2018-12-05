#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/grant_object.hpp.hpp>

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
                               int16_t& min_number_of_positive_reviews,
                               int16_t& researches_to_grant,
                               fc::time_point_sec start_time,
                               fc::time_point_sec end_time);

    const grant_object& get(const grant_id_type& id);

};
} // namespace chain
} // namespace deip