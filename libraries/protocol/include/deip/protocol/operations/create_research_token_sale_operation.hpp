#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/asset.hpp>

namespace deip {
namespace protocol {

using deip::protocol::external_id_type;

struct create_research_token_sale_operation : public entity_operation
{
    external_id_type external_id;
    account_name_type research_group;
    external_id_type research_external_id;
    time_point_sec start_time;
    time_point_sec end_time;
    flat_set<asset> security_tokens_on_sale;
    asset soft_cap;
    asset hard_cap;
    
    extensions_type extensions;

    string entity_id() const { return "external_id"; }
    external_id_type get_entity_id() const { return external_id; }

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(research_group);
    }
};


}
}

FC_REFLECT(deip::protocol::create_research_token_sale_operation,
  (external_id)
  (research_group)
  (research_external_id)
  (start_time)
  (end_time)
  (security_tokens_on_sale)
  (soft_cap)
  (hard_cap)
  (extensions)
)