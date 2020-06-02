#include <deip/protocol/deip_operations.hpp>


namespace deip {
namespace protocol {

void update_proposal_operation::validate() const
{
    validate_160_bits_hexadecimal_string(external_id);
    FC_ASSERT(!(active_approvals_to_add.empty() && active_approvals_to_remove.empty() &&
                owner_approvals_to_add.empty() && owner_approvals_to_remove.empty() &&
                key_approvals_to_add.empty() && key_approvals_to_remove.empty()));

    for( auto a : active_approvals_to_add )
    {
        FC_ASSERT(active_approvals_to_remove.find(a) == active_approvals_to_remove.end(),
                  "Cannot add and remove approval at the same time.");
    }
    for( auto a : owner_approvals_to_add )
    {
        FC_ASSERT(owner_approvals_to_remove.find(a) == owner_approvals_to_remove.end(),
                  "Cannot add and remove approval at the same time.");
    }
    for( auto a : key_approvals_to_add )
    {
        FC_ASSERT(key_approvals_to_remove.find(a) == key_approvals_to_remove.end(),
                  "Cannot add and remove approval at the same time.");
    }
}

} /* deip::protocol */
} /* protocol */