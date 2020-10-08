#pragma once

#include <deip/token_sale_contribution_history/tsc_operation_object.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace deip {
namespace tsc_history {

template <uint16_t TSCHistoryType> struct tsc_history_object : public object<TSCHistoryType, tsc_history_object<TSCHistoryType>>
{
    CHAINBASE_DEFAULT_CONSTRUCTOR(tsc_history_object)

    typedef typename object<TSCHistoryType, tsc_history_object<TSCHistoryType>>::id_type id_type;

    id_type id;

    account_name_type contributor;
    research_id_type research_id;
    research_token_sale_id_type research_token_sale_id;
    protocol::asset amount;
    tsc_operation_object::id_type op;
};

struct by_account_and_research;
struct by_account;
struct by_research;
struct by_token_sale;

template <typename tsc_history_object_t>
using tsc_history_index
    = chainbase::shared_multi_index_container<tsc_history_object_t,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                              member<tsc_history_object_t,
                                                                    typename tsc_history_object_t::id_type,
                                                                    &tsc_history_object_t::id>>,
                                              ordered_non_unique<tag<by_research>,
                                                              member<tsc_history_object_t,
                                                                    research_id_type,
                                                                    &tsc_history_object_t::research_id>>,
                                              ordered_non_unique<tag<by_token_sale>,
                                                              member<tsc_history_object_t,
                                                                    research_token_sale_id_type,
                                                                    &tsc_history_object_t::research_token_sale_id>>,
                                              ordered_non_unique<tag<by_account_and_research>,
                                              composite_key<tsc_history_object_t,
                                                              member<tsc_history_object_t,
                                                                    account_name_type,
                                                                    &tsc_history_object_t::contributor>,
                                                              member<tsc_history_object_t,
                                                                     research_id_type,
                                                                    &tsc_history_object_t::research_id>>>,
                                              ordered_non_unique<tag<by_account>,
                                                              member<tsc_history_object_t,
                                                                     account_name_type,
                                                                    &tsc_history_object_t::contributor>>>>;

using all_tsc_operations_history_object = tsc_history_object<all_tsc_operations_history>;
using contribute_to_token_sale_history_object = tsc_history_object<contribute_to_token_sale_history>;

using tsc_operations_full_history_index = tsc_history_index<all_tsc_operations_history_object>;
using contribute_to_token_sale_history_index = tsc_history_index<contribute_to_token_sale_history_object>;

//
} // namespace tsc_history
} // namespace deip

FC_REFLECT(deip::tsc_history::all_tsc_operations_history_object, (id)(contributor)(research_id)(research_token_sale_id)(amount)(op))
FC_REFLECT(deip::tsc_history::contribute_to_token_sale_history_object, (id)(contributor)(research_id)(research_token_sale_id)(amount)(op))


CHAINBASE_SET_INDEX_TYPE(deip::tsc_history::all_tsc_operations_history_object,
                         deip::tsc_history::tsc_operations_full_history_index)

CHAINBASE_SET_INDEX_TYPE(deip::tsc_history::contribute_to_token_sale_history_object,
                         deip::tsc_history::contribute_to_token_sale_history_index)