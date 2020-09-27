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
    std::vector<authority>& other;

    operation_get_required_auth_visitor(flat_set<account_name_type>& a,
                                        flat_set<account_name_type>& own,
                                        std::vector<authority>& oth)
        : active(a)
        , owner(own)
        , other(oth)
    {
    }

    template <typename T> void operator()(const T& v) const
    {
        v.get_required_active_authorities(active);
        v.get_required_owner_authorities(owner);
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
                                        std::vector<authority>& other)
{
    op.visit(deip::protocol::operation_get_required_auth_visitor(active, owner, other));
}

struct new_account_authority_extractor
{
    typedef void result_type;

    new_account_authority_extractor(flat_map<account_name_type, authority_pack>& accounts_auths)
        : new_accounts_auths(accounts_auths){};

    template <typename T> void operator()(const T&) const
    {
    }

    void operator()(const create_account_operation& op) const
    {
        authority_pack auths;

        auths.owner = op.owner;
        auths.active = op.active;
        auths.active_overrides.insert(op.active_overrides.begin(), op.active_overrides.end());

        new_accounts_auths.insert(std::make_pair(op.new_account_name, auths));
    }

    void operator()(const update_account_operation& op) const
    {
        if (new_accounts_auths.find(op.account) != new_accounts_auths.end())
        {
            FC_ASSERT(false, "Not supported currently");
        }
    }

    void operator()(const request_account_recovery_operation& op) const
    {
        if (new_accounts_auths.find(op.account_to_recover) != new_accounts_auths.end())
        {
            FC_ASSERT(false, "Not supported currently");
        }
    }

    void operator()(const recover_account_operation& op) const
    {
        if (new_accounts_auths.find(op.account_to_recover) != new_accounts_auths.end())
        {
            FC_ASSERT(false, "Not supported currently");
        }
    }

    void operator()(const change_recovery_account_operation& op) const
    {
        if (new_accounts_auths.find(op.account_to_recover) != new_accounts_auths.end())
        {
            FC_ASSERT(false, "Not supported currently");
        }
    }

    private:
        flat_map<account_name_type, authority_pack>& new_accounts_auths;
};

void extract_new_accounts(const vector<operation>& ops, flat_map<account_name_type, authority_pack>& accounts_auths)
{
    const auto& extractor = new_account_authority_extractor(accounts_auths);
    for (const auto& op : ops)
    {
        op.visit(extractor);
    }
}

}
} // deip::protocol

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::operation)