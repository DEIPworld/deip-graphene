#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {


struct join_research_contract_operation : public base_operation
{
    account_name_type member;
    account_name_type research_group;
    percent reward_share;
    optional<flat_set<external_id_type>> researches; // deprecated
    
    extensions_type extensions; // add contract data

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(member);
        a.insert(research_group);
    }
};


}
}

FC_REFLECT(deip::protocol::join_research_contract_operation,
  (member)
  (research_group)
  (reward_share)
  (researches)
  (extensions)
)