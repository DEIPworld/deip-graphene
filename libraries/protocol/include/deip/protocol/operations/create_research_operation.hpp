#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

using deip::protocol::percent;
using deip::protocol::external_id_type;

struct create_research_operation : public entity_operation
{
    external_id_type external_id;
    account_name_type research_group;
    string title;
    string abstract;
    flat_set<external_id_type> disciplines;
    bool is_private;
    optional<percent> review_share;
    optional<percent> compensation_share;
    /*
      Research participants.
      If the list is not provided, research will include all members within the group.
      If the list is empty, research will not include members. They can be added later using [update_research_operation].
      If the list has entries, they will be validated for membership token existence.
     */
    optional<flat_set<account_name_type>> members;
    
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

FC_REFLECT(deip::protocol::create_research_operation,
  (external_id)
  (research_group)
  (title)
  (abstract)
  (disciplines)
  (is_private)
  (review_share)
  (compensation_share)
  (members)
  (extensions)
)