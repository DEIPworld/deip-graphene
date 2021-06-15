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
    account_name_type account;
    string description;
    flat_set<external_id_type> disciplines;
    bool is_private;
    optional<percent> review_share; // deprecated
    optional<percent> compensation_share; // deprecated
    optional<flat_set<account_name_type>> members; // deprecated
    
    extensions_type extensions;

    string entity_id() const { return "external_id"; }
    external_id_type get_entity_id() const { return external_id; }

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(account);
    }
};


}
}

FC_REFLECT(deip::protocol::create_research_operation,
  (external_id)
  (account)
  (description)
  (disciplines)
  (is_private)
  (review_share) // deprecated
  (compensation_share) // deprecated
  (members) // deprecated
  (extensions)
)