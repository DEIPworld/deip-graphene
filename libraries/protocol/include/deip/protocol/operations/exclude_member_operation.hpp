#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

struct exclude_member_operation : public base_operation
{
    account_name_type creator;
    external_id_type research_group_external_id;

    account_name_type account_to_exclude;

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

FC_REFLECT(deip::protocol::exclude_member_operation,
          (creator)
          (research_group_external_id)
          (account_to_exclude)
          (expiration_time)
          (extensions)
)