#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

struct update_research_operation : public base_operation
{
    account_name_type research_group;
    external_id_type external_id;
    string title;
    string abstract;
    string permlink;
    bool is_private;
    percent review_share;
    percent compensation_share;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(research_group);
    }
};


}
}

FC_REFLECT(deip::protocol::update_research_operation,
  (research_group)
  (external_id)
  (title)
  (abstract)
  (permlink)
  (is_private)
  (review_share)
  (compensation_share)
  (extensions)
)
