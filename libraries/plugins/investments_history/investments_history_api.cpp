#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_asset.hpp>
#include <deip/chain/services/dbs_research.hpp>
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
                                                                                                      const string& security_token_symbol,
                                                                                                      const account_revenue_income_history_id_type& cursor,
                                                                                                      const fc::optional<uint16_t> step_opt,
                                                                                                      const fc::optional<string> target_asset_symbol_opt) const
    {
        std::vector<account_revenue_income_history_api_obj> results;

        const auto& db = _app.chain_database();
        const auto& asset_service = db->obtain_service<chain::dbs_asset>();
        const auto& account_revenue_income_hist_idx = db->get_index<account_revenue_income_history_index>()
            .indices()
            .get<by_account_and_security_token_and_cursor>();

        const revenue_period_step step = step_opt.valid() ? static_cast<revenue_period_step>(*step_opt) : revenue_period_step::unknown;

        asset_symbol_type target_asset_symbol = DEIP_USD_SYMBOL;
        if (target_asset_symbol_opt.valid())
        {
            const auto& target_asset = asset_service.get_asset_by_string_symbol(*target_asset_symbol_opt);
            target_asset_symbol = target_asset.symbol;
        }

        const auto& security_token_opt = asset_service.get_asset_by_string_symbol_if_exists(security_token_symbol);
        if (!security_token_opt.valid())
        {
            return results;
        }

        const asset_object& security_token = *security_token_opt;
        if (static_cast<asset_type>(security_token.type) != asset_type::research_security_token)
        {
            return results;
        }

        const auto& security_token_api = app::asset_api_obj(security_token);

        std::multimap<time_point_sec, asset> revenue_by_step;

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = account_revenue_income_hist_idx.lower_bound(std::make_tuple(account, security_token.symbol, cursor)); limit-- && itr != account_revenue_income_hist_idx.end() && itr->account == account && itr->security_token_symbol == security_token.symbol; ++itr)
        {
            const auto& hist = *itr;

            const price rate = price(asset(1, target_asset_symbol), asset(1, hist.revenue.symbol));
            const asset converted_revenue = hist.revenue * rate;

            if (step == revenue_period_step::unknown)
            {
                revenue_by_step.insert(std::make_pair(hist.timestamp, converted_revenue));
            }
            else
            {
                const string& hist_timestamp = hist.timestamp.to_non_delimited_iso_string();
                const date day_date = date_from_iso_string(string(hist_timestamp.substr(0, hist_timestamp.find("T"))));
                const date hist_date = step == revenue_period_step::day ? day_date : day_date.end_of_month();
                const time_point_sec timestamp = time_point_sec::from_iso_string(to_iso_extended_string(hist_date) + "T00:00:00");

                const auto& entry_itr = revenue_by_step.find(timestamp);
                if (entry_itr != revenue_by_step.end())
                {
                    entry_itr->second += converted_revenue;
                }
                else
                {
                    revenue_by_step.insert(std::make_pair(timestamp, converted_revenue));
                }
            }
        }

        for (const auto& pair : revenue_by_step)
        {
            results.push_back(account_revenue_income_history_api_obj(account, security_token_api, pair.second, pair.first));
        }
        
        return results;
    }


    std::vector<account_revenue_income_history_api_obj> get_account_revenue_history(const account_name_type& account,
                                                                                    const account_revenue_income_history_id_type& cursor) const
    {
        std::vector<account_revenue_income_history_api_obj> results;

        const auto& db = _app.chain_database();
        const auto& asset_service = db->obtain_service<chain::dbs_asset>();

        const auto& account_revenue_income_hist_idx = db->get_index<account_revenue_income_history_index>()
            .indices()
            .get<by_account_and_cursor>();

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = account_revenue_income_hist_idx.lower_bound(std::make_tuple(account, cursor)); limit-- && itr != account_revenue_income_hist_idx.end() && itr->account == account; ++itr)
        {
            const auto& hist = *itr;
            const auto& security_token = asset_service.get_asset_by_symbol(hist.security_token_symbol);

            const auto& security_token_api = app::asset_api_obj(security_token);

            results.push_back(account_revenue_income_history_api_obj(account, security_token_api, hist.revenue, hist.timestamp));
        }

        return results;
    }


    std::vector<account_revenue_income_history_api_obj> get_security_token_revenue_history(const string& security_token_symbol,
                                                                                           const account_revenue_income_history_id_type& cursor) const
    {
        std::vector<account_revenue_income_history_api_obj> results;

        const auto& db = _app.chain_database();
        const auto& asset_service = db->obtain_service<chain::dbs_asset>();

        const auto& account_revenue_income_hist_idx = db->get_index<account_revenue_income_history_index>()
            .indices()
            .get<by_security_token_and_cursor>();

        const auto& security_token_opt = asset_service.get_asset_by_string_symbol_if_exists(security_token_symbol);
        if (!security_token_opt.valid())
        {
            return results;
        }

        const asset_object& security_token = *security_token_opt;
        if (static_cast<asset_type>(security_token.type) != asset_type::research_security_token)
        {
            return results;
        }

        const auto& security_token_api = app::asset_api_obj(security_token);

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = account_revenue_income_hist_idx.lower_bound(std::make_tuple(security_token.symbol, cursor)); limit-- && itr != account_revenue_income_hist_idx.end() && itr->security_token_symbol == security_token.symbol; ++itr)
        {
            const auto& hist = *itr;
            results.push_back(account_revenue_income_history_api_obj(hist.account, security_token_api, hist.revenue, hist.timestamp));
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
    const string& security_token_symbol,
    const account_revenue_income_history_id_type& cursor,
    const fc::optional<uint16_t> step,
    const fc::optional<string> target_asset_symbol) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        return _impl->get_account_revenue_history_by_security_token(account, security_token_symbol, cursor, step, target_asset_symbol);
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
investments_history_api::get_security_token_revenue_history(const string& security_token_symbol,
                                                            const account_revenue_income_history_id_type& cursor) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock(
        [&]() { return _impl->get_security_token_revenue_history(security_token_symbol, cursor); });
}


} // namespace investments_history
} // namespace deip