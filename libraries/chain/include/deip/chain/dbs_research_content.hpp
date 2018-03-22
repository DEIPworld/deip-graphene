#pragma once

#include <deip/chain/dbs_base_impl.hpp>
#include <deip/chain/research_content_object.hpp>

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
                                          const string& content,
                                          const flat_set<account_name_type>& authors,
                                          const std::vector<research_references_data>& research_references,
                                          const std::vector<string>& research_external_references);

    const research_content_object& get_content_by_id(const research_content_id_type& id) const;

    research_content_refs_type get_content_by_research_id(const research_id_type& research_id) const;

    research_content_refs_type get_content_by_research_id_and_content_type(const research_id_type& research_id, const research_content_type& type) const;

    void check_research_content_existence(const research_content_id_type& research_content_id);
};
}
}