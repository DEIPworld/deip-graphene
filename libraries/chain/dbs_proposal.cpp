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
                                       const dbs_proposal::account_t initiator,
                                       const dbs_proposal::lifetime_t lifetime)
{
    const proposal_object& new_proposal = db_impl().create<proposal_object>([&](proposal_object& proposal) {
        proposal.action = action;
        proposal.json = json_data;
        proposal.initiator = initiator;
        proposal.lifetime = lifetime;
    });

    return new_proposal;
}

} // namespace chain
} // namespace deip

