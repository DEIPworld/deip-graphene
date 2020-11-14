#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_asset.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/proposal_history/proposal_history_api.hpp>
#include <deip/proposal_history/proposal_history_plugin.hpp>
#include <deip/proposal_history/proposal_state_object.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace deip {
namespace proposal_history {

using namespace boost::gregorian;

namespace detail {

class proposal_history_api_impl
{
public:
    deip::app::application& _app;

public:
    proposal_history_api_impl(deip::app::application& app)
        : _app(app)
    {
    }

    // std::vector<account_revenue_income_history_api_obj> get_account_revenue_history_by_security_token(const account_name_type& account,
    //                                                                                                   const string& security_token_symbol,
    //                                                                                                   const account_revenue_income_history_id_type& cursor,
    //                                                                                                   const fc::optional<uint16_t> step_opt,
    //                                                                                                   const fc::optional<string> target_asset_symbol_opt) const
    // {
    //     std::vector<account_revenue_income_history_api_obj> results;

    //     return results;
    // }


};
} // namespace detail

proposal_history_api::proposal_history_api(const deip::app::api_context& ctx)
    : _impl(new detail::proposal_history_api_impl(ctx.app))
{
}

proposal_history_api::~proposal_history_api()
{
}

void proposal_history_api::on_api_startup()
{
}

// std::vector<account_revenue_income_history_api_obj> proposal_history_api::get_account_revenue_history_by_security_token(
//     const account_name_type& account,
//     const string& security_token_symbol,
//     const account_revenue_income_history_id_type& cursor,
//     const fc::optional<uint16_t> step,
//     const fc::optional<string> target_asset_symbol) const
// {
//     const auto db = _impl->_app.chain_database();
//     return db->with_read_lock([&]() {
//         return _impl->get_account_revenue_history_by_security_token(account, security_token_symbol, cursor, step, target_asset_symbol);
//     });
// }

} // namespace proposal_history
} // namespace deip