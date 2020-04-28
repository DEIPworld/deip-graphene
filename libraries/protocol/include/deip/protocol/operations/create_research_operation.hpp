#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

using deip::protocol::percent;

struct create_research_operation : public base_operation
{
    external_id_type external_id;
    account_name_type research_group;

    string title;
    string abstract;
    string permlink;

    std::set<int64_t> disciplines;
    bool is_private;
    
    flat_set<account_name_type> members;

    percent review_share;
    percent compensation_share;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(research_group);
    }
};


}
}

FC_REFLECT(deip::protocol::create_research_operation,
  (external_id)
  (research_group)
  (title)
  (abstract)
  (permlink)
  (disciplines)
  (is_private)
  (members)
  (review_share)
  (compensation_share)
  (extensions)
)