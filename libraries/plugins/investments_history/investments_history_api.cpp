#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_security_token.hpp>
#include <deip/investments_history/investments_history_api.hpp>
#include <deip/investments_history/investments_history_plugin.hpp>
#include <deip/investments_history/account_revenue_income_history_object.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace deip {
namespace investments_history {

using namespace boost::gregorian;

namespace detail {
  
class investments_history_api_impl
{
public:
    deip::app::application& _app;

public:
    investments_history_api_impl(deip::app::application& app)
        : _app(app)
    {
    }

    std::vector<account_revenue_income_history_api_obj> get_account_revenue_history_by_security_token(const account_name_type& account,
                                                                                                      const external_id_type& security_token_external_id,
                                                                                                      const account_revenue_income_history_id_type& cursor) const
    {
        std::vector<account_revenue_income_history_api_obj> results;

        const auto& db = _app.chain_database();
        const auto& account_revenue_income_hist_idx = db->get_index<account_revenue_income_history_index>()
            .indices()
            .get<by_account_and_security_token_and_cursor>();

        const auto& security_token_service = db->obtain_service<chain::dbs_security_token>();

        const auto& security_token = security_token_service.get_security_token(security_token_external_id);
        const auto& security_token_api = app::security_token_api_obj(security_token);

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = account_revenue_income_hist_idx.lower_bound(std::make_tuple(account, security_token_external_id, cursor)); limit-- && itr != account_revenue_income_hist_idx.end() && itr->account == account && itr->security_token == security_token_external_id; ++itr)
        {
            const auto& hist = *itr;
            results.push_back(account_revenue_income_history_api_obj(hist, security_token_api));
        }

        return results;
    }


    std::vector<account_revenue_income_history_api_obj> get_account_revenue_history(const account_name_type& account,
                                                                                    const account_revenue_income_history_id_type& cursor) const
    {
        std::vector<account_revenue_income_history_api_obj> results;

        const auto& db = _app.chain_database();
        const auto& account_revenue_income_hist_idx = db->get_index<account_revenue_income_history_index>()
            .indices()
            .get<by_account_and_cursor>();

        const auto& security_token_service = db->obtain_service<chain::dbs_security_token>();

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = account_revenue_income_hist_idx.lower_bound(std::make_tuple(account, cursor)); limit-- && itr != account_revenue_income_hist_idx.end() && itr->account == account; ++itr)
        {
            const auto& hist = *itr;

            const auto& security_token = security_token_service.get_security_token(hist.security_token);
            const auto& security_token_api = app::security_token_api_obj(security_token);

            results.push_back(account_revenue_income_history_api_obj(hist, security_token_api));
        }

        return results;
    }


    std::vector<account_revenue_income_history_api_obj> get_security_token_revenue_history(const external_id_type& security_token_external_id,
                                                                                           const account_revenue_income_history_id_type& cursor) const
    {
        std::vector<account_revenue_income_history_api_obj> results;

        const auto& db = _app.chain_database();
        const auto& account_revenue_income_hist_idx = db->get_index<account_revenue_income_history_index>()
            .indices()
            .get<by_security_token_and_cursor>();

        const auto& security_token_service = db->obtain_service<chain::dbs_security_token>();

        const auto& security_token = security_token_service.get_security_token(security_token_external_id);
        const auto& security_token_api = app::security_token_api_obj(security_token);

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = account_revenue_income_hist_idx.lower_bound(std::make_tuple(security_token_external_id, cursor)); limit-- && itr != account_revenue_income_hist_idx.end() && itr->security_token == security_token_external_id; ++itr)
        {
            const auto& hist = *itr;
            results.push_back(account_revenue_income_history_api_obj(hist, security_token_api));
        }

        return results;
    }

};
} // namespace detail

investments_history_api::investments_history_api(const deip::app::api_context& ctx)
    : _impl(new detail::investments_history_api_impl(ctx.app))
{
}

investments_history_api::~investments_history_api()
{
}

void investments_history_api::on_api_startup()
{
}

std::vector<account_revenue_income_history_api_obj>
investments_history_api::get_account_revenue_history_by_security_token(
    const account_name_type& account,
    const external_id_type& security_token_external_id,
    const account_revenue_income_history_id_type& cursor) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        return _impl->get_account_revenue_history_by_security_token(account, security_token_external_id, cursor);
    });
}

std::vector<account_revenue_income_history_api_obj>
investments_history_api::get_account_revenue_history(const account_name_type& account,
                                                     const account_revenue_income_history_id_type& cursor) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock(
        [&]() { return _impl->get_account_revenue_history(account, cursor); });
}

std::vector<account_revenue_income_history_api_obj>
investments_history_api::get_security_token_revenue_history(const external_id_type& security_token_external_id,
                                                            const account_revenue_income_history_id_type& cursor) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock(
        [&]() { return _impl->get_security_token_revenue_history(security_token_external_id, cursor); });
}


} // namespace investments_history
} // namespace deip