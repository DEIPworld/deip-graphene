
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
        uint16_t depth = 0;
        uint16_t nested_create_count = 0;
        uint16_t nested_update_count = 0;
        uint16_t nested_delete_count = 0;
    };

    std::map<external_id_type, proposal_nesting_state> nesting_state;

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
    }

    // loop and self visit in proposals
    void operator()(const create_proposal_operation& op)
    {
        FC_ASSERT(nesting_state.find(op.external_id) == nesting_state.end(),
          "Ambiguous proposal ${1} detected", ("1",  op.external_id));

        proposal_nesting_state proposal_state;
        proposal_state.depth = nesting_state.size() + 1;
        nesting_state[op.external_id] = proposal_state;

        for (const op_wrapper& wrap : op.proposed_ops)
        {
            if (wrap.op.which() == operation::tag<create_proposal_operation>().value)
            {
                // Do not allow more than 1 create_proposal_operation in a proposal
                FC_ASSERT(nesting_state[op.external_id].nested_create_count == 0,
                  "At most one proposal create can be nested in a single proposal!");
                nesting_state[op.external_id].nested_create_count++;
            }

            if (wrap.op.which() == operation::tag<update_proposal_operation>().value)
            {
                const auto& update_proposal_op = wrap.op.get<update_proposal_operation>();
                FC_ASSERT(std::none_of(op.proposed_ops.begin(), op.proposed_ops.end(), [&](const op_wrapper& wrap) {
                    if (wrap.op.which() == operation::tag<create_proposal_operation>().value)
                    {
                        const auto& create_proposal_op = wrap.op.get<create_proposal_operation>();
                        return update_proposal_op.external_id == create_proposal_op.external_id;
                    }
                    if (wrap.op.which() == operation::tag<delete_proposal_operation>().value)
                    {
                        const auto& delete_proposal_op = wrap.op.get<delete_proposal_operation>();
                        return update_proposal_op.external_id == delete_proposal_op.external_id;
                    }
                    return false;
                }), "Nested proposal can not affect its sibling ${1} proposal", ("1", update_proposal_op.external_id));

                // Do not allow more than 1 update_proposal_operation in a proposal
                FC_ASSERT(nesting_state[op.external_id].nested_update_count == 0, 
                  "At most one proposal update can be nested in a single proposal!");
                nesting_state[op.external_id].nested_update_count++;
            }

            if (wrap.op.which() == operation::tag<delete_proposal_operation>().value)
            {
                const auto& delete_proposal_op = wrap.op.get<delete_proposal_operation>();
                FC_ASSERT(std::none_of(op.proposed_ops.begin(), op.proposed_ops.end(), [&](const op_wrapper& wrap) {
                    if (wrap.op.which() == operation::tag<create_proposal_operation>().value)
                    {
                        const auto& create_proposal_op = wrap.op.get<create_proposal_operation>();
                        return delete_proposal_op.external_id == create_proposal_op.external_id;
                    }
                    if (wrap.op.which() == operation::tag<update_proposal_operation>().value)
                    {
                        const auto& update_proposal_op = wrap.op.get<update_proposal_operation>();
                        return delete_proposal_op.external_id == update_proposal_op.external_id;
                    }
                    return false;
                }), "Nested proposal can not affect its sibling ${1} proposal", ("1", delete_proposal_op.external_id));

                // Do not allow more than 1 update_proposal_operation in a proposal
                FC_ASSERT(nesting_state[op.external_id].nested_delete_count == 0,
                  "At most one proposal delete can be nested in a single proposal!");
                nesting_state[op.external_id].nested_delete_count++;
            }

            wrap.op.visit(*this);
        }
    }

    void operator()(const update_proposal_operation& op)
    {
        FC_ASSERT(nesting_state.find(op.external_id) == nesting_state.end(), 
          "Nested proposal can not update its parent ${1} proposal",
          ("1", op.external_id));
    }

    void operator()(const delete_proposal_operation& op)
    {
        FC_ASSERT(nesting_state.find(op.external_id) == nesting_state.end(), 
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