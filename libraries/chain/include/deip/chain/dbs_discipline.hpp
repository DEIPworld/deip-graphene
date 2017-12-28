#pragma once

#include <deip/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/discipline_object.hpp>

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
    using discipline_refs_type = std::vector<std::reference_wrapper<const discipline_object>>;

    /** Lists all disciplines.
    *
    * @returns a list of discipline objects
    */
    discipline_refs_type get_disciplines() const;

    /** Get discipline by id
    */
    const discipline_object& get_discipline(const discipline_id_type id) const;

    /** Get discipline by name
    */
    const discipline_object& get_discipline_by_name(const discipline_name_type& name) const;

    /** Get discipline by parent_id
    */
    discipline_refs_type get_disciplines_by_parent_id(const discipline_id_type parent_id) const;
};
} // namespace chain
} // namespace deip