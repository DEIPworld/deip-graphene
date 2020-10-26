#pragma once

#include <boost/multi_index/composite_key.hpp>

using namespace deip::chain;
using namespace std;

namespace deip {
namespace investments_history {

using chainbase::allocator;

using deip::protocol::asset;
using deip::protocol::external_id_type;

class account_revenue_income_history_object
    : public object<account_revenue_income_history_object_type, account_revenue_income_history_object>
{
public:
    template <typename Constructor, typename Allocator>
    account_revenue_income_history_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    account_revenue_income_history_object()
    {
    }

    account_revenue_income_history_id_type id;

    account_name_type account;
    external_id_type security_token;
    uint32_t security_token_units;

    asset balance;
    asset revenue;

    fc::time_point_sec timestamp;
};

struct by_account_and_cursor;
struct by_security_token_and_cursor;
struct by_account_and_security_token_and_cursor;

typedef chainbase::shared_multi_index_container<
    account_revenue_income_history_object,
    indexed_by<ordered_unique<tag<by_id>,
                              member<account_revenue_income_history_object,
                                     account_revenue_income_history_id_type,
                                     &account_revenue_income_history_object::id>>,

               ordered_non_unique<tag<by_account>,
                                  member<account_revenue_income_history_object,
                                         account_name_type,
                                         &account_revenue_income_history_object::account>>,

               ordered_non_unique<tag<by_account_and_cursor>,
                                  composite_key<account_revenue_income_history_object,
                                                member<account_revenue_income_history_object,
                                                       account_name_type,
                                                       &account_revenue_income_history_object::account>,
                                                member<account_revenue_income_history_object,
                                                       account_revenue_income_history_id_type,
                                                       &account_revenue_income_history_object::id>>>,

               ordered_non_unique<tag<by_security_token_and_cursor>,
                                  composite_key<account_revenue_income_history_object,
                                                member<account_revenue_income_history_object,
                                                       external_id_type,
                                                       &account_revenue_income_history_object::security_token>,
                                                member<account_revenue_income_history_object,
                                                       account_revenue_income_history_id_type,
                                                       &account_revenue_income_history_object::id>>>,

               ordered_non_unique<tag<by_account_and_security_token_and_cursor>,
                                  composite_key<account_revenue_income_history_object,
                                                member<account_revenue_income_history_object,
                                                       account_name_type,
                                                       &account_revenue_income_history_object::account>,
                                                member<account_revenue_income_history_object,
                                                       external_id_type,
                                                       &account_revenue_income_history_object::security_token>,
                                                member<account_revenue_income_history_object,
                                                       account_revenue_income_history_id_type,
                                                       &account_revenue_income_history_object::id>>>

               >
    // , allocator<account_revenue_income_history_object>
    >
    account_revenue_income_history_index;

} // namespace investments_history
} // namespace deip

FC_REFLECT(deip::investments_history::account_revenue_income_history_object,
          (id)
          (account)
          (security_token)
          (security_token_units)
          (balance)
          (revenue)
          (timestamp)
)

CHAINBASE_SET_INDEX_TYPE(deip::investments_history::account_revenue_income_history_object, deip::investments_history::account_revenue_income_history_index)
