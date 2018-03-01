#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/asset.hpp>

#include <fc/utf8.hpp>

namespace deip {
namespace protocol {

struct author_reward_operation : public virtual_operation
{
    author_reward_operation() {}
    author_reward_operation(const account_name_type& a, const string& p, const asset& scr, const asset& v)
        : author(a)
        , permlink(p)
        , deip_payout(scr)
        , vesting_payout(v)
    {
    }

    account_name_type author;
    string permlink;
    asset deip_payout;
    asset vesting_payout;
};

struct curation_reward_operation : public virtual_operation
{
    curation_reward_operation() {}
    curation_reward_operation(const string& c, const asset& r, const string& a, const string& p)
        : curator(c)
        , reward(r)
    {
    }

    account_name_type curator;
    asset reward;
};

struct fill_vesting_withdraw_operation : public virtual_operation
{
    fill_vesting_withdraw_operation() {}
    fill_vesting_withdraw_operation(const string& f, const string& t, const asset& w, const asset& d)
        : from_account(f)
        , to_account(t)
        , withdrawn(w)
        , deposited(d)
    {
    }

    account_name_type from_account;
    account_name_type to_account;
    asset withdrawn;
    asset deposited;
};

struct shutdown_witness_operation : public virtual_operation
{
    shutdown_witness_operation() {}
    shutdown_witness_operation(const string& o)
        : owner(o)
    {
    }

    account_name_type owner;
};

struct hardfork_operation : public virtual_operation
{
    hardfork_operation() {}
    hardfork_operation(uint32_t hf_id)
        : hardfork_id(hf_id)
    {
    }

    uint32_t hardfork_id = 0;
};

struct return_vesting_delegation_operation : public virtual_operation
{
    return_vesting_delegation_operation() {}
    return_vesting_delegation_operation(const account_name_type& a, const asset& v)
        : account(a)
        , vesting_shares(v)
    {
    }

    account_name_type account;
    asset vesting_shares;
};

struct producer_reward_operation : public virtual_operation
{
    producer_reward_operation() {}
    producer_reward_operation(const string& p, const asset& v)
        : producer(p)
        , vesting_shares(v)
    {
    }

    account_name_type producer;
    asset vesting_shares;
};
}
} // deip::protocol

FC_REFLECT(deip::protocol::author_reward_operation, (author)(permlink)(deip_payout)(vesting_payout))
FC_REFLECT(deip::protocol::curation_reward_operation, (curator)(reward))
FC_REFLECT(deip::protocol::fill_vesting_withdraw_operation, (from_account)(to_account)(withdrawn)(deposited))
FC_REFLECT(deip::protocol::shutdown_witness_operation, (owner))
FC_REFLECT(deip::protocol::hardfork_operation, (hardfork_id))
FC_REFLECT(deip::protocol::return_vesting_delegation_operation, (account)(vesting_shares))
FC_REFLECT(deip::protocol::producer_reward_operation, (producer)(vesting_shares))
