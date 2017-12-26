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
    const discipline_object& get_discipline(discipline_id_type id) const;
};
} // namespace chain
} // namespace deip
