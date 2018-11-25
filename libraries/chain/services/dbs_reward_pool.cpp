#include <deip/chain/services/dbs_reward_pool.hpp>
#include <deip/chain/database/database.hpp>


namespace deip{
namespace chain{

dbs_reward_pool::dbs_reward_pool(database &db) : _base_type(db)
{
}

const reward_pool_object& dbs_reward_pool::create(const research_content_id_type& research_content_id,
                                                                                    const discipline_id_type& discipline_id,
                                                                                    const asset& balance,
                                                                                    const share_type& expertise)
{
    const auto& new_research_reward_pool = db_impl().create<reward_pool_object>([&](reward_pool_object& r) {
        r.research_content_id = research_content_id;
        r.discipline_id = discipline_id;
        r.balance = balance;
        r.expertise = expertise;
    });

    return new_research_reward_pool;
}

const reward_pool_object& dbs_reward_pool::get(const reward_pool_id_type& id) const
{
    try {
        return db_impl().get<reward_pool_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const reward_pool_object& dbs_reward_pool::get_by_research_content_id_and_discipline_id(const research_content_id_type& research_content_id,
                                                                                                                          const discipline_id_type& discipline_id) const
{
    try {
        return db_impl().get<reward_pool_object,
                by_content_and_discipline>(std::make_tuple(research_content_id, discipline_id));
    }
    FC_CAPTURE_AND_RETHROW((research_content_id))
}

dbs_reward_pool::reward_pool_refs_type
dbs_reward_pool::get_reward_pools_by_content_id(const research_content_id_type &research_content_id) const
{
    reward_pool_refs_type ret;

    auto it_pair = db_impl().get_index<reward_pool_index>().indicies().get<by_research_content>().equal_range(research_content_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

bool dbs_reward_pool::is_research_reward_pool_exists_by_research_content_id_and_discipline_id(const research_content_id_type& research_content_id,
                                                                                                               const discipline_id_type& discipline_id)
{
    const auto& idx = db_impl().get_index<reward_pool_index>().indices().get<by_content_and_discipline>();

    if (idx.find(std::make_tuple(research_content_id, discipline_id)) != idx.cend())
        return true;
    else
        return false;
}

void dbs_reward_pool::increase_reward_pool(const reward_pool_object& research_reward_pool, const asset& delta)
{
    FC_ASSERT(delta >= asset(0, DEIP_SYMBOL), "Cannot update research reward pool (delta <= 0)");
    db_impl().modify(research_reward_pool, [&](reward_pool_object& rrp_o) { rrp_o.balance += delta; });
}

void dbs_reward_pool::increase_expertise_pool(const reward_pool_object& research_reward_pool, const share_type& delta)
{
    FC_ASSERT(delta >= 0, "Cannot update research expertise pool (delta <= 0)");
    db_impl().modify(research_reward_pool, [&](reward_pool_object& rrp_o) { rrp_o.expertise += delta; });
}

}
}