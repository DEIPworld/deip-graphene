#pragma once

#include <deip/chain/dbs_base_impl.hpp>

#include <vector>
#include <set>
#include <functional>

#include <deip/chain/deip_object_types.hpp>
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
    const research_group_object& create_research_group(const string permlink, const string description);
};

class dbs_research_group_token : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_research_group_token() = delete;
protected:
    explicit dbs_research_group_token(database& db);

public:

    /** Get research group token by id
     */
    const research_group_token_object& get_research_group_token(research_group_token_id_type id) const;

    const research_group_token_object& get_research_group_token(const account_name_type account_name, 
                                                                const research_group_id_type research_group_id) const;
    /** Create research_group_token object.
     *
     * @returns research group token object
     */

    const research_group_token_object& create_research_group_token(const research_group_id_type research_group,
                                                                   const share_type amount,
                                                                   const account_name_type account_name);

    void remove_token_object(const account_name_type& account_name, research_group_id_type research_group_id);

    bool token_exists(const account_name_type& account_name, 
                      research_group_id_type research_group_id) const;

    void add_share_to_research_group_token(const share_type amount, 
                                           const research_group_token_object& research_group_token);
};

} // namespace chain
} // namespace deip