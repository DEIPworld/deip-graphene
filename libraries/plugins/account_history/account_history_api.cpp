#include <deip/account_history/account_history_api.hpp>
#include <deip/account_history/account_history_plugin.hpp>
#include <deip/account_history/account_history_object.hpp>
#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/chain/operation_object.hpp>
#include <map>

#define MAX_HISTORY_DEPTH 10000

namespace deip {
namespace account_history {

namespace detail {
class account_history_api_impl
{
public:
    deip::app::application& _app;

public:
    account_history_api_impl(deip::app::application& app)
        : _app(app)
    {
    }

    template <typename history_object_type>
    std::map<uint32_t, applied_operation> get_history(const std::string& account, uint64_t from, uint32_t limit) const
    {
        const auto db = _app.chain_database();

        FC_ASSERT(limit <= MAX_HISTORY_DEPTH, "Limit of ${l} is greater than maxmimum allowed ${2}",
                  ("l", limit)("2", MAX_HISTORY_DEPTH));
        FC_ASSERT(from >= limit, "From must be greater than limit");

        const auto& idx = db->get_index<history_index<history_object_type>>().indices().get<by_account>();
        auto itr = idx.lower_bound(boost::make_tuple(account, from));
        auto end = idx.upper_bound(boost::make_tuple(account, std::max(int64_t(0), int64_t(itr->sequence) - limit)));

        std::map<uint32_t, applied_operation> result;
        while (itr != end)
        {
            result[itr->sequence] = db->get(itr->op);
            ++itr;
        }
        return std::move(result);
    }
};
} // namespace detail

account_history_api::account_history_api(const deip::app::api_context& ctx)
{
    my = std::make_shared<detail::account_history_api_impl>(ctx.app);
}

void account_history_api::on_api_startup()
{
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_deip_to_deip_transfers(const std::string& account, uint64_t from, uint32_t limit) const
{
    return my->_app.chain_database()->with_read_lock(
        [&]() { return my->get_history<transfers_to_deip_history_object>(account, from, limit); });
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_scr_to_sp_transfers(const std::string& account, uint64_t from, uint32_t limit) const
{
    return my->_app.chain_database()->with_read_lock(
        [&]() { return my->get_history<transfers_to_common_tokens_history_object>(account, from, limit); });
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_history(const std::string& account, uint64_t from, uint32_t limit) const
{
    return my->_app.chain_database()->with_read_lock(
        [&]() { return my->get_history<account_history_object>(account, from, limit); });
}

} // namespace account_history
} // namespace deip