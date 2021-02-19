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
    using research_content_optional_ref_type = fc::optional<std::reference_wrapper<const research_content_object>>;

    const research_content_object& create_research_content(const research_group_object& research_group,
                                                           const research_object& research,
                                                           const external_id_type& external_id,
                                                           const std::string& description,
                                                           const std::string& content,
                                                           const research_content_type& type,
                                                           const flat_set<account_name_type>& authors,
                                                           const flat_set<external_id_type>& references,
                                                           const fc::time_point_sec& timestamp);

    const research_content_object& get_research_content(const research_content_id_type& id) const;

    const research_content_object& get_research_content(const external_id_type& external_id) const;

    const research_content_optional_ref_type get_research_content_if_exists(const external_id_type& external_id) const;

    const research_content_optional_ref_type get_research_content_if_exists(const research_content_id_type& id) const;

    research_content_refs_type get_research_content_by_research_id(const research_id_type& research_id) const;

    research_content_refs_type get_by_research_and_type(const research_id_type &research_id, const research_content_type &type) const;

    void check_research_content_existence(const research_content_id_type& research_content_id);
        
    const std::map<discipline_id_type, share_type> get_eci_evaluation(const research_content_id_type& research_content_id) const;

    const research_content_object& update_eci_evaluation(const research_content_id_type& research_content_id);

    const research_content_refs_type lookup_research_contents(const research_content_id_type& lower_bound, uint32_t limit) const;
};
}
}