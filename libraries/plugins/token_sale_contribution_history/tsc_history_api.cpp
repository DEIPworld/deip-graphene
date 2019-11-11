#include <deip/token_sale_contribution_history/tsc_history_api.hpp>
#include <deip/token_sale_contribution_history/tsc_history_plugin.hpp>
#include <deip/token_sale_contribution_history/tsc_history_object.hpp>
#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/token_sale_contribution_history/tsc_operation_object.hpp>
#include <map>

namespace deip {
namespace tsc_history {

namespace detail {
class tsc_history_api_impl
{
public:
    deip::app::application& _app;

public:
    tsc_history_api_impl(deip::app::application& app)
        : _app(app)
    {
    }

    template <typename history_object_type>
    std::vector<applied_tsc_operation> get_contributions_history_by_contributor(const account_name_type& contributor) const
    {
        std::vector<applied_tsc_operation> result;

        const auto db = _app.chain_database();

        const auto& idx = db->get_index<tsc_operations_full_history_index>().indices().get<by_account>().equal_range(contributor);

        auto it = idx.first;
        const auto it_end = idx.second;
        while (it != it_end)
        {
            result.push_back(db->get(it->op));
            ++it;
        }

        return result;
    }

    template <typename history_object_type>
    std::vector<applied_tsc_operation> get_contributions_history_by_contributor_and_research(const account_name_type& contributor, const research_id_type& research_id) const
    {
        std::vector<applied_tsc_operation> result;

        const auto db = _app.chain_database();

        const auto& idx = db->get_index<tsc_operations_full_history_index>().indices().get<by_account_and_research>().equal_range(std::make_tuple(contributor, research_id));

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

tsc_history_api::tsc_history_api(const deip::app::api_context& ctx)
    : _impl(new detail::tsc_history_api_impl(ctx.app))
{
}
tsc_history_api::~tsc_history_api()
{
}

void tsc_history_api::on_api_startup() {
}

std::vector<applied_tsc_operation>
tsc_history_api::get_contributions_history_by_contributor(const account_name_type &contributor) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        return _impl->get_contributions_history_by_contributor<all_tsc_operations_history_object>(contributor);
    });
}

std::vector<applied_tsc_operation>
tsc_history_api::get_contributions_history_by_contributor_and_research(const account_name_type& contributor, const research_id_type& research_id) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        return _impl->get_contributions_history_by_contributor_and_research<all_tsc_operations_history_object>(contributor, research_id);
    });
}

} // namespace tcp_history
} // namespace deip