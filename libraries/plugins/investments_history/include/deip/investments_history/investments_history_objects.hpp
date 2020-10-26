#pragma once

#ifndef INVESTMENTS_HISTORY_SPACE_ID
#define INVESTMENTS_HISTORY_SPACE_ID 17
#endif

using namespace deip::chain;
using namespace std;

namespace deip {
namespace investments_history {

enum investments_history_plugin_object_type
{
    account_revenue_income_history_object_type = (INVESTMENTS_HISTORY_SPACE_ID << 8)
};

class account_revenue_income_history_object;

typedef oid<account_revenue_income_history_object> account_revenue_income_history_id_type;


}
}

FC_REFLECT_ENUM(deip::investments_history::investments_history_plugin_object_type,
  (account_revenue_income_history_object_type)
)