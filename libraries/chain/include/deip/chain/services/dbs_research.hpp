#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/research_object.hpp>

#include <vector>

namespace deip{
namespace chain{

class dbs_research : public dbs_base {

    friend class dbservice_dbs_factory;

    dbs_research() = delete;

protected:

    explicit dbs_research(database &db);

public:

    using research_refs_type = std::vector<std::reference_wrapper<const research_object>>;
    using research_optional_ref_type = fc::optional<std::reference_wrapper<const research_object>>;

    const research_object& create_research(const research_group_object& research_group,
                                           const external_id_type& external_id,
                                           const string& title,
                                           const string& abstract,
                                           const std::set<discipline_id_type>& disciplines,
                                           const optional<percent>& review_share,
                                           const optional<percent>& compensation_share,
                                           const bool& is_private,
                                           const bool& is_finished,
                                           const flat_set<account_name_type>& members,
                                           const time_point_sec& created_at);

    const research_object& update_research(const research_object& research,
                                           const string& title,
                                           const string& abstract,
                                           const bool& is_private,
                                           const optional<percent>& review_share,
                                           const optional<percent>& compensation_share,
                                           const flat_set<account_name_type>& members);

    const research_refs_type get_researches_by_research_group(const research_group_id_type& research_group_id) const;

    const research_refs_type get_researches_by_research_group(const account_name_type& research_group) const;

    const research_object& get_research(const research_id_type& id) const;

    const research_object& get_research(const external_id_type& external_id) const;

    /* [DEPRECATED] */ const research_object& get_research_by_permlink(const string& research_group_permlink,
                                                                       const string& research_permlink) const;

    const research_optional_ref_type get_research_if_exists(const research_id_type& id) const;

    const research_optional_ref_type get_research_if_exists(const external_id_type& external_id) const;

    /* [DEPRECATED] */ const research_optional_ref_type get_research_by_permlink_if_exists(const string& research_group_permlink,
                                                                                           const string& research_permlink) const;

    void check_research_existence(const research_id_type& id) const;

    const std::map<discipline_id_type, share_type> get_eci_evaluation(const research_id_type& research_id) const;

    const research_object& update_eci_evaluation(const research_id_type& research_id);

    const bool research_exists(const research_id_type& research_id) const;

    const bool research_exists(const external_id_type& external_id) const;

    const research_refs_type lookup_researches(const research_id_type& lower_bound, uint32_t limit) const;

    const research_refs_type get_researches_by_member(const account_name_type& member) const;
};
}
}