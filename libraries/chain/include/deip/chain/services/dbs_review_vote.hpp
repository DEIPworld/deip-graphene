#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/expertise_contribution_object.hpp>
#include <deip/chain/schema/review_vote_object.hpp>

namespace deip {
namespace chain {

///** DB service for operations with vote_object
// *  --------------------------------------------
// */
class dbs_review_vote : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_review_vote() = delete;

protected:
    explicit dbs_review_vote(database &db);

public:

    using review_vote_refs_type = std::vector<std::reference_wrapper<const review_vote_object>>;

    const review_vote_object& create_review_vote(const account_name_type& voter,
                                                 const review_id_type& review_id,
                                                 const discipline_id_type& discipline_id,
                                                 const uint64_t& weight,
                                                 const time_point_sec& voting_time,
                                                 const time_point_sec& review_time,
                                                 const research_content_id_type& research_content_id,
                                                 const research_id_type& research_id);

    review_vote_refs_type get_review_votes(const review_id_type& review_id) const;

    review_vote_refs_type get_review_votes_by_voter(const account_name_type& voter) const;

    review_vote_refs_type get_review_votes_by_review_and_discipline(const review_id_type &review_id, const discipline_id_type &discipline_id) const;

    review_vote_refs_type get_review_votes_by_discipline(const discipline_id_type &discipline_id) const;

    review_vote_refs_type get_review_votes_by_researh_content(const research_content_id_type& research_content_id) const;

    review_vote_refs_type get_review_votes_by_research(const research_id_type& research_id) const;

    bool review_vote_exists_by_voter_and_discipline(const review_id_type& review_id, const account_name_type& voter, const discipline_id_type& discipline_id) const;
};
} // namespace chain
} // namespace deip
