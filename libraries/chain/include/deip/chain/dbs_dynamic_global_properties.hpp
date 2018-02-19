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
};
} // namespace chain
} // namespace deip