#pragma once

#include "deip_object_types.hpp"
#include <fc/shared_string.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <deip/protocol/transaction.hpp>

using namespace deip::protocol;

namespace deip {
namespace chain {

using fc::shared_string;
using fc::time_point_sec;

enum class proposal_status : uint8_t
{
    pending = 1,
    approved = 2,
    rejected = 3,
    failed = 4,
    expired = 5
};

class proposal_object : public object<proposal_object_type, proposal_object>
{

public:
    template <typename Constructor, typename Allocator> proposal_object(Constructor&& c, allocator<Allocator> a) 
      : fail_reason(a)
    {
        c(*this);
    }

public:

      proposal_id_type                id;

      external_id_type                external_id; 
      time_point_sec                  expiration_time;
      optional<time_point_sec>        review_period_time;
      transaction                     proposed_transaction;
      flat_set<account_name_type>     required_active_approvals;
      flat_set<account_name_type>     available_active_approvals;
      flat_set<account_name_type>     required_owner_approvals;
      flat_set<account_name_type>     available_owner_approvals;
      flat_set<public_key_type>       available_key_approvals;
      account_name_type               proposer;
      shared_string                   fail_reason;

      time_point_sec                  created_at;
      
      bool is_authorized_to_execute(chainbase::database& db) const;

      bool is_authorized_to_execute(chainbase::database& db,
                                    const flat_set<account_name_type>& active_approvals_checklist,
                                    const flat_set<account_name_type>& owner_approvals_checklist) const;
};


struct by_external_id;
struct by_proposer;
struct by_expiration;


typedef multi_index_container<proposal_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
        member<
          proposal_object,
          proposal_id_type,
          &proposal_object::id
        >
    >,
    ordered_unique<
      tag<by_external_id>,
        member<
          proposal_object,
          external_id_type,
          &proposal_object::external_id
        >
    >,
    
    ordered_unique<
      tag<by_expiration>,
        composite_key<proposal_object,
          member<
            proposal_object, 
            fc::time_point_sec, 
            &proposal_object::expiration_time
          >,
          member<
            proposal_object, 
            proposal_id_type, 
            &proposal_object::id
          >
        >
    >,

    ordered_non_unique<
      tag<by_proposer>,
        member<
          proposal_object,
          account_name_type,
          &proposal_object::proposer
        >
    >
  >,
  allocator<proposal_object>>

  proposal_index;


} // namespace chain
} // namespace deip


FC_REFLECT(deip::chain::proposal_object, 
  (id)
  (external_id)
  (expiration_time)
  (review_period_time)
  (proposed_transaction)
  (required_active_approvals)
  (available_active_approvals)
  (required_owner_approvals)
  (available_owner_approvals)
  (available_key_approvals)
  (proposer)
  (fail_reason)
  (created_at)
)

FC_REFLECT_ENUM(deip::chain::proposal_status, (pending)(approved)(rejected)(failed)(expired))

CHAINBASE_SET_INDEX_TYPE(deip::chain::proposal_object, deip::chain::proposal_index)