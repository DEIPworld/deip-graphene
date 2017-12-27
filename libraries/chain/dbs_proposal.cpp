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

} // namespace chain
} // namespace deip

