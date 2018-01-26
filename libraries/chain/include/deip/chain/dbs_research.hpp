#pragma once

#include <deip/chain/dbs_base_impl.hpp>
#include <deip/chain/research_object.hpp>

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

    const research_object& create(const string& name, const string& abstract, const string& permlink,
                                  const research_group_id_type& research_group_id, const uint32_t& percent_for_review);

    research_refs_type get_researches() const;

    const research_object& get_research(const research_id_type& id) const;

    const research_object& get_research_by_permlink(const string& permlink) const;

    void check_research_existence(const research_id_type& id) const;
};
}
}