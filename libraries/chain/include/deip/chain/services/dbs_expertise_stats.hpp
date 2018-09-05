#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/expertise_stats_object.hpp>

namespace deip {
namespace chain {

class dbs_expertise_stats : public dbs_base
{

    friend class dbservice_dbs_factory;

    dbs_expertise_stats() = delete;

protected:
    explicit dbs_expertise_stats(database& db);

public:
    const expertise_stats_object& get_expertise_stats() const;
    const expertise_stats_object& reset_used_expertise_per_block();
    const expertise_stats_object& update_used_expertise(const share_type &delta);
};
} // namespace chain
} // namespace deip
