#pragma once

#include <deip/blockchain_history/operation_objects.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace blockchain_history {

template <uint16_t HistoryType> struct history_object : public object<HistoryType, history_object<HistoryType>>
{
    CHAINBASE_DEFAULT_CONSTRUCTOR(history_object)

    typedef typename object<HistoryType, history_object<HistoryType>>::id_type id_type;

    id_type id;

    account_name_type account;
    uint32_t sequence = 0;
    operation_object::id_type op;
};

struct by_account;

template <typename history_object_t>
using history_index
    = chainbase::shared_multi_index_container<history_object_t,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<history_object_t,
                                                                    typename history_object_t::id_type,
                                                                    &history_object_t::id>>,
                                              ordered_unique<tag<by_account>,
                                                             composite_key<history_object_t,
                                                                           member<history_object_t,
                                                                                  account_name_type,
                                                                                  &history_object_t::account>,
                                                                           member<history_object_t,
                                                                                  uint32_t,
                                                                                  &history_object_t::sequence>>,
                                                             composite_key_compare<std::less<account_name_type>,
                                                                                    // for derect oder iteration from
                                                                                   // grater value to less
                                                                                   std::greater<uint32_t>>>>>;

using account_history_object = history_object<all_account_operations_history>;
using transfers_to_deip_history_object = history_object<account_deip_to_deip_transfers_history>;
using transfers_to_common_tokens_history_object = history_object<account_deip_to_common_tokens_transfers_history>;

using account_operations_full_history_index = history_index<account_history_object>;
using transfers_to_deip_history_index = history_index<transfers_to_deip_history_object>;
using transfers_to_common_tokens_history_index = history_index<transfers_to_common_tokens_history_object>;
//
} // namespace blockchain_history
} // namespace deip

FC_REFLECT(deip::blockchain_history::account_history_object, (id)(account)(sequence)(op))
FC_REFLECT(deip::blockchain_history::transfers_to_deip_history_object, (id)(account)(sequence)(op))
FC_REFLECT(deip::blockchain_history::transfers_to_common_tokens_history_object, (id)(account)(sequence)(op))

CHAINBASE_SET_INDEX_TYPE(deip::blockchain_history::account_history_object,
                         deip::blockchain_history::account_operations_full_history_index)

CHAINBASE_SET_INDEX_TYPE(deip::blockchain_history::transfers_to_deip_history_object,
                         deip::blockchain_history::transfers_to_deip_history_index)

CHAINBASE_SET_INDEX_TYPE(deip::blockchain_history::transfers_to_common_tokens_history_object,
                         deip::blockchain_history::transfers_to_common_tokens_history_index)