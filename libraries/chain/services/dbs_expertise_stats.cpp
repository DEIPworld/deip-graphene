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
    db_impl().modify(expertise_stats, [&](expertise_stats_object& eso) {
        eso.used_expertise_per_block = 0;
    });
    return expertise_stats;
}

const expertise_stats_object& dbs_expertise_stats::update_used_expertise(const share_type &delta)
{
    FC_ASSERT(delta >= 0, "Cannot update all used expertise.(delta < 0)");
    auto& expertise_stats = get_expertise_stats();
    db_impl().modify(expertise_stats, [&](expertise_stats_object& eso) {
        eso.total_used_expertise += delta;
        eso.used_expertise_per_block += delta;
    });
    return expertise_stats;
}

const expertise_stats_object& dbs_expertise_stats::calculate_used_expertise_for_week()
{
//    auto& expertise_stats = get_expertise_stats();
//    db_impl().modify(expertise_stats, [&](expertise_stats_object& ueo) {
//        ueo.used_expertise_last_week.push_front(ueo.used_expertise_per_block);
//        if (ueo.used_expertise_last_week.size() > DEIP_BLOCKS_PER_WEEK) {
//            ueo.used_expertise_last_week.pop_back();
//        }
//    });
//    return expertise_stats;

    auto& used_expertise_stats = get_expertise_stats();
    share_type delta = 0;
    if (used_expertise_stats.used_expertise_last_week.size() <= DEIP_BLOCKS_PER_WEEK) {
        delta = used_expertise_stats.used_expertise_per_block;
    } else {
        auto first = used_expertise_stats.used_expertise_last_week.front().value;
        db_impl().modify(used_expertise_stats, [&](expertise_stats_object& eso) {
            eso.used_expertise_last_week.pop_front();
        });
        delta = used_expertise_stats.used_expertise_per_block - first;
    }

    db_impl().modify(used_expertise_stats, [&](expertise_stats_object& eso) {
        eso.used_expertise_last_week.push_back(used_expertise_stats.used_expertise_per_block);
        eso.total_used_expertise_last_week += delta;
    });

    return used_expertise_stats;

}

} // namespace chain
} // namespace deip
