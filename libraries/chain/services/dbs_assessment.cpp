#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_assessment.hpp>

namespace deip {
namespace chain {

dbs_assessment::dbs_assessment(database& db)
    : _base_type(db)
{
}

const assessment_object& dbs_assessment::create_assessment(const external_id_type& external_id,
                                                           const account_name_type& creator)
{
    const auto& assessment = db_impl().create<assessment_object>([&](assessment_object& a_o) {
        a_o.external_id = external_id;
        a_o.creator = creator;
    });

    return assessment;
}

} // namespace chain
} // namespace deip