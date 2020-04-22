
#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

// struct operation;
struct op_wrapper;

// void operation_validate(const deip::protocol::operation& op);

struct create_proposal_operation : public base_operation
{
    account_name_type creator;
    vector<op_wrapper> proposed_ops;
    time_point_sec expiration_time;
    optional<uint32_t> review_period_seconds;
    extensions_type extensions;

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(creator);
    }
};

}
}


FC_REFLECT(deip::protocol::create_proposal_operation,
  (creator)
  (proposed_ops)
  (expiration_time)
  (review_period_seconds)
  (extensions)
)
