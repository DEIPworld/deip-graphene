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
    /** Lists all disciplines.
    *
    * @returns a list of discipline objects
    */
    discipline_ref_type get_disciplines() const;

    /** Get discipline by id
    */
    const discipline_object& get_discipline(const discipline_id_type& id) const;

    const discipline_object& get_discipline(const external_id_type& external_id) const;

    const discipline_optional_ref_type get_discipline_if_exists(const discipline_id_type& id) const;

    const discipline_optional_ref_type get_discipline_if_exists(const external_id_type& id) const;

    void check_discipline_existence(const discipline_id_type& id);

    const bool discipline_exists(const discipline_id_type& id) const;

    /** Get discipline by parent_id
    */
    discipline_ref_type get_disciplines_by_parent_id(const discipline_id_type parent_id) const;
    
};
} // namespace chain
} // namespace deip
