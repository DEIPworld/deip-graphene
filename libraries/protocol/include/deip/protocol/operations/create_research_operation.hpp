#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct create_research_operation : public base_operation
{
    account_name_type creator;
    external_id_type research_group_external_id;

    string title;
    string abstract;
    string permlink;
    uint16_t review_share_in_percent;
    uint16_t dropout_compensation_in_percent;
    std::set<int64_t> disciplines;
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

FC_REFLECT(deip::protocol::create_research_operation,
          (creator)
          (research_group_external_id)
          (title)
          (abstract)
          (permlink)
          (review_share_in_percent)
          (dropout_compensation_in_percent)
          (disciplines)
          (is_private)
          (expiration_time)
          (extensions)
)