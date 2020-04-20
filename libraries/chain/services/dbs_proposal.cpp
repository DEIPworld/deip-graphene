#include <deip/chain/services/dbs_proposal.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_proposal::dbs_proposal(database& db)
    : _base_type(db)
{
}

const proposal_object& dbs_proposal::get_proposal(const proposal_id_type& id) const
{
    try {
        return db_impl().get<proposal_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const dbs_proposal::proposal_ref_type
dbs_proposal::get_proposals_by_research_group_id(const research_group_id_type& research_group_id) const
{
    proposal_ref_type ret;

    auto it_pair = db_impl().get_index<proposal_index>().indicies().get<by_research_group_id>().equal_range(research_group_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const proposal_object& dbs_proposal::create_proposal(const std::string json_data,
                                                     const account_name_type& creator,
                                                     const research_group_id_type& research_group_id,
                                                     const fc::time_point_sec expiration_time,
                                                     const percent_type quorum)
{
    const proposal_object& new_proposal = db_impl().create<proposal_object>([&](proposal_object& proposal) {
        fc::from_string(proposal.data, json_data);
        proposal.creator = creator;
        proposal.research_group_id = research_group_id;
        proposal.creation_time = db_impl().head_block_time();
        proposal.expiration_time = expiration_time;
        proposal.quorum = quorum;

    });

    return new_proposal;
}

void dbs_proposal::remove(const proposal_object& proposal)
{
    db_impl().remove(proposal);
}

void dbs_proposal::check_proposal_existence(const proposal_id_type& proposal_id) const
{
    const auto& proposal = db_impl().get_index<proposal_index>().indices().get<by_id>();
    FC_ASSERT(proposal.find(proposal_id) != proposal.cend(), "Proposal \"${1}\" does not exist.", ("1", proposal_id));
}

bool dbs_proposal::is_expired(const proposal_object& proposal)
{
    return db_impl().head_block_time() > proposal.expiration_time && !proposal.is_completed;
}

void dbs_proposal::clear_expired_proposals()
{
    const auto& proposal_expiration_index = db_impl().get_index<proposal_index>().indices().get<by_expiration_time>();

    auto block_time = db_impl().head_block_time();
    auto proposal_itr = proposal_expiration_index.upper_bound(block_time);
    auto it = proposal_expiration_index.begin();

    while (it != proposal_itr)
    {
        auto current = it++;
        if (!current->is_completed)
            db_impl().remove(*current);
    }
}

const proposal_vote_object& dbs_proposal::vote_for(const proposal_id_type &proposal_id, const account_name_type &voter)
{
    auto& proposal = get_proposal(proposal_id);

    db_impl().modify(proposal, [&](proposal_object& p) {
        p.voted_accounts.insert(voter);
    });

    return create_vote(voter, proposal_id, proposal.research_group_id);
}

void dbs_proposal::remove_proposal_votes(const account_name_type& account,
                                         const research_group_id_type& research_group_id)
{
    const auto& proposal_votes_idx
            = db_impl().get_index<proposal_vote_index>().indices().get<by_voter>();
    auto proposal_vote_itr = proposal_votes_idx.find(boost::make_tuple(account, research_group_id));

    while(proposal_vote_itr != proposal_votes_idx.end())
    {
        const auto& current_proposal_vote = *proposal_vote_itr;
        auto& proposal = get_proposal(current_proposal_vote.proposal_id);

        db_impl().modify(proposal, [&](proposal_object& p) {
            p.voted_accounts.erase(current_proposal_vote.voter);
        });

        ++proposal_vote_itr;
        db_impl().remove(current_proposal_vote);

    }
}

const proposal_vote_object& dbs_proposal::create_vote(const account_name_type& voter,
                                                      const proposal_id_type& proposal_id,
                                                      const research_group_id_type& research_group_id)
{
    const proposal_vote_object& new_proposal_vote = db_impl().create<proposal_vote_object>([&](proposal_vote_object& proposal_vote) {
        proposal_vote.voter = voter;
        proposal_vote.proposal_id = proposal_id;
        proposal_vote.research_group_id = research_group_id;
        proposal_vote.voting_time = db_impl().head_block_time();;
    });

    return new_proposal_vote;
}

const dbs_proposal::proposal_votes_ref_type  dbs_proposal::get_votes_for(const proposal_id_type &proposal_id) {
    proposal_votes_ref_type ret;

    auto it_pair = db_impl().get_index<proposal_vote_index>().indicies().get<by_proposal_id>().equal_range(proposal_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

} // namespace chain
} // namespace deip

