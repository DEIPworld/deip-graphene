#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include "deip_object_types.hpp"
#include "witness_objects.hpp"
#include "shared_authority.hpp"

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {

using deip::protocol::authority;

// clang-format off
class account_object : public object<account_object_type, account_object>
{
    account_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    account_object(Constructor&& c, allocator<Allocator> a)
        : json_metadata(a)
    {
        c(*this);
    };

    id_type id;

    account_name_type name;
    public_key_type memo_key;
    fc::shared_string json_metadata;
    account_name_type proxy;

    time_point_sec last_account_update;

    time_point_sec created;
    bool mined = true;
    account_name_type recovery_account;
    time_point_sec last_account_recovery;
    uint32_t lifetime_vote_count = 0;

    bool can_vote = true;

    bool is_research_group;

    //asset balance = asset(0, DEIP_SYMBOL); ///< total liquid shares held by this account
    share_type expertise_tokens_balance = 0; ///< total expertise tokens held by this account
    share_type common_tokens_balance = 0; ///< total common tokens held by this account

    share_type common_tokens_withdraw_rate = 0; ///< at the time this is updated it can be at most common_tokens/104
    time_point_sec next_common_tokens_withdrawal = fc::time_point_sec::maximum(); ///< after every withdrawal this is incremented by 1 week
    share_type withdrawn = 0; /// Track how many shares have been withdrawn
    share_type to_withdraw = 0; /// Might be able to look this up with operation history.
    uint16_t withdraw_routes = 0;

    fc::array<share_type, DEIP_MAX_PROXY_RECURSION_DEPTH> proxied_vsf_votes; // = std::vector<share_type>(
    // DEIP_MAX_PROXY_RECURSION_DEPTH, 0 );
    // ///< the total VFS votes proxied to
    // this account

    uint16_t witnesses_voted_for = 0;
    /// This function should be used only when the account votes for a witness directly
    share_type witness_vote_weight() const
    {
        return std::accumulate(proxied_vsf_votes.begin(), proxied_vsf_votes.end(), expertise_tokens_balance);
    }
    share_type proxied_vsf_votes_total() const
    {
        return std::accumulate(proxied_vsf_votes.begin(), proxied_vsf_votes.end(), share_type());
    }
};
// clang-format on

class account_authority_object : public object<account_authority_object_type, account_authority_object>
{
    account_authority_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    account_authority_object(Constructor&& c, allocator<Allocator> a)
        : owner(a)
        , active(a)
        , posting(a)
        , threshold_overrides(a)
    {
        c(*this);
    }

    id_type id;

    account_name_type account;

    shared_authority owner; ///< used for backup control, can set owner or active
    shared_authority active; ///< used for all monetary operations, can set active or posting
    shared_authority posting; ///< used for voting and posting

    time_point_sec last_owner_update;

    typedef allocator<std::pair<const uint16_t, authority>> op_tag_authority_allocator_type;
    typedef chainbase::bip::map<uint16_t, authority, std::less<uint16_t>, op_tag_authority_allocator_type> op_tag_authority_map;

    op_tag_authority_map threshold_overrides;
};

class owner_authority_history_object
    : public object<owner_authority_history_object_type, owner_authority_history_object>
{
    owner_authority_history_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    owner_authority_history_object(Constructor&& c, allocator<Allocator> a)
        : previous_owner_authority(shared_authority::allocator_type(a.get_segment_manager()))
    {
        c(*this);
    }

    id_type id;

    account_name_type account;
    shared_authority previous_owner_authority;
    time_point_sec last_valid_time;
};

class account_recovery_request_object
    : public object<account_recovery_request_object_type, account_recovery_request_object>
{
    account_recovery_request_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    account_recovery_request_object(Constructor&& c, allocator<Allocator> a)
        : new_owner_authority(shared_authority::allocator_type(a.get_segment_manager()))
    {
        c(*this);
    }

    id_type id;

    account_name_type account_to_recover;
    shared_authority new_owner_authority;
    time_point_sec expires;
};

class change_recovery_account_request_object
    : public object<change_recovery_account_request_object_type, change_recovery_account_request_object>
{
public:
    template <typename Constructor, typename Allocator>
    change_recovery_account_request_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    id_type id;

    account_name_type account_to_recover;
    account_name_type recovery_account;
    time_point_sec effective_on;
};

struct by_name;
struct by_proxy;
struct by_next_common_tokens_withdrawal;
struct by_deip_balance;
struct by_ct_balance;
struct by_vote_count;
struct by_research_group;

/**
 * @ingroup object_index
 */
typedef multi_index_container<account_object,
  indexed_by<
    ordered_unique<
      tag<by_id>,
        member<
          account_object, 
          account_id_type, 
          &account_object::id
        >
    >,
    ordered_unique<
      tag<by_name>,
        member<
          account_object,
          account_name_type,
          &account_object::name
        >
    >,
    ordered_unique<
      tag<by_research_group>,
        member<
          account_object,
          bool,
          &account_object::is_research_group
        >
    >,
    ordered_unique<
      tag<by_proxy>,
        composite_key<account_object,
          member<
            account_object,
            account_name_type,
            &account_object::proxy
          >,
          member<
            account_object,
            account_id_type,
            &account_object::id
          >
        >
    >,
    ordered_unique<
      tag<by_next_common_tokens_withdrawal>,
        composite_key<account_object,
          member<
            account_object,
            time_point_sec,
            &account_object::next_common_tokens_withdrawal
          >,
          member<
            account_object,
            account_id_type,
            &account_object::id
          >
        >
    >,
    ordered_unique<
      tag<by_ct_balance>,
        composite_key<account_object,
          member<
            account_object,
            share_type,
            &account_object::common_tokens_balance
          >,
          member<
            account_object,
            account_id_type,
            &account_object::id
          >
        >,
        composite_key_compare<
          std::greater<asset>,
          std::less<account_id_type>
        >
    >,
    ordered_unique<
      tag<by_vote_count>,
        composite_key<account_object,
          member<
            account_object,
            uint32_t,
            &account_object::lifetime_vote_count
          >,
          member<
            account_object,
            account_id_type,
            &account_object::id
          >
        >,
        composite_key_compare<
          std::greater<uint32_t>,
          std::less<account_id_type>
        >
    >
  >,
  allocator<account_object>>
  account_index;

struct by_account;
struct by_last_valid;

typedef multi_index_container<owner_authority_history_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<owner_authority_history_object,
                                                               owner_authority_history_id_type,
                                                               &owner_authority_history_object::id>>,
                                         ordered_unique<tag<by_account>,
                                                        composite_key<owner_authority_history_object,
                                                                      member<owner_authority_history_object,
                                                                             account_name_type,
                                                                             &owner_authority_history_object::account>,
                                                                      member<owner_authority_history_object,
                                                                             time_point_sec,
                                                                             &owner_authority_history_object::
                                                                                 last_valid_time>,
                                                                      member<owner_authority_history_object,
                                                                             owner_authority_history_id_type,
                                                                             &owner_authority_history_object::id>>,
                                                        composite_key_compare<std::less<account_name_type>,
                                                                              std::less<time_point_sec>,
                                                                              std::
                                                                                  less<owner_authority_history_id_type>>>>,
                              allocator<owner_authority_history_object>>
    owner_authority_history_index;

struct by_last_owner_update;

typedef multi_index_container<account_authority_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<account_authority_object,
                                                               account_authority_id_type,
                                                               &account_authority_object::id>>,
                                         ordered_unique<tag<by_account>,
                                                        composite_key<account_authority_object,
                                                                      member<account_authority_object,
                                                                             account_name_type,
                                                                             &account_authority_object::account>,
                                                                      member<account_authority_object,
                                                                             account_authority_id_type,
                                                                             &account_authority_object::id>>,
                                                        composite_key_compare<std::less<account_name_type>,
                                                                              std::less<account_authority_id_type>>>,
                                         ordered_unique<tag<by_last_owner_update>,
                                                        composite_key<account_authority_object,
                                                                      member<account_authority_object,
                                                                             time_point_sec,
                                                                             &account_authority_object::
                                                                                 last_owner_update>,
                                                                      member<account_authority_object,
                                                                             account_authority_id_type,
                                                                             &account_authority_object::id>>,
                                                        composite_key_compare<std::greater<time_point_sec>,
                                                                              std::less<account_authority_id_type>>>>,
                              allocator<account_authority_object>>
    account_authority_index;

struct by_expiration;

typedef multi_index_container<account_recovery_request_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<account_recovery_request_object,
                                                               account_recovery_request_id_type,
                                                               &account_recovery_request_object::id>>,
                                         ordered_unique<tag<by_account>,
                                                        composite_key<account_recovery_request_object,
                                                                      member<account_recovery_request_object,
                                                                             account_name_type,
                                                                             &account_recovery_request_object::
                                                                                 account_to_recover>,
                                                                      member<account_recovery_request_object,
                                                                             account_recovery_request_id_type,
                                                                             &account_recovery_request_object::id>>,
                                                        composite_key_compare<std::less<account_name_type>,
                                                                              std::
                                                                                  less<account_recovery_request_id_type>>>,
                                         ordered_unique<tag<by_expiration>,
                                                        composite_key<account_recovery_request_object,
                                                                      member<account_recovery_request_object,
                                                                             time_point_sec,
                                                                             &account_recovery_request_object::expires>,
                                                                      member<account_recovery_request_object,
                                                                             account_recovery_request_id_type,
                                                                             &account_recovery_request_object::id>>,
                                                        composite_key_compare<std::less<time_point_sec>,
                                                                              std::
                                                                                  less<account_recovery_request_id_type>>>>,
                              allocator<account_recovery_request_object>>
    account_recovery_request_index;

struct by_effective_date;

typedef multi_index_container<change_recovery_account_request_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<change_recovery_account_request_object,
                                                               change_recovery_account_request_id_type,
                                                               &change_recovery_account_request_object::id>>,
                                         ordered_unique<tag<by_account>,
                                                        composite_key<change_recovery_account_request_object,
                                                                      member<change_recovery_account_request_object,
                                                                             account_name_type,
                                                                             &change_recovery_account_request_object::
                                                                                 account_to_recover>,
                                                                      member<change_recovery_account_request_object,
                                                                             change_recovery_account_request_id_type,
                                                                             &change_recovery_account_request_object::
                                                                                 id>>,
                                                        composite_key_compare<std::less<account_name_type>,
                                                                              std::
                                                                                  less<change_recovery_account_request_id_type>>>,
                                         ordered_unique<tag<by_effective_date>,
                                                        composite_key<change_recovery_account_request_object,
                                                                      member<change_recovery_account_request_object,
                                                                             time_point_sec,
                                                                             &change_recovery_account_request_object::
                                                                                 effective_on>,
                                                                      member<change_recovery_account_request_object,
                                                                             change_recovery_account_request_id_type,
                                                                             &change_recovery_account_request_object::
                                                                                 id>>,
                                                        composite_key_compare<std::less<time_point_sec>,
                                                                              std::
                                                                                  less<change_recovery_account_request_id_type>>>>,
                              allocator<change_recovery_account_request_object>>
    change_recovery_account_request_index;
}
}

// clang-format off

FC_REFLECT( deip::chain::account_object,
             (id)(name)(memo_key)(json_metadata)(proxy)(last_account_update)
             (created)(mined)
             (recovery_account)(last_account_recovery)
             (lifetime_vote_count)(can_vote)(is_research_group)
             (common_tokens_withdraw_rate)(next_common_tokens_withdrawal)
             (withdrawn)(to_withdraw)(withdraw_routes)
             (expertise_tokens_balance)
             (common_tokens_balance)
             (proxied_vsf_votes)(witnesses_voted_for)
          )
CHAINBASE_SET_INDEX_TYPE( deip::chain::account_object, deip::chain::account_index )

FC_REFLECT( deip::chain::account_authority_object,
             (id)(account)(owner)(active)(posting)(last_owner_update)(threshold_overrides)
)
CHAINBASE_SET_INDEX_TYPE( deip::chain::account_authority_object, deip::chain::account_authority_index )


FC_REFLECT( deip::chain::owner_authority_history_object,
             (id)(account)(previous_owner_authority)(last_valid_time)
          )
CHAINBASE_SET_INDEX_TYPE( deip::chain::owner_authority_history_object, deip::chain::owner_authority_history_index )

FC_REFLECT( deip::chain::account_recovery_request_object,
             (id)(account_to_recover)(new_owner_authority)(expires)
          )
CHAINBASE_SET_INDEX_TYPE( deip::chain::account_recovery_request_object, deip::chain::account_recovery_request_index )

FC_REFLECT( deip::chain::change_recovery_account_request_object,
             (id)(account_to_recover)(recovery_account)(effective_on)
          )
CHAINBASE_SET_INDEX_TYPE( deip::chain::change_recovery_account_request_object, deip::chain::change_recovery_account_request_index )

// clang-format on
