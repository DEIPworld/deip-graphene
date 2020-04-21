#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <deip/protocol/asset.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct create_research_token_sale_operation : public base_operation
{
    account_name_type creator;
    external_id_type research_group_external_id;

    external_id_type research_external_id;
    time_point_sec start_time;
    time_point_sec end_time;
    share_type amount_for_sale;
    asset soft_cap;
    asset hard_cap;

    time_point_sec expiration_time;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(creator);
    }
};


}
}

FC_REFLECT(deip::protocol::create_research_token_sale_operation,
          (creator)
          (research_group_external_id)
          (research_external_id)
          (start_time)
          (end_time)
          (amount_for_sale)
          (soft_cap)
          (hard_cap)
          (expiration_time)
          (extensions)
)