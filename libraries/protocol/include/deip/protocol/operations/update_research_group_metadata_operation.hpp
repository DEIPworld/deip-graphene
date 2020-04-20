#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct update_research_group_metadata_operation : public base_operation
{
    account_name_type creator;
    external_id_type research_group_external_id;

    string research_group_name;
    string research_group_description;

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

FC_REFLECT(deip::protocol::update_research_group_metadata_operation,
          (creator)
          (research_group_external_id)
          (research_group_name)
          (research_group_description)
          (expiration_time)
          (extensions)
)