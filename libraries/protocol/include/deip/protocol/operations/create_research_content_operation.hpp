#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

struct create_research_content_operation : public entity_operation
{
    external_id_type external_id;
    external_id_type research_external_id;
    account_name_type research_group;
    uint16_t type;
    string title;
    string content;
    flat_set<account_name_type> authors;
    flat_set<external_id_type> references;
    flat_set<string> foreign_references;
    
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

FC_REFLECT(deip::protocol::create_research_content_operation,
  (external_id)
  (research_external_id)
  (research_group)
  (type)
  (title)
  (content)
  (authors)
  (references)
  (foreign_references)
  (extensions)
)