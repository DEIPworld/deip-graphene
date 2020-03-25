#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/expertise_allocation_proposal_object.hpp>
#include <deip/chain/schema/expertise_allocation_proposal_vote_object.hpp>
#include <deip/chain/schema/expert_token_object.hpp>

namespace deip {
namespace chain {

class dbs_expertise_allocation_proposal : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_expertise_allocation_proposal() = delete;

protected:
    explicit dbs_expertise_allocation_proposal(database &db);

public:

    using expertise_allocation_proposal_refs_type = std::vector<std::reference_wrapper<const expertise_allocation_proposal_object>>;

    const expertise_allocation_proposal_object& create(const account_name_type& claimer,
                                                       const discipline_id_type& discipline_id,
                                                       const string& description);

    const expertise_allocation_proposal_object& get(const expertise_allocation_proposal_id_type& id) const;

    void remove(const expertise_allocation_proposal_id_type& id);

    expertise_allocation_proposal_refs_type get_by_claimer(const account_name_type& claimer) const;

    const expertise_allocation_proposal_object& get_by_claimer_and_discipline(const account_name_type& claimer,
                                                                              const discipline_id_type& discipline_id) const;


    expertise_allocation_proposal_refs_type get_by_discipline_id(const discipline_id_type& discipline_id) const;

    void check_existence_by_claimer_and_discipline(const account_name_type &claimer,
                                                   const discipline_id_type &discipline_id);

    bool exists_by_claimer_and_discipline(const account_name_type &claimer,
                                          const discipline_id_type &discipline_id);

    void upvote(const expertise_allocation_proposal_object &expertise_allocation_proposal,
                const account_name_type &voter,
                const share_type weight);

    void downvote(const expertise_allocation_proposal_object &expertise_allocation_proposal,
                  const account_name_type &voter,
                  const share_type weight);

    bool is_expired(const expertise_allocation_proposal_object& expertise_allocation_proposal);

    bool is_quorum(const expertise_allocation_proposal_object &expertise_allocation_proposal);

    void delete_by_claimer_and_discipline(const account_name_type &claimer,
                                          const discipline_id_type& discipline_id);

    void clear_expired_expertise_allocation_proposals();

    void process_expertise_allocation_proposals();

    /* Expertise allocation proposal vote */

    using expertise_allocation_proposal_vote_refs_type = std::vector<std::reference_wrapper<const expertise_allocation_proposal_vote_object>>;

    const expertise_allocation_proposal_vote_object& create_vote(const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id,
                                                                 const discipline_id_type& discipline_id,
                                                                 const account_name_type &voter,
                                                                 const share_type weight);
    const expertise_allocation_proposal_vote_object& get_vote(const expertise_allocation_proposal_vote_id_type& id) const;

    const expertise_allocation_proposal_vote_object& get_vote_by_voter_and_expertise_allocation_proposal_id(const account_name_type &voter,
                                                                                                            const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id);

    expertise_allocation_proposal_vote_refs_type get_votes_by_expertise_allocation_proposal_id(const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id);

    expertise_allocation_proposal_vote_refs_type get_votes_by_voter_and_discipline_id(const account_name_type& voter,
                                                                                      const discipline_id_type& discipline_id) const;

    expertise_allocation_proposal_vote_refs_type get_votes_by_voter(const account_name_type& voter) const;

    bool vote_exists_by_voter_and_expertise_allocation_proposal_id(const account_name_type &voter,
                                                                      const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id);

    /* Adjusting */

    void adjust_expert_token_vote(const expert_token_object& expert_token, share_type delta);
};

} // namespace chain
} // namespace deip
