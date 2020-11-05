#pragma once
#include <deip/app/deip_api_objects.hpp>
#include <deip/investments_history/investments_history_objects.hpp>
#include <deip/investments_history/account_revenue_income_history_object.hpp>

namespace deip {
namespace investments_history {

struct account_revenue_income_history_api_obj
{
    account_revenue_income_history_api_obj(){};
    account_revenue_income_history_api_obj(const account_name_type& account,
                                           const app::asset_api_obj& security_token,
                                           const asset& revenue,
                                           const fc::time_point_sec& timestamp)
        : account(account)
        , security_token(security_token)
        , revenue(revenue)
        , timestamp(timestamp)
    {}

    account_name_type account;
    app::asset_api_obj security_token;
    asset revenue;
    fc::time_point_sec timestamp;  
};

} // namespace investments_history
}


FC_REFLECT(deip::investments_history::account_revenue_income_history_api_obj, 
  (account)
  (security_token)
  (revenue)
  (timestamp)
)
