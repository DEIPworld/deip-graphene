#include <deip/chain/dbs_research_content_reward_pool.hpp>
#include <deip/chain/database.hpp>


namespace deip{
namespace chain{

dbs_research_content_reward_pool::dbs_research_content_reward_pool(database &db) : _base_type(db)
{
}

const research_content_reward_pool_object& dbs_research_content_reward_pool::create(const research_content_id_type& research_content_id,
                                                                                    const discipline_id_type& discipline_id,
                                                                                    const share_type reward_share,
                                                                                    const share_type expertise_share)
{
    const auto& new_research_reward_pool = db_impl().create<research_content_reward_pool_object>([&](research_content_reward_pool_object& r) {
        r.research_content_id = research_content_id;
        r.discipline_id = discipline_id;
        r.reward_share = reward_share;
        r.expertise_share = expertise_share;
    });

    return new_research_reward_pool;
}

const research_content_reward_pool_object& dbs_research_content_reward_pool::get(const research_content_reward_pool_id_type& id) const
{
    try {
        return db_impl().get<research_content_reward_pool_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const research_content_reward_pool_object& dbs_research_content_reward_pool::get_by_research_content_id_and_discipline_id(const research_content_id_type& research_content_id,
                                                                                                                          const discipline_id_type& discipline_id) const
{
    try {
        return db_impl().get<research_content_reward_pool_object,
                by_content_and_discipline>(std::make_tuple(research_content_id, discipline_id));
    }
    FC_CAPTURE_AND_RETHROW((research_content_id))
}

bool dbs_research_content_reward_pool::is_research_reward_pool_exists_by_research_content_id_and_discipline_id(const research_content_id_type& research_content_id,
                                                                                                               const discipline_id_type& discipline_id)
{
    const auto& idx = db_impl().get_index<research_content_reward_pool_index>().indices().get<by_content_and_discipline>();

    if (idx.find(std::make_tuple(research_content_id, discipline_id)) != idx.cend())
        return true;
    else
        return false;
}

void dbs_research_content_reward_pool::increase_reward_pool(const research_content_reward_pool_object& research_reward_pool, const share_type delta)
{
    FC_ASSERT(delta >= 0, "Cannot update research reward pool (delta <= 0)");
    db_impl().modify(research_reward_pool, [&](research_content_reward_pool_object& rrp_o) { rrp_o.reward_share += delta; });
}

void dbs_research_content_reward_pool::increase_expertise_pool(const research_content_reward_pool_object& research_reward_pool, const share_type delta)
{
    FC_ASSERT(delta >= 0, "Cannot update research expertise pool (delta <= 0)");
    db_impl().modify(research_reward_pool, [&](research_content_reward_pool_object& rrp_o) { rrp_o.expertise_share += delta; });
}

}
}