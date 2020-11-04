#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct reserve_asset_operation : public base_operation
{
    account_name_type owner;
    asset amount;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(owner);
    }
};

} // namespace protocol
} // namespace deip

FC_REFLECT(deip::protocol::reserve_asset_operation, (owner)(amount)(extensions))
