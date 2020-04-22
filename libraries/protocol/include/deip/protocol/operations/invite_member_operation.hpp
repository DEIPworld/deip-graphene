#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

struct invite_member_operation : public base_operation
{
    account_name_type creator;
    external_id_type research_group_external_id;

    account_name_type invitee;
    share_type research_group_token_amount_in_percent;
    string cover_letter;

    bool is_head;

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

FC_REFLECT(deip::protocol::invite_member_operation,
          (creator)
          (research_group_external_id)
          (invitee)
          (research_group_token_amount_in_percent)
          (cover_letter)
          (is_head)
          (expiration_time)
          (extensions)
)