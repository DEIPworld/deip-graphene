#pragma once

#include <fc/api.hpp>
#include <deip/chain/schema/deip_object_types.hpp>
#include <deip/investments_history/investments_history_api_objects.hpp>

namespace deip {
namespace app {
struct api_context;
}
} // namespace deip

namespace deip {
namespace investments_history {

using namespace std;
using namespace deip::chain;

namespace detail {
class investments_history_api_impl;
}

class investments_history_api
{
public:
    investments_history_api(const deip::app::api_context& ctx);
    ~investments_history_api();

    void on_api_startup();

    std::vector<account_revenue_income_history_api_obj>
    get_account_revenue_history_by_security_token(const account_name_type& account,
                                                  const external_id_type& security_token_external_id,
                                                  const account_revenue_income_history_id_type& cursor) const;

    std::vector<account_revenue_income_history_api_obj>
    get_account_revenue_history(const account_name_type& account,
                                const account_revenue_income_history_id_type& cursor) const;

    std::vector<account_revenue_income_history_api_obj>
    get_security_token_revenue_history(const external_id_type& security_token_external_id,
                                       const account_revenue_income_history_id_type& cursor) const;

private : std::unique_ptr<detail::investments_history_api_impl> _impl;

};
} // namespace investments_history
} // namespace deip



FC_API(deip::investments_history::investments_history_api,
  (get_account_revenue_history_by_security_token)
  (get_account_revenue_history)
  (get_security_token_revenue_history)
)