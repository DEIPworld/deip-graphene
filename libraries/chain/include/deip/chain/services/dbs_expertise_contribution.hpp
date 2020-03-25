#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/expertise_contribution_object.hpp>

namespace deip {
namespace chain {

///** DB service for operations with expertise_contribution
// *  --------------------------------------------
// */
class dbs_expertise_contribution : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_expertise_contribution() = delete;

protected:
    explicit dbs_expertise_contribution(database& db);

public:

    using expertise_contributions_refs_type = std::vector<std::reference_wrapper<const expertise_contribution_object>>;

    const expertise_contribution_object& adjust_expertise_contribution( const discipline_id_type& discipline_id,
                                                                        const research_id_type& research_id,
                                                                        const research_content_id_type& research_content_id,
                                                                        const eci_diff& diff);
    
    const expertise_contribution_object& get_expertise_contribution(const expertise_contribution_id_type& id) const;

    const expertise_contribution_object& get_expertise_contribution_by_research_content_and_discipline( const research_content_id_type& research_content_id, 
                                                                                                        const discipline_id_type& discipline_id) const;
                                              
    expertise_contributions_refs_type get_expertise_contributions_by_research_and_discipline(const research_id_type& research_id,
                                                                                             const discipline_id_type& discipline_id) const;

    expertise_contributions_refs_type get_expertise_contributions_by_discipline(const discipline_id_type& discipline_id) const;

    expertise_contributions_refs_type get_expertise_contributions_by_research_content(const research_content_id_type& research_content_id) const;

    expertise_contributions_refs_type get_expertise_contributions_by_research(const research_id_type& research_id) const;

    const bool expertise_contribution_exists( const research_content_id_type& research_content_id,
                                              const discipline_id_type& discipline_id) const;

    expertise_contributions_refs_type get_altered_expertise_contributions_in_block() const;

    expertise_contributions_refs_type get_increased_expertise_contributions_in_block() const;

    expertise_contributions_refs_type get_decreased_expertise_contributions_in_block() const;
};
} // namespace chain
} // namespace deip
