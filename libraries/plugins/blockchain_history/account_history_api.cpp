#include <deip/blockchain_history/account_history_api.hpp>
#include <deip/blockchain_history/account_history_plugin.hpp>
#include <deip/blockchain_history/account_history_object.hpp>
#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/blockchain_history/operation_objects.hpp>
#include <map>

namespace deip {
namespace blockchain_history {

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

    template <typename history_object_type, typename fill_result_functor>
    void get_history(const std::string& account, uint64_t from, uint32_t limit, fill_result_functor& funct) const
    {
        static const uint32_t max_history_depth = 100;

        const auto db = _app.chain_database();

        FC_ASSERT(limit > 0, "Limit must be greater than zero");
        FC_ASSERT(limit <= max_history_depth, "Limit of ${l} is greater than maxmimum allowed ${2}",
                  ("l", limit)("2", max_history_depth));
        FC_ASSERT(from >= limit, "From must be greater than limit");

        std::map<uint32_t, applied_operation> result;

        const auto& idx = db->get_index<history_index<history_object_type>>().indices().get<by_account>();
        auto itr = idx.lower_bound(boost::make_tuple(account, from));
        if (itr != idx.end())
        {
            auto end = idx.upper_bound(boost::make_tuple(account, int64_t(0)));
            int64_t pos = int64_t(itr->sequence) - limit;
            if (pos > 0)
            {
                end = idx.lower_bound(boost::make_tuple(account, pos));
            }
            while (itr != end)
            {
                funct(*itr);
                ++itr;
            }
        }
    }

    template <typename history_object_type>
    std::map<uint32_t, applied_operation> get_history(const std::string& account, uint64_t from, uint32_t limit) const
    {
        std::map<uint32_t, applied_operation> result;

         const auto db = _app.chain_database();
         
         auto fill_funct = [&](const history_object_type& hobj) { result[hobj.sequence] = db->get(hobj.op); };
        this->template get_history<history_object_type>(account, from, limit, fill_funct);
         return result;
    }
};
} // namespace detail

account_history_api::account_history_api(const deip::app::api_context& ctx)
    : _impl(new detail::account_history_api_impl(ctx.app))
{
}
account_history_api::~account_history_api()
{
}

void account_history_api::on_api_startup()
{
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_deip_to_deip_transfers(const std::string& account, uint64_t from, uint32_t limit) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock(
        [&]() { return _impl->get_history<transfers_to_deip_history_object>(account, from, limit); });
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_deip_to_common_tokens_transfers(const std::string& account, uint64_t from, uint32_t limit) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock(
        [&]() { return _impl->get_history<transfers_to_common_tokens_history_object>(account, from, limit); });
}

std::map<uint32_t, applied_operation>
account_history_api::get_account_history(const std::string& account, uint64_t from, uint32_t limit) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->get_history<account_history_object>(account, from, limit); });
}

} // namespace blockchain_history
} // namespace deip