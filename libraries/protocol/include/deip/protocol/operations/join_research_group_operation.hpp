#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

struct join_research_group_operation : public base_operation
{
    account_name_type member;
    account_name_type research_group;
    bool is_invitation;
    percent weight;
    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(member);
        a.insert(research_group);
    }
};


}
}

FC_REFLECT(deip::protocol::join_research_group_operation,
  (member)
  (research_group)
  (is_invitation)
  (weight)
  (extensions)
)