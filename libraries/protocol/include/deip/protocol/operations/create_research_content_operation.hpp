#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct create_research_content_operation : public base_operation
{
    account_name_type creator;
    external_id_type external_id;
    external_id_type research_external_id;
    external_id_type research_group_external_id;

    uint16_t type;
    string title;
    string content;
    string permlink;
    std::set<account_name_type> authors;
    std::set<external_id_type> references;
    std::set<string> foreign_references;

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

FC_REFLECT(deip::protocol::create_research_content_operation,
          (creator)
          (external_id)
          (research_external_id)
          (research_group_external_id)
          (type)
          (title)
          (content)
          (permlink)
          (authors)
          (references)
          (foreign_references)
          (expiration_time)
          (extensions)
)