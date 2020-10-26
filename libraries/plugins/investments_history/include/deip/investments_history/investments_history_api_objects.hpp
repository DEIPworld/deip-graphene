#pragma once
#include <deip/app/deip_api_objects.hpp>
#include <deip/investments_history/investments_history_objects.hpp>
#include <deip/investments_history/account_revenue_income_history_object.hpp>

namespace deip {
namespace investments_history {

struct account_revenue_income_history_api_obj
{
    account_revenue_income_history_api_obj(){};
    account_revenue_income_history_api_obj(const account_revenue_income_history_object& hist,
                                           const app::security_token_api_obj& security_token)
        : id(hist.id._id)
        , account(hist.account)
        , security_token(security_token)
        , security_token_units(hist.security_token_units)
        , balance(hist.balance)
        , revenue(hist.revenue)
        , timestamp(hist.timestamp)
    {}

    int64_t id;

    account_name_type account;
    app::security_token_api_obj security_token;
    uint32_t security_token_units;

    asset balance;
    asset revenue;

    fc::time_point_sec timestamp;  
};

} // namespace investments_history
}


FC_REFLECT(deip::investments_history::account_revenue_income_history_api_obj, 
  (id)
  (account)
  (security_token)
  (security_token_units)
  (balance)
  (revenue)
  (timestamp)
)
