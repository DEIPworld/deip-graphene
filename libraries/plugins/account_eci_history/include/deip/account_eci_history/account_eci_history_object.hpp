#pragma once

#include <deip/account_eci_history/account_eci_operation_object.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace account_eci_history {

template <uint16_t AccountECIHistoryType> struct account_eci_history_object : public object<AccountECIHistoryType, account_eci_history_object<AccountECIHistoryType>>
{
    template <typename Constructor, typename Allocator>
    account_eci_history_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    typedef typename object<AccountECIHistoryType, account_eci_history_object<AccountECIHistoryType>>::id_type id_type;

    id_type id;

    account_name_type account_name;
    discipline_id_type discipline_id;

    share_type new_eci_amount;
    share_type delta;

    uint16_t action;
    int64_t action_object_id;

    uint32_t timestamp;

    account_eci_operation_object::id_type op;
};

struct by_account_name_and_discipline;
struct by_account_name;

template <typename account_eci_history_object_t>
using account_eci_history_index
    = chainbase::shared_multi_index_container<account_eci_history_object_t,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<account_eci_history_object_t,
                                                                    typename account_eci_history_object_t::id_type,
                                                                    &account_eci_history_object_t::id>>,
                                              ordered_non_unique<tag<by_account_name_and_discipline>,
                                              composite_key<account_eci_history_object_t,
                                                             member<account_eci_history_object_t,
                                                                    account_name_type,
                                                                    &account_eci_history_object_t::account_name>,
                                                             member<account_eci_history_object_t,
                                                                     discipline_id_type,
                                                                    &account_eci_history_object_t::discipline_id>>>,
                                              ordered_non_unique<tag<by_account_name>,
                                                             member<account_eci_history_object_t,
                                                                     account_name_type,
                                                                    &account_eci_history_object_t::account_name>>>>;

using account_eci_operations_history_object = account_eci_history_object<all_account_eci_operations_history>;
using account_eci_operations_history_index = account_eci_history_index<account_eci_operations_history_object>;

//
} // namespace account_eci_history
} // namespace deip

FC_REFLECT(deip::account_eci_history::account_eci_operations_history_object, (id)(account_name)(discipline_id)(new_eci_amount)(delta)(action)(action_object_id)(timestamp)(op))

CHAINBASE_SET_INDEX_TYPE(deip::account_eci_history::account_eci_operations_history_object,
                         deip::account_eci_history::account_eci_operations_history_index)
