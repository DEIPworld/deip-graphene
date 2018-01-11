#pragma once

#include <deip/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/vote_object.hpp>

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
    /** Create research vote object
    *
    * @returns new vote object
    */
    const vote_object& create_research_vote(const discipline_id_type& discipline_id,
                    const account_name_type& voter,
                    const int64_t& research_id,
                    const share_type& weight,
                    const time_point_sec& voting_time);

    /** Create research material vote object
    *
    * @returns new vote object
    */
    const vote_object& create_material_vote(const discipline_id_type& discipline_id,
                                      const account_name_type& voter,
                                      const int64_t& material_id,
                                      const share_type& weight,
                                      const time_point_sec& voting_time);

    /** Create review vote object
    *
    * @returns new vote object
    */
    const vote_object& create_review_vote(const discipline_id_type& discipline_id,
                                      const account_name_type& voter,
                                      const int64_t& review_id,
                                      const share_type& weight,
                                      const time_point_sec& voting_time);

private:
    const vote_object& create_vote(const discipline_id_type& discipline_id,
                                   const account_name_type& voter,
                                   const share_type& weight,
                                   const time_point_sec& voting_time);
};
} // namespace chain
} // namespace deip
