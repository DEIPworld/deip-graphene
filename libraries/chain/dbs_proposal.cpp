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
                                                     const fc::time_point_sec expiration_time,
                                                     const int quorum_percent)
{
    const proposal_object& new_proposal = db_impl().create<proposal_object>([&](proposal_object& proposal) {
        proposal.action = action;
        proposal.data = json_data;
        proposal.creator = creator;
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

bool dbs_proposal::is_exist(proposal_id_type proposal_id)
{
    auto proposal = db_impl().find<proposal_object, by_id>(proposal_id);
    return proposal != nullptr;
}

size_t dbs_proposal::get_votes(const proposal_object& proposal)
{
    return proposal.voted_accounts.size();
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
                                                      const proposal_id_type id)
{
    const proposal_vote_object& new_proposal_vote = db_impl().create<proposal_vote_object>([&](proposal_vote_object& proposal_vote) {
        proposal_vote.voter = voter;
        proposal_vote.weight = weight;
        proposal_vote.proposal_id = id;
    });

    return new_proposal_vote;
}

} // namespace chain
} // namespace deip

