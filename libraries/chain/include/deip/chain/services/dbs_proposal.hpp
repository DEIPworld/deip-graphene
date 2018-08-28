#pragma once

#include "dbs_base_impl.hpp"

#include <vector>
#include <set>
#include <functional>

#include "../schema/proposal_object.hpp"
#include "../schema/account_object.hpp"
#include "../schema/proposal_vote_object.hpp"

namespace deip {
namespace chain {

class proposal_object;

class dbs_proposal : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_proposal() = delete;
protected:
    explicit dbs_proposal(database& db);

public:
    using proposal_votes_ref_type = std::vector<std::reference_wrapper<const proposal_vote_object>>;
    using proposal_ref_type = std::vector<std::reference_wrapper<const proposal_object>>;

    typedef deip::protocol::proposal_action_type action_t;

        /** Create proposal object.
     *
     * @param action - type of proposal
     * @param json_data - data attached to particular action type
     * @param initiator - person who promote this proposal
     * @param lifetime - lifetime of proposal !!!(will be changed to end date)
     * @returns proposal object
     */
    const proposal_object& create_proposal(const action_t action,
                                           const std::string json_data,
                                           const account_name_type& initiator,
                                           const research_group_id_type& research_group_id,
                                           const fc::time_point_sec expiration_time,
                                           const share_type quorum_percent,
                                           const size_t object_hash);

    /** Get proposal by id
     */
    const proposal_object& get_proposal(const proposal_id_type& id) const;

    /** Get proposals by research group
     */
    const proposal_ref_type get_proposals_by_research_group_id(const research_group_id_type& research_group_id) const;

    void remove(const proposal_object& proposal);

    void check_proposal_existence(const proposal_id_type& proposal_id) const;

    bool is_expired(const proposal_object& proposal);

    void complete(const proposal_object& proposal);

    void clear_expired_proposals();

    void remove_proposal_votes(const account_name_type& account, const research_group_id_type& research_group_id);
    const proposal_vote_object& vote_for(const proposal_id_type& proposal_id, const account_name_type& voter);

    const proposal_votes_ref_type get_votes_for(const proposal_id_type& proposal_id);

private:
    /* Create proposal vote object
     * @param voter - person who vote
     * @param weight - weight of persons vote
     * @param id - id of proposal
     * */
    const proposal_vote_object& create_vote(const account_name_type& voter,
                                            const share_type weight,
                                            const proposal_id_type& proposal_id,
                                            const research_group_id_type& research_group_id);
};

} // namespace chain
} // namespace deip