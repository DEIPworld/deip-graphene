#include <deip/chain/dbs_proposal.hpp>
#include <deip/chain/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_proposal::dbs_proposal(database& db)
    : _base_type(db)
{
}

const proposal_object& dbs_proposal::get_proposal(proposal_id_type id) const
{
    return db_impl().get<proposal_object>(id);
}

const proposal_object& dbs_proposal::create_proposal(const dbs_proposal::action_t action,
                                                     const std::string json_data,
                                                     const dbs_proposal::account_t creator,
                                                     const research_group_id_type research_group_id,
                                                     const fc::time_point_sec expiration_time,
                                                     const u_int16_t quorum_percent)
{
    const proposal_object& new_proposal = db_impl().create<proposal_object>([&](proposal_object& proposal) {
        proposal.action = action;
        proposal.data = json_data;
        proposal.creator = creator;
        proposal.research_group_id = research_group_id;
        proposal.creation_time = fc::time_point_sec();
        proposal.expiration_time = expiration_time;
        proposal.quorum_percent = quorum_percent;
    });

    return new_proposal;
}

void dbs_proposal::remove(const proposal_object& proposal)
{
    db_impl().remove(proposal);
}

void dbs_proposal::check_proposal_existance(proposal_id_type proposal_id) const
{
    const auto& proposal = db_impl().get_index<proposal_index>().indices().get<by_id>();
    FC_ASSERT(proposal.find(proposal_id) != proposal.cend(), "Proposal \"${1}\" does not exist.", ("1", proposal_id));
}

bool dbs_proposal::is_expired(const proposal_object& proposal)
{
    return _get_now() > proposal.expiration_time;
}

void dbs_proposal::clear_expired_proposals()
{
    const auto& proposal_expiration_index = db_impl().get_index<proposal_index>().indices().get<by_expiration_time>();

    while (!proposal_expiration_index.empty() && is_expired(*proposal_expiration_index.begin()))
    {
        db_impl().remove(*proposal_expiration_index.begin());
    }
}

const proposal_vote_object& dbs_proposal::create_vote(const dbs_proposal::account_t voter,
                                                      const deip::chain::share_type weight,
                                                      const proposal_id_type id,
                                                      const research_group_id_type research_group_id)
{
    const proposal_vote_object& new_proposal_vote = db_impl().create<proposal_vote_object>([&](proposal_vote_object& proposal_vote) {
        proposal_vote.voter = voter;
        proposal_vote.weight = weight;
        proposal_vote.proposal_id = id;
        proposal_vote.research_group_id = research_group_id;
    });

    return new_proposal_vote;
}

void dbs_proposal::remove_proposal_votes(const account_name_type account_t,
                                                  const research_group_id_type research_group_id)
{
    const auto& proposal_votes_idx
            = db_impl().get_index<proposal_vote_index>().indices().get<by_voter>();
    auto proposal_vote_itr = proposal_votes_idx.find(boost::make_tuple(account_t, research_group_id));

    while(proposal_vote_itr != proposal_votes_idx.end())
    {
        const auto& current_proposal_vote = *proposal_vote_itr;
        ++proposal_vote_itr;
        db_impl().remove(current_proposal_vote);
    }
}

} // namespace chain
} // namespace deip

