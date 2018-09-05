#include <deip/chain/services/dbs_expertise_stats.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

dbs_expertise_stats::dbs_expertise_stats(database& db)
        : _base_type(db)
{
}

const expertise_stats_object& dbs_expertise_stats::get_expertise_stats() const
{
    try
    {
        return db_impl().get_expertise_stats();
    }
    FC_CAPTURE_AND_RETHROW();
}

const expertise_stats_object& dbs_expertise_stats::reset_used_expertise_per_block()
{
    auto& expertise_stats = get_expertise_stats();
    db_impl().modify(expertise_stats, [&](expertise_stats_object& ueo) { ueo.used_expertise_per_block = 0; });
    return expertise_stats;
}

const expertise_stats_object& dbs_expertise_stats::update_used_expertise(const share_type &delta)
{
    FC_ASSERT(delta >= 0, "Cannot update all used expertise.(delta < 0)");
    auto& expertise_stats = get_expertise_stats();
    db_impl().modify(expertise_stats, [&](expertise_stats_object& ueo) {
        ueo.total_used_expertise += delta;
        ueo.used_expertise_per_block += delta;
    });
    return expertise_stats;
}

} // namespace chain
} // namespace deip
