#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/research_discipline_relation_object.hpp>

namespace deip {
namespace chain {

///** DB service for operations with research_discipline_relation_object
// *  --------------------------------------------
// */
class dbs_research_discipline_relation : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_research_discipline_relation() = delete;

protected:
    explicit dbs_research_discipline_relation(database &db);

public:
    using research_discipline_relation_refs_type = std::vector<std::reference_wrapper<const research_discipline_relation_object>>;

    const research_discipline_relation_object& create(const research_id_type& research_id, const discipline_id_type& discipline_id);

    const research_discipline_relation_object& update_votes_count(const research_id_type& research_id, const discipline_id_type& discipline_id, int16_t delta);

    /** Get research_discipline_relation by id
    */
    const research_discipline_relation_object& get_research_discipline_relation(const research_discipline_relation_id_type& id) const;

    /** Get research_discipline_relations by research_id
    */
    research_discipline_relation_refs_type get_research_discipline_relations_by_research(const research_id_type& research_id) const;

    /** Get research_discipline_relations by discipline_id
    */
    research_discipline_relation_refs_type get_research_discipline_relations_by_discipline(const discipline_id_type& discipline_id) const;

    /** Get research_discipline_relations by research_id & discipline_id
    */
    const research_discipline_relation_object& get_research_discipline_relations_by_research_and_discipline(const research_id_type& research_id, const discipline_id_type& discipline_id) const;

    void check_existence_by_research_and_discipline(const research_id_type& research_id, const discipline_id_type& discipline_id) const;
};
} // namespace chain
} // namespace deip
