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

struct tx_info
{
    transaction_id_type trx_id;
    uint32_t block_num;
    time_point_sec timestamp;
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
    
    flat_map<account_name_type, tx_info> approvals;
    flat_map<account_name_type, tx_info> rejectors;

    flat_set<account_name_type> owner_approvals;
    flat_set<account_name_type> active_approvals;
    flat_set<public_key_type> key_approvals;

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

               ordered_unique<tag<by_external_id>,
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
    >
    proposal_history_index;


class proposal_lookup_object : public object<proposal_lookup_object_type, proposal_lookup_object>
{
public:
    template <typename Constructor, typename Allocator> proposal_lookup_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    proposal_lookup_object_id_type id;
    external_id_type proposal;
    account_name_type account;
};

struct lookup_by_proposal;
struct lookup_by_account;
struct lookup_by_account_and_proposal;

typedef chainbase::shared_multi_index_container<
    proposal_lookup_object,
          indexed_by<ordered_unique<tag<by_id>,
                                  member<proposal_lookup_object,
                                        proposal_lookup_object_id_type,
                                        &proposal_lookup_object::id>>,

               ordered_non_unique<tag<lookup_by_proposal>,
                                  member<proposal_lookup_object,
                                        external_id_type,
                                        &proposal_lookup_object::proposal>>,
                
               ordered_non_unique<tag<lookup_by_account>,
                                  member<proposal_lookup_object,
                                        account_name_type,
                                        &proposal_lookup_object::account>>,

               ordered_unique<tag<lookup_by_account_and_proposal>,
                                  composite_key<proposal_lookup_object,
                                                member<proposal_lookup_object,
                                                       external_id_type,
                                                       &proposal_lookup_object::proposal>,
                                                member<proposal_lookup_object,
                                                       account_name_type,
                                                       &proposal_lookup_object::account>>>
               >
    >
    proposal_lookup_index;


} // namespace proposal_history
} // namespace deip

FC_REFLECT(deip::proposal_history::proposal_state_object,
          (id)
          (external_id)
          (proposer)
          (status)
          (required_approvals)
          (approvals)
          (rejectors)
          (owner_approvals)
          (active_approvals)
          (key_approvals)
          (proposed_transaction)
          (fail_reason)
          (expiration_time)
          (review_period_time)
          (created_at)
)

FC_REFLECT(deip::proposal_history::tx_info,
          (trx_id)
          (block_num)
          (timestamp)
)

FC_REFLECT(deip::proposal_history::proposal_lookup_object,
          (id)
          (proposal)
          (account)
)

CHAINBASE_SET_INDEX_TYPE(deip::proposal_history::proposal_state_object, deip::proposal_history::proposal_history_index)
CHAINBASE_SET_INDEX_TYPE(deip::proposal_history::proposal_lookup_object, deip::proposal_history::proposal_lookup_index)
