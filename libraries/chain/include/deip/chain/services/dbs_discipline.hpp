#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/discipline_object.hpp>

namespace deip {
    namespace chain {

///** DB service for operations with discipline_object
// *  --------------------------------------------
// */
class dbs_discipline : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_discipline() = delete;

protected:
    explicit dbs_discipline(database &db);

public:
    using discipline_ref_type = std::vector<std::reference_wrapper<const discipline_object>>;
    using discipline_optional_ref_type = fc::optional<std::reference_wrapper<const discipline_object>>;


    const discipline_object& get_discipline(const discipline_id_type& id) const;

    const discipline_object& get_discipline(const external_id_type& external_id) const;

    const discipline_optional_ref_type get_discipline_if_exists(const discipline_id_type& id) const;

    const discipline_optional_ref_type get_discipline_if_exists(const external_id_type& id) const;

    const bool discipline_exists(const discipline_id_type& id) const;

    const bool discipline_exists(const external_id_type& external_id) const;

    const discipline_ref_type get_disciplines_by_parent(const external_id_type& parent_external_id) const;

    const discipline_ref_type lookup_disciplines(const discipline_id_type& lower_bound, uint32_t limit) const;
};
} // namespace chain
} // namespace deip
