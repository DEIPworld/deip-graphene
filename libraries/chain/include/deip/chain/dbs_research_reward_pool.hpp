#pragma once

#include <deip/chain/dbs_base_impl.hpp>
#include <deip/chain/research_reward_pool_object.hpp>

#include <vector>

namespace deip{
namespace chain{

class dbs_research_reward_pool : public dbs_base{

    friend class dbservice_dbs_factory;

    dbs_research_reward_pool() = delete;

protected:

    explicit dbs_research_reward_pool(database &db);

public:

    const research_reward_pool_object& create(const research_id_type& research_id, const share_type reward_amount);

    const research_reward_pool_object& get(const research_reward_pool_id_type& id) const;

    const research_reward_pool_object& get_by_research_id(const research_id_type& research_id) const;

    void check_research_reward_pool_existence_by_research_id(const research_id_type& research_id) const;

    void increase_reward_pool(const research_reward_pool_object& research_reward_pool, const share_type delta);

};
}
}