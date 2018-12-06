#include <deip/chain/services/dbs_grant.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

dbs_grant::dbs_grant(database &db)
    : _base_type(db)
{
}

const grant_object& dbs_grant::create(const discipline_id_type& target_discipline,
                                      const asset& amount,
                                      const int16_t& min_number_of_positive_reviews,
                                      const int16_t& researches_to_grant,
                                      fc::time_point_sec start_time,
                                      fc::time_point_sec end_time)
{
    auto now = db_impl().head_block_time();

    FC_ASSERT(start_time >= now, "Start time must be greater then now");

    auto& grant = db_impl().create<grant_object>([&](grant_object& grant) {
        grant.target_discipline = target_discipline;
        grant.amount = amount;
        grant.min_number_of_positive_reviews = min_number_of_positive_reviews;
        grant.researches_to_grant = researches_to_grant;
        grant.start_time = start_time;
        grant.end_time = end_time;
        grant.created_at = now;
    });

    return grant;
}

const grant_object& dbs_grant::get(const grant_id_type& id) const
{
    try {
        return db_impl().get<grant_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

void dbs_grant::check_grant_existence(const grant_id_type& id) const
{
    const auto& grant = db_impl().find<grant_object, by_id>(id);
    FC_ASSERT(grant != nullptr, "Grant with id \"${1}\" must exist.", ("1", id));
}

} //namespace chain
} //namespace deip