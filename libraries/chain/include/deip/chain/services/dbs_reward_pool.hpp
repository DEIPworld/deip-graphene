#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/reward_pool_object.hpp>

#include <vector>

namespace deip{
namespace chain{

class dbs_reward_pool : public dbs_base{

    friend class dbservice_dbs_factory;

    dbs_reward_pool() = delete;

protected:

    explicit dbs_reward_pool(database &db);

public:
    using reward_pool_refs_type = std::vector<std::reference_wrapper<const reward_pool_object>>;

    const reward_pool_object& create(const research_content_id_type& research_content_id,
                                                      const discipline_id_type& discipline_id,
                                                      const asset& balance,
                                                      const share_type& expertise);

    const reward_pool_object& get(const reward_pool_id_type& id) const;

    const reward_pool_object& get_by_research_content_id_and_discipline_id(const research_content_id_type& research_content_id,
                                                                                            const discipline_id_type& discipline_id) const;

    reward_pool_refs_type get_reward_pools_by_content_id(const research_content_id_type &research_content_id) const;

    reward_pool_refs_type get_all() const;

    bool is_research_reward_pool_exists_by_research_content_id_and_discipline_id(const research_content_id_type& research_content_id,
                                                                                 const discipline_id_type& discipline_id);

    void increase_reward_pool(const reward_pool_object& research_reward_pool, const asset& delta);

    void increase_expertise_pool(const reward_pool_object& research_reward_pool, const share_type& delta);


};
}
}