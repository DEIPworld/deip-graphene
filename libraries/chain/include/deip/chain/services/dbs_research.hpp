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

    const research_object& create_research(const research_group_id_type& research_group_id,
                                           const external_id_type& external_id,
                                           const string& title,
                                           const string& abstract,
                                           const string& permlink,
                                           std::set<discipline_id_type>& disciplines,
                                           const uint16_t& review_share_in_percent,
                                           const uint16_t& dropout_compensation_in_percent,
                                           const bool& is_private,
                                           const bool& is_finished,
                                           const share_type& owned_tokens,
                                           const time_point_sec& created_at,
                                           const time_point_sec& last_update_time,
                                           const time_point_sec& review_share_in_percent_last_update);

    research_refs_type get_researches() const;

    research_refs_type get_researches_by_research_group(const research_group_id_type& research_group_id) const;

    const research_object& get_research(const research_id_type& id) const;

    const research_object& get_research(const external_id_type& external_id) const;

    const research_optional_ref_type get_research_if_exists(const external_id_type& external_id) const;

    const research_object& get_research_by_permlink(const research_group_id_type& research_group_id, const string& permlink) const;

    void check_research_existence(const research_id_type& id) const;

    void decrease_owned_tokens(const research_object& research, const share_type delta);

    void increase_owned_tokens(const research_object& research, const share_type delta);
    
    void change_research_review_share_percent(const research_id_type& research_id, const uint16_t review_share_in_percent);

    const std::map<discipline_id_type, share_type> get_eci_evaluation(const research_id_type& research_id) const;

    const research_object& update_eci_evaluation(const research_id_type& research_id);

    const bool research_exists(const research_id_type& research_id) const;
};
}
}