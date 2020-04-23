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
    try { return db_impl().get<proposal_object>(id); }
    FC_CAPTURE_AND_RETHROW((id))
}

const proposal_object& dbs_proposal::get_proposal(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<proposal_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    FC_ASSERT(itr != idx.end(), "Proposal ${1} does not exist.", ("1", external_id));
    return *itr;
}

const dbs_proposal::proposal_optional_ref_type dbs_proposal::get_proposal_if_exists(const external_id_type& external_id) const
{
    proposal_optional_ref_type result;

    const auto& idx = db_impl()
      .get_index<proposal_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const bool dbs_proposal::proposal_exists(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<proposal_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    return itr != idx.end();
}

const dbs_proposal::proposal_ref_type dbs_proposal::get_proposals_by_research_group_id(const account_name_type& proposer) const
{
    proposal_ref_type ret;

    const auto& idx = db_impl()
      .get_index<proposal_index>()
      .indicies()
      .get<by_proposer>();
      
    auto it_pair = idx.equal_range(proposer);
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
        // fc::from_string(proposal.data, json_data);
        // proposal.creator = creator;
        // proposal.research_group_id = research_group_id;
        // proposal.creation_time = db_impl().head_block_time();
        // proposal.expiration_time = expiration_time;
        // proposal.quorum = quorum;
    });

    return new_proposal;
}

void dbs_proposal::remove(const proposal_object& proposal)
{
    db_impl().remove(proposal);
}

const bool dbs_proposal::is_proposal_expired(const proposal_object& proposal) const
{
    return db_impl().head_block_time() > proposal.expiration_time;
}

void dbs_proposal::clear_expired_proposals()
{

}

} // namespace chain
} // namespace deip

