#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_review_vote::dbs_review_vote(database &db)
        : _base_type(db)
{
}

const review_vote_object& dbs_review_vote::create_review_vote(const external_id_type& external_id,
                                                              const account_name_type& voter,
                                                              const external_id_type& review_external_id,
                                                              const review_id_type& review_id,
                                                              const external_id_type& discipline_external_id,
                                                              const discipline_id_type& discipline_id,
                                                              const uint64_t& weight,
                                                              const time_point_sec& voting_time,
                                                              const time_point_sec& review_time,
                                                              const research_content_id_type& research_content_id,
                                                              const research_id_type& research_id)
{
    uint64_t max_vote_weight = 0;
    const review_vote_object& review_vote = db_impl().create<review_vote_object>([&](review_vote_object& v) {
        v.external_id = external_id; 
        v.voter = voter;
        v.review_external_id = review_external_id;
        v.review_id = review_id;
        v.discipline_external_id = discipline_external_id;
        v.discipline_id = discipline_id;
        v.weight = weight;
        v.voting_time = voting_time;
        v.research_content_id = research_content_id;
        v.research_id = research_id;

        max_vote_weight = v.weight;
        // discount weight by time
        uint128_t w(max_vote_weight);
        const uint64_t delta_t = std::min(
          uint64_t((voting_time - review_time).to_seconds()),
          uint64_t(DEIP_REVERSE_AUCTION_WINDOW_SECONDS)
        );

        w *= delta_t;
        w /= DEIP_REVERSE_AUCTION_WINDOW_SECONDS;
        v.weight = w.to_uint64();
    });

    return review_vote;
}

const review_vote_object& dbs_review_vote::get_review_vote(const review_vote_id_type& id) const
{
    try
    {
        return db_impl().get<review_vote_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const dbs_review_vote::review_vote_optional_ref_type dbs_review_vote::get_review_vote_if_exists(const review_vote_id_type& id) const
{
    review_vote_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<review_vote_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

dbs_review_vote::review_vote_refs_type dbs_review_vote::get_review_votes(const review_id_type& review_id) const
{
    review_vote_refs_type ret;

    const auto& idx = db_impl()
      .get_index<review_vote_index>()
      .indicies()
      .get<by_review_id>();

    auto it_pair = idx.equal_range(review_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_review_vote::review_vote_refs_type dbs_review_vote::get_review_votes(const external_id_type& review_external_id) const
{
    review_vote_refs_type ret;

    const auto& idx = db_impl()
      .get_index<review_vote_index>()
      .indicies()
      .get<by_review_external_id>();

    auto it_pair = idx.equal_range(review_external_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_review_vote::review_vote_refs_type dbs_review_vote::get_review_votes_by_voter(const account_name_type& voter) const
{
    review_vote_refs_type ret;

    auto it_pair = db_impl().get_index<review_vote_index>().indicies().get<by_voter>().equal_range(voter);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_review_vote::review_vote_refs_type dbs_review_vote::get_review_votes_by_review_and_discipline(const review_id_type &review_id,
                                                                                    const discipline_id_type &discipline_id) const
{
    review_vote_refs_type ret;

    auto it_pair = db_impl().get_index<review_vote_index>().indicies().get<by_review_and_discipline>()
            .equal_range(std::make_tuple(review_id, discipline_id));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_review_vote::review_vote_refs_type dbs_review_vote::get_review_votes_by_discipline(const discipline_id_type &discipline_id) const
{
    review_vote_refs_type ret;

    auto it_pair = db_impl().get_index<review_vote_index>().indicies().get<by_discipline_id>().equal_range(discipline_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_review_vote::review_vote_refs_type dbs_review_vote::get_review_votes_by_researh_content(const research_content_id_type& research_content_id) const
{
    review_vote_refs_type ret;

    auto it_pair = db_impl().get_index<review_vote_index>().indicies().get<by_research_content>().equal_range(research_content_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

dbs_review_vote::review_vote_refs_type dbs_review_vote::get_review_votes_by_research(const research_id_type& research_id) const
{
    review_vote_refs_type ret;

    const auto& idx = db_impl()
      .get_index<review_vote_index>()
      .indicies()
      .get<by_research>();

    auto it_pair = idx.equal_range(research_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }
    return ret;
}

bool dbs_review_vote::review_vote_exists_by_voter_and_discipline(const review_id_type& review_id, const account_name_type& voter, const discipline_id_type& discipline_id) const
{
    const auto& idx = db_impl().get_index<review_vote_index>().indices().get<by_voter_discipline_and_review>();
    return idx.find(std::make_tuple(voter, discipline_id, review_id)) != idx.cend();
}

} //namespace chain
} //namespace deip