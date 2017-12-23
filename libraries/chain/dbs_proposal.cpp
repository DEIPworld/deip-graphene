#include <deip/chain/dbs_proposal.hpp>
#include <deip/chain/database.hpp>
#include <deip/chain/pool/reward_pool.hpp>

namespace deip {
namespace chain {

dbs_proposal::dbs_proposal(database& db)
    : _base_type(db)
{
}

} // namespace chain
} // namespace deip

