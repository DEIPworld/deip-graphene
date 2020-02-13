#pragma once

#include <fc/api.hpp>
#include <deip/account_eci_history/applied_account_eci_operation.hpp>

namespace deip {
namespace app {
struct api_context;
}
} // namespace deip

namespace deip {
namespace account_eci_history {

namespace detail {
class account_eci_history_api_impl;
}

class account_eci_history_api
{
public:
    account_eci_history_api(const deip::app::api_context& ctx);
    ~account_eci_history_api();

    void on_api_startup();

    std::vector<applied_account_eci_operation>
    get_eci_history_by_account_and_discipline(const account_name_type& account_name, const discipline_id_type& discipline_id) const;


private:
    std::unique_ptr<detail::account_eci_history_api_impl> _impl;
};
} // namespace account_eci_history
} // namespace deip

FC_API(deip::account_eci_history::account_eci_history_api,
       (get_eci_history_by_account_and_discipline))