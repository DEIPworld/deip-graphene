#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/award_object.hpp>
#include <deip/chain/schema/award_research_relation_object.hpp>

#include <vector>

namespace deip{
namespace chain{

class dbs_award : public dbs_base{

    friend class dbservice_dbs_factory;

    dbs_award() = delete;

protected:

    explicit dbs_award(database &db);

public:

    const award_object& create_award(const funding_opportunity_id_type& funding_opportunity_id,
                                     const account_name_type& creator,
                                     const asset& amount);

    const award_object& get_award(const award_id_type& id) const;

    // Award research relation

    using award_research_relation_refs_type = std::vector<std::reference_wrapper<const award_research_relation_object>>;

    const award_research_relation_object& create_award_research_relation(const award_id_type& award_id,
                                                                         const research_id_type& research_id,
                                                                         const research_group_id_type& research_group_id,
                                                                         const account_name_type& awardee,
                                                                         const asset& total_amount,
                                                                         const research_group_id_type& university_id,
                                                                         const share_type& university_overhead);

    const award_research_relation_object& get_award_research_relation(const award_research_relation_id_type& id);


};
}
}
