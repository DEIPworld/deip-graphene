#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/asset.hpp>

#include <fc/utf8.hpp>

namespace deip {
namespace protocol {

struct fill_common_tokens_withdraw_operation : public virtual_operation
{
    fill_common_tokens_withdraw_operation() {}
    fill_common_tokens_withdraw_operation(const string& f, const string& t, const share_type& w, const share_type& d, const bool tr)
        : from_account(f)
        , to_account(t)
        , withdrawn(w)
        , deposited(d)
        , transfer(tr)
    {
    }

    account_name_type from_account;
    account_name_type to_account;
    share_type withdrawn;
    share_type deposited;
    bool transfer;
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

struct producer_reward_operation : public virtual_operation
{
    producer_reward_operation() {}
    producer_reward_operation(const string& p, const share_type c)
        : producer(p)
        , common_tokens_amount(c)
    {
    }

    account_name_type producer;
    share_type common_tokens_amount;
};
}
} // deip::protocol

FC_REFLECT(deip::protocol::fill_common_tokens_withdraw_operation, (from_account)(to_account)(withdrawn)(deposited)(transfer))
FC_REFLECT(deip::protocol::shutdown_witness_operation, (owner))
FC_REFLECT(deip::protocol::hardfork_operation, (hardfork_id))
FC_REFLECT(deip::protocol::producer_reward_operation, (producer)(common_tokens_amount))
