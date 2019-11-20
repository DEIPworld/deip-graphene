#pragma once
#include <fc/api.hpp>
#include <deip/app/deip_api_objects.hpp>
#include <deip/blockchain_history/applied_operation.hpp>
#include <deip/protocol/transaction.hpp>
#include <map>


namespace deip {
namespace app {

struct api_context;
class application;

} // namespace app
} // namespace deip

namespace deip {
namespace blockchain_history {

using deip::protocol::annotated_signed_transaction;
using deip::protocol::transaction_id_type;

namespace detail {
class blockchain_history_api_impl;
}

class blockchain_history_api
{
public:
    blockchain_history_api(const deip::app::api_context& ctx);

    ~blockchain_history_api();

    void on_api_startup();

    std::map<uint32_t, applied_operation>
    get_ops_history(uint32_t from_op, uint32_t limit, const applied_operation_type& opt) const;

    /**
     *  @brief Get sequence of operations included/generated within a particular block
     *  @param block_num Height of the block whose generated virtual operations should be returned
     *  @param only_virtual Whether to only include virtual operations in returned results (default: true)
     *  @return sequence of operations included/generated within the block
     */
    std::map<uint32_t, applied_operation> get_ops_in_block(uint32_t block_num, applied_operation_type opt) const;

    annotated_signed_transaction get_transaction(transaction_id_type trx_id) const;

    optional<protocol::block_header> get_block_header(uint32_t block_num) const;

    std::map<uint32_t, protocol::block_header> get_block_headers_history(uint32_t block_num, uint32_t limit) const;

    std::map<uint32_t, app::signed_block_api_obj> get_blocks_history(uint32_t block_num, uint32_t limit) const;

private:
    std::unique_ptr<detail::blockchain_history_api_impl> _impl;
};

} // namespace blockchain_history
} // namespace deip

FC_API(deip::blockchain_history::blockchain_history_api,
        (get_ops_history)
        (get_ops_in_block)
        (get_transaction)
        (get_block_header)
        (get_block_headers_history)
        (get_blocks_history)
)