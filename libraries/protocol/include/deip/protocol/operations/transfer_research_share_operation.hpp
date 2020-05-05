#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

using deip::protocol::percent;

struct transfer_research_share_operation : public base_operation
{
    external_id_type research_external_id;
    account_name_type sender;
    account_name_type receiver;
    percent share;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(sender);
    }
};


}
}

FC_REFLECT(deip::protocol::transfer_research_share_operation,
        (research_external_id)
        (sender)
        (receiver)
        (share)
        (extensions)
)