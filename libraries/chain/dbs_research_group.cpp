#include <deip/chain/dbs_research_group.hpp>
#include <deip/chain/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_research_group::dbs_research_group(database& db)
    : _base_type(db)
{
}

const research_group_object& get_research_group(research_group_id_type id) const {
    return db_impl().get<research_group_object>(id);
}

const research_group_object& create_research_group(const fc::shared_string permlink,
                                                   const fc::shared_string description) {
    const research_group_object& new_research_group = db_impl().create<research_group_object>([&](research_group_object& research_group) {
        research_group.permlink = permlink;
        research_group.desciption = description;
    });

    return new_research_group;
}

} // namespace chain
} // namespace deip