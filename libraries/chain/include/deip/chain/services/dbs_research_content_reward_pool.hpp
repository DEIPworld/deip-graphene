#pragma once

#include "dbs_base_impl.hpp"
#include "../schema/research_content_reward_pool_object.hpp"

#include <vector>

namespace deip{
namespace chain{

class dbs_research_content_reward_pool : public dbs_base{

    friend class dbservice_dbs_factory;

    dbs_research_content_reward_pool() = delete;

protected:

    explicit dbs_research_content_reward_pool(database &db);

public:
    using research_content_reward_pool_refs_type = std::vector<std::reference_wrapper<const research_content_reward_pool_object>>;

    const research_content_reward_pool_object& create(const research_content_id_type& research_content_id,
                                                      const discipline_id_type& discipline_id,
                                                      const share_type reward_share,
                                                      const share_type expertise_share);

    const research_content_reward_pool_object& get(const research_content_reward_pool_id_type& id) const;

    const research_content_reward_pool_object& get_by_research_content_id_and_discipline_id(const research_content_id_type& research_content_id,
                                                                                            const discipline_id_type& discipline_id) const;

    research_content_reward_pool_refs_type get_research_content_reward_pools_by_content_id(const research_content_id_type &research_content_id) const;

    bool is_research_reward_pool_exists_by_research_content_id_and_discipline_id(const research_content_id_type& research_content_id,
                                                                                 const discipline_id_type& discipline_id);

    void increase_reward_pool(const research_content_reward_pool_object& research_reward_pool, const share_type delta);

    void increase_expertise_pool(const research_content_reward_pool_object& research_reward_pool, const share_type delta);


};
}
}