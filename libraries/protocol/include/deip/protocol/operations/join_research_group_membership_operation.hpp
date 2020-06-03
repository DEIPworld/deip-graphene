#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

struct join_research_group_membership_operation : public base_operation
{
    account_name_type member;
    account_name_type research_group;
    percent reward_share;
    /*
      Researches to include the member as a researcher.
      If the list is not provided, the member will be included to all researches as a researcher
      If the list is empty, the member will not be included to group researches as a researcher. He can be
      added later using [update_research_operation]. 
      If the list has entries, they will be validated for research group ownership.
    */
    optional<flat_set<external_id_type>> researches;
    
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

FC_REFLECT(deip::protocol::join_research_group_membership_operation,
  (member)
  (research_group)
  (reward_share)
  (researches)
  (extensions)
)