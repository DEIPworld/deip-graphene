#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct issue_asset_operation : public base_operation
{
    account_name_type issuer;
    asset amount;
    account_name_type recipient;
    optional<string> memo;
    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(issuer);
    }
};



} // namespace protocol
} // namespace deip

FC_REFLECT(deip::protocol::issue_asset_operation, (issuer)(amount)(recipient)(memo)(extensions))