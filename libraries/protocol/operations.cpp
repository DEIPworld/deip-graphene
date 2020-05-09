#include <deip/protocol/operations.hpp>
#include <fc/static_variant.hpp>
#include <fc/variant.hpp>

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

struct operation_validate_visitor
{
    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
        v.validate();
    }
};

struct operation_entity_validator
{
    uint16_t ref_block_num;
    uint32_t ref_block_prefix;

    operation_entity_validator(uint16_t block_num, uint32_t block_prefix)
        : ref_block_num(block_num)
        , ref_block_prefix(block_prefix){};

    typedef void result_type;
    template <typename T> void operator()(const T& op) const
    {
        if (!op.entity_id().empty() && !op.ignore_entity_id_validation())
        {
            op.validate_entity_id(op, ref_block_num, ref_block_prefix);
        }
    }
};

struct operation_get_required_auth_visitor
{
    typedef void result_type;

    flat_set<account_name_type>& active;
    flat_set<account_name_type>& owner;
    flat_set<account_name_type>& posting;
    std::vector<authority>& other;

    operation_get_required_auth_visitor(flat_set<account_name_type>& a,
                                        flat_set<account_name_type>& own,
                                        flat_set<account_name_type>& post,
                                        std::vector<authority>& oth)
        : active(a)
        , owner(own)
        , posting(post)
        , other(oth)
    {
    }

    template <typename T> void operator()(const T& v) const
    {
        v.get_required_active_authorities(active);
        v.get_required_owner_authorities(owner);
        v.get_required_posting_authorities(posting);
        v.get_required_authorities(other);
    }
};

void entity_validate(const operation& op, uint16_t ref_block_num, uint32_t ref_block_prefix)
{
    op.visit(operation_entity_validator(ref_block_num, ref_block_prefix));

    if (op.which() == operation::tag<deip::protocol::create_proposal_operation>::value)
    {
        const create_proposal_operation& proposal = op.get<create_proposal_operation>();
        for (const auto& wrap : proposal.proposed_ops)
        {
            entity_validate(wrap.op, ref_block_num, ref_block_prefix);
        }
    }
}

void operation_validate(const operation& op)
{
    op.visit(operation_validate_visitor());
}

void operation_get_required_authorities(const operation& op,
                                        flat_set<account_name_type>& active,
                                        flat_set<account_name_type>& owner,
                                        flat_set<account_name_type>& posting,
                                        std::vector<authority>& other)
{
    op.visit(deip::protocol::operation_get_required_auth_visitor(active, owner, posting, other));
}


}
} // deip::protocol


namespace fc {

using namespace deip::protocol;

std::string op_name_from_type(const std::string& type_name)
{
    auto start = type_name.find_last_of(':') + 1;
    auto end = type_name.find_last_of('_');
    return type_name.substr(start, end - start);
}

struct from_operation
{
    variant& var;
    from_operation(variant& dv)
        : var(dv)
    {
    }

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
        auto name = op_name_from_type(fc::get_typename<T>::name());
        var = variant(std::make_pair(name, v));
    }
};

struct get_operation_name
{
    string& name;
    get_operation_name(string& dv)
        : name(dv)
    {
    }

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
        name = op_name_from_type(fc::get_typename<T>::name());
    }
};

void to_variant(const operation& var, fc::variant& vo)
{
    var.visit(from_operation(vo));
}

void from_variant(const fc::variant& var, operation& vo)
{
    static std::map<string, uint32_t> to_tag = []() {
        std::map<string, uint32_t> name_map;
        for (int i = 0; i < operation::count(); ++i)
        {
            operation tmp;
            tmp.set_which(i);
            string n;
            tmp.visit(get_operation_name(n));
            name_map[n] = i;
        }
        return name_map;
    }();

    auto ar = var.get_array();
    if (ar.size() < 2)
        return;
    if (ar[0].is_uint64())
        vo.set_which(ar[0].as_uint64());
    else
    {
        auto itr = to_tag.find(ar[0].as_string());
        FC_ASSERT(itr != to_tag.end(), "Invalid operation name: ${n}", ("n", ar[0]));
        vo.set_which(to_tag[ar[0].as_string()]);
    }
    vo.visit(fc::to_static_variant(ar[1]));
}

} // namespace fc