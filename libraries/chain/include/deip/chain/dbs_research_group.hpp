#pragma once

#include <deip/chain/dbs_base_impl.hpp>

#include <vector>
#include <set>
#include <functional>

#include <deip/chain/research_group_object.hpp>

namespace deip {
namespace chain {

class proposal_object;

class dbs_research_group : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_research_group() = delete;
protected:
    explicit dbs_research_group(database& db);

public:

    /** Get research group by id
     */
    const research_group_object& get_research_group(research_group_id_type id) const;

    /** Create research_group object.
     *
     * @returns research group object
     */
    const research_group_object& create_research_group(const fc::shared_string permlink, const fc::shared_string description);
};

} // namespace chain
} // namespace deip