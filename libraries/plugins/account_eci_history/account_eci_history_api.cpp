#include <deip/account_eci_history/account_eci_history_api.hpp>
#include <deip/account_eci_history/account_eci_history_plugin.hpp>
#include <deip/account_eci_history/account_eci_history_object.hpp>
#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/account_eci_history/account_eci_operation_object.hpp>
#include <map>

namespace deip {
namespace account_eci_history {

namespace detail {
class account_eci_history_api_impl
{
public:
    deip::app::application& _app;

public:
    account_eci_history_api_impl(deip::app::application& app)
        : _app(app)
    {
    }

    template <typename history_object_type>
    std::vector<applied_account_eci_operation>
    get_eci_history_by_account_and_discipline(const account_name_type& account_name, const discipline_id_type& discipline_id) const
    {
        std::vector<applied_account_eci_operation> result;

        const auto db = _app.chain_database();

        const auto& idx = db->get_index<account_eci_operations_history_index>().indices().get<by_account_name_and_discipline>().equal_range(std::make_tuple(account_name, discipline_id));

        auto it = idx.first;
        const auto it_end = idx.second;
        while (it != it_end)
        {
            result.push_back(db->get(it->op));
            ++it;
        }

        return result;
    }
};
} // namespace detail

account_eci_history_api::account_eci_history_api(const deip::app::api_context& ctx)
    : _impl(new detail::account_eci_history_api_impl(ctx.app))
{
}
account_eci_history_api::~account_eci_history_api()
{
}

void account_eci_history_api::on_api_startup() {
}

std::vector<applied_account_eci_operation>
account_eci_history_api::get_eci_history_by_account_and_discipline(const account_name_type& account_name, const discipline_id_type& discipline_id) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        return _impl->get_eci_history_by_account_and_discipline<account_eci_operations_history_object>(account_name, discipline_id);
    });
}

} // namespace account_eci_history
} // namespace deip
