#include <deip/ip_protection_history/ip_protection_history_api.hpp>
#include <deip/ip_protection_history/ip_protection_history_plugin.hpp>
#include <deip/ip_protection_history/ip_protection_history_object.hpp>
#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/ip_protection_history/ip_protection_operation_object.hpp>
#include <map>

namespace deip {
namespace ip_protection_history {

namespace detail {
class ip_protection_history_api_impl
{
public:
    deip::app::application& _app;

public:
    ip_protection_history_api_impl(deip::app::application& app)
        : _app(app)
    {
    }

    template <typename history_object_type>
    applied_ip_protection_operation get_history(const std::string& content_hash) const
    {
        const auto db = _app.chain_database();

        const auto& idx = db->get_index<ip_protection_operations_full_history_index>().indices().get<by_content_hash>();
        auto itr = idx.find(content_hash, fc::strcmp_less());

        return db->get(itr->op);
    }
};
} // namespace detail

ip_protection_history_api::ip_protection_history_api(const deip::app::api_context& ctx)
    : _impl(new detail::ip_protection_history_api_impl(ctx.app))
{
}
ip_protection_history_api::~ip_protection_history_api()
{
}

void ip_protection_history_api::on_api_startup() {
}

applied_ip_protection_operation
ip_protection_history_api::get_content_history(const std::string& content_hash) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        return _impl->get_history<all_ip_protection_operations_history_object>(content_hash);
    });
}

} // namespace ip_protection_history
} // namespace deip
