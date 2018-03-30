#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/asset.hpp>

#include <fc/utf8.hpp>

namespace deip {
namespace protocol {

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

FC_REFLECT(deip::protocol::shutdown_witness_operation, (owner))
FC_REFLECT(deip::protocol::hardfork_operation, (hardfork_id))
FC_REFLECT(deip::protocol::producer_reward_operation, (producer)(vesting_shares))
