#pragma once

#include <deip/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/vote_object.hpp>
#include <deip/chain/total_votes_object.hpp>
#include <deip/chain/review_vote_object.hpp>

namespace deip {
namespace chain {

///** DB service for operations with vote_object
// *  --------------------------------------------
// */
class dbs_vote : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_vote() = delete;

protected:
    explicit dbs_vote(database &db);

public:
    using vote_refs_type = std::vector<std::reference_wrapper<const vote_object>>;

    vote_refs_type get_votes_by_discipline(const discipline_id_type& discipline_id) const;
    vote_refs_type get_votes_by_voter(const account_name_type& voter) const;
    vote_refs_type get_votes_by_research(const research_id_type& research_id) const;
    vote_refs_type get_votes_by_research_content(const research_content_id_type& research_content_id) const;
    vote_refs_type get_votes_by_research_and_discipline(const research_id_type& research_id,
                                                        const discipline_id_type& discipline_id) const;
    vote_refs_type get_votes_by_research_content_and_discipline(const research_content_id_type& research_content_id,
                                                                const discipline_id_type& discipline_id) const;

    const vote_object& create_vote(const discipline_id_type& discipline_id,
                                   const account_name_type& voter,
                                   const research_id_type& research_id,
                                   const research_content_id_type& research_content_id,
                                   const share_type& tokens_amount,
                                   const int16_t& weight,
                                   const uint16_t& voting_power,
                                   const time_point_sec& voting_time);

    // total_votes_object

    using total_votes_refs_type = std::vector<std::reference_wrapper<const total_votes_object>>;

    const total_votes_object& create_total_votes(const discipline_id_type& discipline_id,
                                                 const research_id_type& research_id,
                                                 const research_content_id_type& research_content_id);

    const total_votes_object& get_total_votes_by_content_and_discipline(const research_content_id_type& research_content_id,
                                                                               const discipline_id_type& discipline_id) const;
    total_votes_refs_type get_total_votes_by_research_and_discipline(const research_id_type& research_id,
                                                                               const discipline_id_type& discipline_id) const;
    total_votes_refs_type get_total_votes_by_discipline(const discipline_id_type& discipline_id) const;

    const total_votes_object& update_total_votes(const total_votes_object& total_votes, const share_type total_weight);

    total_votes_refs_type get_total_votes_by_content(const research_content_id_type& research_content_id) const;

    total_votes_refs_type get_total_votes_by_research(const research_id_type& research_id) const;

    bool is_exists_by_research_and_discipline(const research_content_id_type& research_content_id, const discipline_id_type& discipline_id);

    // review_votes_object
    using review_vote_refs_type = std::vector<std::reference_wrapper<const review_vote_object>>;

    const review_vote_object &create_review_vote(const review_id_type &review_id, const discipline_id_type &discipline_id,
                                                     const account_name_type &voter, const int16_t &weight,
                                                     const time_point_sec &voting_time);

    review_vote_refs_type get_review_votes(const review_id_type& review_id) const;
    review_vote_refs_type get_review_votes_by_voter(const account_name_type& voter) const;
    review_vote_refs_type get_review_votes_by_review_and_discipline(const review_id_type &review_id,
                                                                    const discipline_id_type &discipline_id) const;
    review_vote_refs_type get_review_votes_by_discipline(const discipline_id_type &discipline_id) const;

};
} // namespace chain
} // namespace deip
