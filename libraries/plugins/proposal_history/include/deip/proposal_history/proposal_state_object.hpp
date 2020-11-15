#pragma once

#include <boost/multi_index/composite_key.hpp>

using namespace deip::chain;
using namespace std;

namespace deip {
namespace proposal_history {

using chainbase::allocator;

using deip::protocol::asset;
using deip::protocol::external_id_type;
using deip::protocol::asset_symbol_type;
using fc::shared_string;

enum class proposal_status : uint8_t
{
    pending = 1,
    approved = 2,
    rejected = 3,
    failed = 4,
    expired = 5
};

class proposal_state_object : public object<proposal_state_object_type, proposal_state_object>
{
public:
    template <typename Constructor, typename Allocator>
    proposal_state_object(Constructor&& c, allocator<Allocator> a) : fail_reason(a)

    {
        c(*this);
    }

    proposal_state_id_type id;

    external_id_type external_id;
    account_name_type proposer;

    uint8_t status;

    flat_set<account_name_type> required_approvals;
    flat_set<account_name_type> approvals;
    account_name_type rejector;

    flat_set<public_key_type> signers;

    transaction proposed_transaction;
    shared_string fail_reason;
    time_point_sec expiration_time;
    optional<time_point_sec> review_period_time;
    time_point_sec created_at;
};

struct by_external_id;
struct by_proposer;
struct by_status;


typedef chainbase::shared_multi_index_container<
    proposal_state_object,
          indexed_by<ordered_unique<tag<by_id>,
                                  member<proposal_state_object,
                                        proposal_state_id_type,
                                        &proposal_state_object::id>>,

               ordered_non_unique<tag<by_external_id>,
                                  member<proposal_state_object,
                                        external_id_type,
                                        &proposal_state_object::external_id>>,
                
               ordered_non_unique<tag<by_proposer>,
                                  member<proposal_state_object,
                                        account_name_type,
                                        &proposal_state_object::proposer>>,

               ordered_non_unique<tag<by_status>,
                                  member<proposal_state_object,
                                         uint8_t,
                                         &proposal_state_object::status>>

               >
    // , allocator<account_revenue_income_history_object>
    >
    proposal_history_index;

} // namespace proposal_history
} // namespace deip

FC_REFLECT(deip::proposal_history::proposal_state_object,
          (id)
          (external_id)
          (proposer)
          (status)
          (required_approvals)
          (approvals)
          (rejector)
          (signers)
          (proposed_transaction)
          (fail_reason)
          (expiration_time)
          (review_period_time)
          (created_at)
)

FC_REFLECT_ENUM(deip::proposal_history::proposal_status, (pending)(approved)(rejected)(failed)(expired))

CHAINBASE_SET_INDEX_TYPE(deip::proposal_history::proposal_state_object, deip::proposal_history::proposal_history_index)
