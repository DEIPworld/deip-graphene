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
    const research_group_object& get_research_group(const research_group_id_type& id) const;

    /** Create research_group object.
     *
     * @returns research group object
     */
    const research_group_object& create_research_group(const string permlink, const string description);

    void change_quorum(const uint32_t quorum_percent, const research_group_id_type& research_group_id);

    void check_research_group_existence(const research_group_id_type& research_group_id) const;

    const research_group_token_object& get_research_group_token_by_id(const research_group_token_id_type& id) const;

    const research_group_token_object& get_research_group_token_by_account(const account_name_type& account,
                                                                const research_group_id_type& research_group_id) const;

    const research_group_token_object& create_research_group_token(const research_group_id_type& research_group_id,
                                                                   const share_type amount,
                                                                   const account_name_type& account);

    void remove_token(const account_name_type& account, const research_group_id_type& research_group_id);

    void check_research_group_token_existence(const account_name_type& account,
                                        const research_group_id_type& research_group_id) const;

    void update_research_group_token_share(const share_type amount,
                                           const research_group_token_id_type& token_id);

    void check_member_existence(const account_name_type& account, const research_group_id_type& group_id);

    size_t get_members_count(const research_group_id_type& group_id);

};

} // namespace chain
} // namespace deip