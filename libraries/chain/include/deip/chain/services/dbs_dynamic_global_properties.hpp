#pragma once

#include "dbs_base_impl.hpp"
#include <deip/protocol/block.hpp>
#include <deip/chain/schema/recent_entity_object.hpp>

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

    const recent_entity_object& create_recent_entity(const external_id_type& external_id);

    void clear_expired_recent_entities(const signed_block& next_block);

    const bool entity_exists(const external_id_type& external_id) const;
};
} // namespace chain
} // namespace deip