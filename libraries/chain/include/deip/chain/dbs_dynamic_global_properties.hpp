#pragma once

#include <deip/chain/dbs_base_impl.hpp>

namespace deip {
namespace chain {

class dbs_dynamic_global_properties : public dbs_base
{

    friend class dbservice_dbs_factory;

    dbs_dynamic_global_properties() = delete;

protected:
    explicit dbs_dynamic_global_properties(database& db);

public:
    const dynamic_global_property_object& get_dynamic_global_properties() const;
    const dynamic_global_property_object& reset_used_expertise_per_block();
    const dynamic_global_property_object& update_used_expertise(const share_type &delta);
};
} // namespace chain
} // namespace deip