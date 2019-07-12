#include <deip/protocol/operations.hpp>

#include <deip/protocol/operation_util_impl.hpp>

namespace deip {
namespace protocol {

struct is_market_op_visitor
{
    typedef bool result_type;

    template <typename T> bool operator()(T&& v) const
    {
        return false;
    }
    bool operator()(const transfer_operation&) const
    {
        return true;
    }
    bool operator()(const transfer_to_common_tokens_operation&) const
    {
        return true;
    }
};

bool is_market_operation(const operation& op)
{
    return op.visit(is_market_op_visitor());
}

struct is_vop_visitor
{
    typedef bool result_type;

    template <typename T> bool operator()(const T& v) const
    {
        return v.is_virtual();
    }
};

bool is_virtual_operation(const operation& op)
{
    return op.visit(is_vop_visitor());
}

struct is_ip_protection_op_visitor
{
    typedef bool result_type;

    template <typename T> bool operator()(T&& v) const
    {
        return false;
    }
    bool operator()(const create_proposal_operation& op) const
    {
        if (op.action == deip::protocol::proposal_action_type::create_research_material)
            return true;
        else
            return false;
    }
};

bool is_ip_protection_operation(const operation& op)
{
    return op.visit(is_ip_protection_op_visitor());
}

}
} // deip::protocol

DEFINE_OPERATION_TYPE(deip::protocol::operation)
