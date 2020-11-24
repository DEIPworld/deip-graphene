
#include <deip/protocol/deip_operations.hpp>
#include <deip/protocol/operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

struct proposal_nesting_guard
{
    struct proposal_nesting_state
    {
        uint16_t stage = 0;
        uint16_t nested_create_count = 0;
        uint16_t nested_update_count = 0;
        uint16_t nested_delete_count = 0;
    };

    std::map<external_id_type, proposal_nesting_state> nesting_state;
    external_id_type last_stage_id;

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
    }

    // loop and self visit in proposals
    void operator()(const create_proposal_operation& op)
    {
        FC_ASSERT(nesting_state.find(op.external_id) == nesting_state.end(),
          "Duplicated proposal ${1} detected", ("1", op.external_id));

        proposal_nesting_state proposal_state;
        proposal_state.stage = nesting_state.size() + 1;
        nesting_state[op.external_id] = proposal_state;
        last_stage_id = op.external_id;

        // FC_ASSERT(proposal_state.stage <= DEIP_MAX_PROPOSAL_NESTED_STAGE, "Max proposal nesting depth reached");

        for (const op_wrapper& wrap : op.proposed_ops)
        {
            FC_ASSERT(!is_virtual_operation(wrap.op), "Virtual operations can not be nested in proposals");

            if (wrap.op.which() == operation::tag<create_proposal_operation>().value)
            {
                // FC_ASSERT(nesting_state[op.external_id].nested_create_count == 1,
                //   "At most two create_proposal can be nested in a single proposal!");
                nesting_state[op.external_id].nested_create_count++;
            }

            if (wrap.op.which() == operation::tag<update_proposal_operation>().value)
            {
                // FC_ASSERT(nesting_state[op.external_id].nested_update_count == 1, 
                //   "At most two update_proposal can be nested in a single proposal!");
                nesting_state[op.external_id].nested_update_count++;
            }

            if (wrap.op.which() == operation::tag<delete_proposal_operation>().value)
            {
                // FC_ASSERT(nesting_state[op.external_id].nested_delete_count == 1,
                //   "At most two delete_proposal can be nested in a single proposal!");
                nesting_state[op.external_id].nested_delete_count++;
            }

            wrap.op.visit(*this);
        }
    }

    void operator()(const update_proposal_operation& op)
    {
        FC_ASSERT(last_stage_id == op.external_id || nesting_state.find(op.external_id) == nesting_state.end(), 
          "Nested proposal can not update its parent ${1} proposal",
          ("1", op.external_id));
    }

    void operator()(const delete_proposal_operation& op)
    {
        FC_ASSERT(last_stage_id == op.external_id || nesting_state.find(op.external_id) == nesting_state.end(), 
          "Nested proposal can not delete its parent ${1} proposal",
          ("1", op.external_id));
    }
};

void create_proposal_operation::validate() const
{
    validate_account_name(creator);
    FC_ASSERT(!proposed_ops.empty());
    for (const auto& op : proposed_ops)
    {
        operation_validate(op.op);
    }
    // let's keep the nesting as simple as possible for now
    proposal_nesting_guard nesting_guard;
    nesting_guard(*this);
}

} // namespace protocol
} // namespace deip