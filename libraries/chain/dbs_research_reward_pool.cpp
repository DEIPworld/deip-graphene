#include <deip/chain/dbs_research_reward_pool.hpp>
#include <deip/chain/database.hpp>


namespace deip{
namespace chain{

dbs_research_reward_pool::dbs_research_reward_pool(database &db) : _base_type(db)
{
}

const research_reward_pool_object& dbs_research_reward_pool::create(const research_id_type& research_id, const share_type reward_amount)
{
    const auto& new_research_reward_pool = db_impl().create<research_reward_pool_object>([&](research_reward_pool_object& r) {
        r.research_id = research_id;
        r.reward_amount = reward_amount;
    });

    return new_research_reward_pool;
}

const research_reward_pool_object& dbs_research_reward_pool::get(const research_reward_pool_id_type& id) const
{
    try {
        return db_impl().get<research_reward_pool_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const research_reward_pool_object& dbs_research_reward_pool::get_by_research_id(const research_id_type& research_id) const
{
    try {
        return db_impl().get<research_reward_pool_object, by_research_id>(research_id);
    }
    FC_CAPTURE_AND_RETHROW((research_id))
}

void dbs_research_reward_pool::check_research_reward_pool_existence_by_research_id(const research_id_type& research_id) const
{
    auto research_reward_pool = db_impl().find<research_reward_pool_object, by_research_id>(research_id);
    FC_ASSERT(research_reward_pool != nullptr, "Research reward pool with research_id \"${1}\" must exist.", ("1", research_id));
}

void dbs_research_reward_pool::increase_reward_pool(const research_reward_pool_object& research_reward_pool, const share_type delta)
{
    FC_ASSERT(delta >= 0, "Cannot update research reward pool (delta <= 0)");
    db_impl().modify(research_reward_pool, [&](research_reward_pool_object& rrp_o) { rrp_o.reward_amount += delta; });
}

}
}