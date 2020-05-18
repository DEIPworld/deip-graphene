#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/asset.hpp>

namespace deip {
namespace protocol {

struct create_research_token_sale_operation : public base_operation
{
    account_name_type research_group;
    external_id_type research_external_id;
    time_point_sec start_time;
    time_point_sec end_time;
    percent share;
    asset soft_cap;
    asset hard_cap;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(research_group);
    }
};


}
}

FC_REFLECT(deip::protocol::create_research_token_sale_operation,
  (research_group)
  (research_external_id)
  (start_time)
  (end_time)
  (share)
  (soft_cap)
  (hard_cap)
  (extensions)
)