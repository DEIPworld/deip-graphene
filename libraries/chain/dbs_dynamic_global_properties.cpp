#include <deip/chain/dbs_dynamic_global_properties.hpp>
#include <deip/chain/database.hpp>

namespace deip {
namespace chain {

dbs_dynamic_global_properties::dbs_dynamic_global_properties(database& db)
    : _base_type(db)
{
}

const dynamic_global_property_object& dbs_dynamic_global_properties::get_dynamic_global_properties() const
{
    try {
        return db_impl().get_dynamic_global_properties();
    }
    FC_CAPTURE_AND_RETHROW();
}

const dynamic_global_property_object& dbs_dynamic_global_properties::reset_used_expertise_per_block()
{
    auto& global_property_object = get_dynamic_global_properties();
    db_impl().modify(global_property_object, [&](dynamic_global_property_object& dgp_o) { dgp_o.used_expertise_per_block = 0; });
    return global_property_object;
}

const dynamic_global_property_object& dbs_dynamic_global_properties::update_used_expertise(const share_type &delta)
{
    FC_ASSERT((delta >= 0), "Cannot update all used expertise.(delta < 0)");
    auto& global_property_object = db_impl().get_dynamic_global_properties();
    db_impl().modify(global_property_object, [&](dynamic_global_property_object& dgp_o) { dgp_o.total_used_expertise += delta;
                                                                                          dgp_o.used_expertise_per_block += delta; });
    return global_property_object;
}

} // namespace chain
} // namespace deip