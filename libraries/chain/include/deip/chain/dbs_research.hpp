#pragma once

#include <deip/chain/dbs_base_impl.hpp>

#include <research_object.hpp>
#include <vector>

namespace deip{
namespace chain{

class dbs_research : public dbs_base{

    friend class dbservice_dbs_factory;

    dbs_research() = delete;

protected:

    explicit dbs_research(database &db);

public:

    using research_refs_type = std::vector<std::reference_wrapper<const research_object>>;

    research_refs_type get_researchs() const;

    const research_object& get_research(const research_id_type id) const;

    const research_object& get_research_by_permlink(const string& permlink) const;

    const research_object& get_research_by_discipline_id(const discpline_id_type discipline_ids) const;
};
}
}