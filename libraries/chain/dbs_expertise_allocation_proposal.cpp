#include <deip/chain/dbs_expertise_allocation_proposal.hpp>
#include <deip/chain/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_expertise_allocation_proposal::dbs_expertise_allocation_proposal(database &db)
    : _base_type(db)
{
}

const expertise_allocation_proposal_object& dbs_expertise_allocation_proposal::create(const account_name_type& initiator,
                                                                                      const account_name_type& claimer,
                                                                                      const discipline_id_type& discipline_id)
{
    auto& expertise_allocation_proposal = db_impl().create<expertise_allocation_proposal_object>([&](expertise_allocation_proposal_object& eap_o) {
        eap_o.initiator = initiator;
        eap_o.claimer = claimer;
        eap_o.discipline_id = discipline_id;
        eap_o.creation_time = db_impl().head_block_time();
        eap_o.expiration_time = db_impl().head_block_time() + DAYS_TO_SECONDS(14);
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

const expertise_allocation_proposal_object& dbs_expertise_allocation_proposal::get_by_discipline_and_claimer(const discipline_id_type& discipline_id,
                                                                                                             const account_name_type& claimer) const
{
    try {
        return db_impl().get<expertise_allocation_proposal_object, by_discipline_and_claimer>(std::make_tuple(discipline_id, claimer));
    }
    FC_CAPTURE_AND_RETHROW((discipline_id)(claimer))
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

void dbs_expertise_allocation_proposal::check_existence_by_discipline_and_claimer(const discipline_id_type& discipline_id,
                                                                                  const account_name_type& claimer)
{
    const auto& idx = db_impl().get_index<expertise_allocation_proposal_index>().indices().get<by_discipline_and_claimer>();

    FC_ASSERT(idx.find(std::make_tuple(discipline_id, claimer)) != idx.cend(),
              "Expertise allocation proposal for discipline \"${1}\" and clainer \"${2}\" does not exist", ("1", discipline_id)("2", claimer));
}

bool dbs_expertise_allocation_proposal::is_exists_by_discipline_and_claimer(const discipline_id_type& discipline_id,
                                                                            const account_name_type& claimer)
{
    const auto& idx = db_impl().get_index<expertise_allocation_proposal_index>().indices().get<by_discipline_and_claimer>();
    return idx.find(boost::make_tuple(discipline_id, claimer)) != idx.cend();
}

void dbs_expertise_allocation_proposal::increase_total_voted_expertise(const expertise_allocation_proposal_object& expertise_allocation_proposal,
                                                                       const account_name_type &voter,
                                                                       const share_type &amount){
    FC_ASSERT(amount >= 0, "Amount cannot be <= 0");

    db_impl().modify(expertise_allocation_proposal, [&](expertise_allocation_proposal_object eap_o) {
        if (std::find(eap_o.downvoted_accounts.begin(), eap_o.downvoted_accounts.end(), voter) != eap_o.downvoted_accounts.end())
        {
            eap_o.downvoted_accounts.erase(std::remove(eap_o.downvoted_accounts.begin(), eap_o.downvoted_accounts.end(), voter), eap_o.downvoted_accounts.end());
            eap_o.upvoted_accounts.push_back(voter);
            eap_o.total_voted_expertise += 2 * amount;
        } else{
            eap_o.upvoted_accounts.push_back(voter);
            eap_o.total_voted_expertise += amount;
        }
    });
}

void dbs_expertise_allocation_proposal::decrease_total_voted_expertise(const expertise_allocation_proposal_object& expertise_allocation_proposal,
                                                                       const account_name_type &voter,
                                                                       const share_type &amount){
    FC_ASSERT(amount <= 0, "Amount cannot be >= 0");

    db_impl().modify(expertise_allocation_proposal, [&](expertise_allocation_proposal_object eap_o) {
        if (std::find(eap_o.upvoted_accounts.begin(), eap_o.upvoted_accounts.end(), voter) != eap_o.upvoted_accounts.end())
        {
            eap_o.upvoted_accounts.erase(std::remove(eap_o.upvoted_accounts.begin(), eap_o.upvoted_accounts.end(), voter), eap_o.upvoted_accounts.end());
            eap_o.downvoted_accounts.push_back(voter);
            eap_o.total_voted_expertise -= 2 * amount;
        } else{
            eap_o.downvoted_accounts.push_back(voter);
            eap_o.total_voted_expertise -= amount;
        }
    });
}

} //namespace chain
} //namespace deip