#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/research_content_object.hpp>

#include <vector>


namespace deip{
namespace chain{

class dbs_research_content : public dbs_base {

    friend class dbservice_dbs_factory;

    dbs_research_content() = delete;

protected:

    explicit dbs_research_content(database &db);

public:

    using research_content_refs_type = std::vector<std::reference_wrapper<const research_content_object>>;

    const research_content_object& create(const research_id_type& research_id,
                                          const research_content_type& type,
                                          const string& title,
                                          const string& content,
                                          const string& permlink,
                                          const std::vector<account_name_type>& authors,
                                          const std::vector<research_content_id_type>& references,
                                          const std::vector<string>& external_references);

    const research_content_object& get(const research_content_id_type& id) const;

    const research_content_object& get_by_permlink(const research_id_type &research_id, const string &permlink) const;

    research_content_refs_type get_by_research_id(const research_id_type &research_id) const;

    research_content_refs_type get_by_research_and_type(const research_id_type &research_id,
                                                        const research_content_type &type) const;

    void check_research_content_existence(const research_content_id_type& research_content_id);

    research_content_refs_type get_all_milestones_by_research_id(const research_id_type& research_id) const;
};
}
}