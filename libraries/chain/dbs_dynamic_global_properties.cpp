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
    return db_impl().get_dynamic_global_properties();
}

} // namespace chain
} // namespace deip