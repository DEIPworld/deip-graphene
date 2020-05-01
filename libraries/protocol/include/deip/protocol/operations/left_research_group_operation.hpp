#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

struct left_research_group_operation : public base_operation
{
    account_name_type member;
    account_name_type research_group;
    bool is_exclusion;
    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        if (is_exclusion)
        {
            a.insert(research_group);
        }
        else 
        {
            a.insert(member);
        }
    }
};


}
}

FC_REFLECT(deip::protocol::left_research_group_operation,
  (member)
  (research_group)
  (is_exclusion)
  (extensions)
)