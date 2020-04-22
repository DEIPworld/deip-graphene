#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

struct update_research_metadata_operation : public base_operation
{
    account_name_type creator;
    external_id_type research_group_external_id;

    external_id_type research_external_id;
    string research_title;
    string research_abstract;

    bool is_private;

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

FC_REFLECT(deip::protocol::update_research_metadata_operation,
          (creator)
          (research_group_external_id)
          (research_external_id)
          (research_title)
          (research_abstract)
          (expiration_time)
          (extensions)
)