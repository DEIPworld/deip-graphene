#include <deip/chain/services/dbs_expertise_allocation_proposal.hpp>
#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_expertise_allocation_proposal::dbs_expertise_allocation_proposal(database &db)
    : _base_type(db)
{
}

const expertise_allocation_proposal_object& dbs_expertise_allocation_proposal::create(const account_name_type& claimer,
                                                                                      const discipline_id_type& discipline_id,
                                                                                      const string& description)
{
    auto& expertise_allocation_proposal = db_impl().create<expertise_allocation_proposal_object>([&](expertise_allocation_proposal_object& eap_o) {
        eap_o.claimer = claimer;
        eap_o.discipline_id = discipline_id;
        eap_o.quorum = 15 * DEIP_1_PERCENT;
        eap_o.creation_time = db_impl().head_block_time();
        eap_o.expiration_time = db_impl().head_block_time() + DAYS_TO_SECONDS(14);
        fc::from_string(eap_o.description, description);
    });

    return expertise_allocation_proposal;
}

const expertise_allocation_proposal_object& dbs_expertise_allocation_proposal::get(const expertise_allocation_proposal_id_type& id) const
{
    try {
        return db_impl().get<expertise_allocation_proposal_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

dbs_expertise_allocation_proposal::expertise_allocation_proposal_refs_type
dbs_expertise_allocation_proposal::get_by_claimer(const account_name_type& claimer) const
{
    expertise_allocation_proposal_refs_type ret;

    auto it_pair = db_impl().get_index<expertise_allocation_proposal_index>().indicies().get<by_claimer>().equal_range(claimer);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const expertise_allocation_proposal_object&
dbs_expertise_allocation_proposal::get_by_claimer_and_discipline(const account_name_type& claimer,
                                                                 const discipline_id_type& discipline_id) const
{
    const auto& idx = db_impl().get_index<expertise_allocation_proposal_index>().indices().get<by_claimer_and_discipline>();
    auto itr = idx.find(std::make_tuple(claimer, discipline_id));

    return *itr;
}

dbs_expertise_allocation_proposal::expertise_allocation_proposal_refs_type dbs_expertise_allocation_proposal::get_by_discipline_id(const discipline_id_type& discipline_id) const
{
    expertise_allocation_proposal_refs_type ret;

    auto it_pair = db_impl().get_index<expertise_allocation_proposal_index>().indicies().get<by_discipline_id>().equal_range(discipline_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_expertise_allocation_proposal::check_existence_by_claimer_and_discipline(const account_name_type &claimer,
                                                                                  const discipline_id_type &discipline_id)
{
    const auto& idx = db_impl().get_index<expertise_allocation_proposal_index>().indices().get<by_claimer_and_discipline>();

    FC_ASSERT(idx.find(std::make_tuple(claimer, discipline_id)) != idx.cend(),
              "Expertise allocation proposal for clainer \"${1}\"  and discipline \"${2}\" does not exist", ("1", claimer)("2", discipline_id));
}

bool dbs_expertise_allocation_proposal::exists_by_claimer_and_discipline(const account_name_type &claimer,
                                                                         const discipline_id_type& discipline_id)
{
    const auto& idx = db_impl().get_index<expertise_allocation_proposal_index>().indices().get<by_claimer_and_discipline>();
    return idx.find(boost::make_tuple(claimer, discipline_id)) != idx.cend();
}

void dbs_expertise_allocation_proposal::upvote(const expertise_allocation_proposal_object& expertise_allocation_proposal,
                                               const account_name_type &voter,
                                               const share_type weight)
{
    FC_ASSERT(weight > 0, "Weight must be greater than zero");

    if (vote_exists_by_voter_and_expertise_allocation_proposal_id(voter, expertise_allocation_proposal.id))
    {
        auto& vote = get_vote_by_voter_and_expertise_allocation_proposal_id(voter, expertise_allocation_proposal.id);
        const bool& is_negative_vote = vote.weight < 0;
        
        FC_ASSERT(is_negative_vote, "User \"${1}\" has voted positively for proposal \"${2}\" already", ("1", voter)("2", expertise_allocation_proposal.id));

        db_impl().modify(expertise_allocation_proposal, [&](expertise_allocation_proposal_object& eap_o) {
            eap_o.total_voted_expertise += (std::abs(vote.weight.value) + weight.value);
        });
        db_impl().modify(vote, [&](expertise_allocation_proposal_vote_object& eapv_o) {
            eapv_o.weight = weight.value;
        });
    }
    else
    {
        create_vote(expertise_allocation_proposal.id, expertise_allocation_proposal.discipline_id, voter, weight);
        db_impl().modify(expertise_allocation_proposal, [&](expertise_allocation_proposal_object& eap_o) {
            eap_o.total_voted_expertise += weight.value;
        });
    }
}

void dbs_expertise_allocation_proposal::downvote(const expertise_allocation_proposal_object& expertise_allocation_proposal,
                                                 const account_name_type &voter,
                                                 const share_type weight)
{
    FC_ASSERT(weight > 0, "Weight must be greater than zero");

    if (vote_exists_by_voter_and_expertise_allocation_proposal_id(voter, expertise_allocation_proposal.id))
    {
        auto& vote = get_vote_by_voter_and_expertise_allocation_proposal_id(voter, expertise_allocation_proposal.id);
        const bool& is_positive_vote = vote.weight > 0;

        FC_ASSERT(is_positive_vote, "User \"${1}\" has voted negatively for proposal \"${2}\" already", ("1", voter)("2", expertise_allocation_proposal.id));

        db_impl().modify(expertise_allocation_proposal, [&](expertise_allocation_proposal_object& eap_o) {
            eap_o.total_voted_expertise -= (std::abs(vote.weight.value) + weight.value);
        });
        db_impl().modify(vote, [&](expertise_allocation_proposal_vote_object& eapv_o) {
            eapv_o.weight = -weight.value;
        });
    }
    else
    {
        create_vote(expertise_allocation_proposal.id, expertise_allocation_proposal.discipline_id, voter, -weight);
        db_impl().modify(expertise_allocation_proposal, [&](expertise_allocation_proposal_object& eap_o) {
            eap_o.total_voted_expertise -= weight.value;
        });
    }
}

bool dbs_expertise_allocation_proposal::is_quorum(const expertise_allocation_proposal_object &expertise_allocation_proposal)
{
    auto& discipline = db_impl().get<discipline_object, by_id>(expertise_allocation_proposal.discipline_id);
    if (discipline.total_expertise_amount == 0) // for now let's wait for someone who has expertise
        return false;
    
    auto quorum_amount = (expertise_allocation_proposal.quorum * discipline.total_expertise_amount) / DEIP_100_PERCENT;
    return expertise_allocation_proposal.total_voted_expertise >= quorum_amount.value;
}

void dbs_expertise_allocation_proposal::delete_by_claimer_and_discipline(const account_name_type &claimer, 
                                                                         const discipline_id_type& discipline_id)
{
    const auto& idx = db_impl().get_index<expertise_allocation_proposal_index>().indices().get<by_claimer_and_discipline>();
    auto it_pair = idx.equal_range(boost::make_tuple(claimer, discipline_id));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;

    while (it != it_end)
    {
        const auto& current_proposal = *it;
        ++it;
        db_impl().remove(current_proposal);
    }
}


void dbs_expertise_allocation_proposal::clear_expired_expertise_allocation_proposals()
{
    const auto& expiration_index = db_impl().get_index<expertise_allocation_proposal_index>().indices().get<by_expiration_time>();
    while (!expiration_index.empty() && is_expired(*expiration_index.begin()))
        db_impl().remove(*expiration_index.begin());
}

const expertise_allocation_proposal_vote_object& dbs_expertise_allocation_proposal::create_vote(const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id,
                                                                                                const discipline_id_type& discipline_id,
                                                                                                const account_name_type &voter,
                                                                                                const share_type weight)
{
    auto& expertise_allocation_proposal_vote = db_impl().create<expertise_allocation_proposal_vote_object>([&](expertise_allocation_proposal_vote_object& eapv_o) {
        eapv_o.expertise_allocation_proposal_id = expertise_allocation_proposal_id;
        eapv_o.discipline_id = discipline_id;
        eapv_o.voter = voter;
        eapv_o.weight = weight;
        eapv_o.voting_time = db_impl().head_block_time();
    });

    return expertise_allocation_proposal_vote;
}

const expertise_allocation_proposal_vote_object& dbs_expertise_allocation_proposal::get_vote(const expertise_allocation_proposal_vote_id_type& id) const
{
    try {
        return db_impl().get<expertise_allocation_proposal_vote_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const expertise_allocation_proposal_vote_object&
dbs_expertise_allocation_proposal::get_vote_by_voter_and_expertise_allocation_proposal_id(const account_name_type &voter,
                                                                                          const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id)
{
    try {
        return db_impl().get<expertise_allocation_proposal_vote_object, by_voter_and_expertise_allocation_proposal_id>(
                boost::make_tuple(voter, expertise_allocation_proposal_id));
    }
    FC_CAPTURE_AND_RETHROW((voter)(expertise_allocation_proposal_id))
}

dbs_expertise_allocation_proposal::expertise_allocation_proposal_vote_refs_type
dbs_expertise_allocation_proposal::get_votes_by_expertise_allocation_proposal_id(const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id)
{
    expertise_allocation_proposal_vote_refs_type ret;

    auto it_pair = db_impl().get_index<expertise_allocation_proposal_vote_index>().indicies().get<by_expertise_allocation_proposal_id>().equal_range(expertise_allocation_proposal_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_expertise_allocation_proposal::
expertise_allocation_proposal_vote_refs_type dbs_expertise_allocation_proposal::get_votes_by_voter_and_discipline_id(const account_name_type& voter,
                                                                                                                     const discipline_id_type& discipline_id) const
{
    expertise_allocation_proposal_vote_refs_type ret;

    auto it_pair = db_impl().get_index<expertise_allocation_proposal_vote_index>().indicies().get<by_voter_and_discipline_id>().equal_range(std::make_tuple(voter, discipline_id));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_expertise_allocation_proposal::expertise_allocation_proposal_vote_refs_type
dbs_expertise_allocation_proposal::get_votes_by_voter(const account_name_type& voter) const
{
    expertise_allocation_proposal_vote_refs_type ret;

    auto it_pair = db_impl().get_index<expertise_allocation_proposal_vote_index>().indicies().get<by_voter>().equal_range(voter);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

bool dbs_expertise_allocation_proposal::vote_exists_by_voter_and_expertise_allocation_proposal_id(const account_name_type &voter,
                                                                                                     const expertise_allocation_proposal_id_type& expertise_allocation_proposal_id)
{
    const auto& idx = db_impl().get_index<expertise_allocation_proposal_vote_index>().indices().get<by_voter_and_expertise_allocation_proposal_id>();
    return idx.find(boost::make_tuple(voter, expertise_allocation_proposal_id)) != idx.cend();
}

void dbs_expertise_allocation_proposal::adjust_expert_token_vote(const expert_token_object& expert_token, share_type delta)
{
    const auto& idx = db_impl().get_index<expertise_allocation_proposal_vote_index>().indices().get<by_voter_and_discipline_id>();
    if (idx.find(boost::make_tuple(expert_token.account_name, expert_token.discipline_id)) != idx.cend())
    {
        auto& vote = db_impl().get<expertise_allocation_proposal_vote_object, by_voter_and_discipline_id>(boost::make_tuple(expert_token.account_name,
                                                                                                                            expert_token.discipline_id));
        db_impl().modify(vote, [&](expertise_allocation_proposal_vote_object& eapv_o) {
            eapv_o.weight += delta.value;
        });

        auto& proposal = db_impl().get<expertise_allocation_proposal_object>(vote.expertise_allocation_proposal_id);

        db_impl().modify(proposal, [&](expertise_allocation_proposal_object& eap_o) {
            eap_o.total_voted_expertise += delta.value;
        });
    }
}

bool dbs_expertise_allocation_proposal::is_expired(const expertise_allocation_proposal_object& eap_o)
{
    return eap_o.expiration_time < _get_now();
}

void dbs_expertise_allocation_proposal::remove(const expertise_allocation_proposal_id_type& id)
{
    const expertise_allocation_proposal_object& proposal = get(id);
    db_impl().remove(proposal);
}

} //namespace chain
} //namespace deip