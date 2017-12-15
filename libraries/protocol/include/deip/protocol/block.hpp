#pragma once
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/transaction.hpp>

namespace deip {
namespace protocol {

struct signed_block : public signed_block_header
{
    checksum_type calculate_merkle_root() const;
    vector<signed_transaction> transactions;
};
}
} // deip::protocol

FC_REFLECT_DERIVED(deip::protocol::signed_block, (deip::protocol::signed_block_header), (transactions))
