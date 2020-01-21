#include <deip/research_content_eci_history/research_content_eci_history_api.hpp>
#include <deip/research_content_eci_history/research_content_eci_history_plugin.hpp>
#include <deip/research_content_eci_history/research_content_eci_history_object.hpp>
#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/research_content_eci_history/research_content_eci_operation_object.hpp>
#include <map>

namespace deip {
namespace research_content_eci_history {

namespace detail {
class research_content_eci_history_api_impl
{
public:
    deip::app::application& _app;

public:
    research_content_eci_history_api_impl(deip::app::application& app)
        : _app(app)
    {
    }

    template <typename history_object_type>
    std::vector<applied_research_content_eci_operation>
    get_eci_history_by_content_and_discipline(const research_content_id_type& research_content_id, const discipline_id_type& discipline_id) const
    {
        std::vector<applied_research_content_eci_operation> result;

        const auto db = _app.chain_database();

        const auto& idx = db->get_index<research_content_eci_operations_history_index>().indices().get<by_content_and_discipline>().equal_range(std::make_tuple(research_content_id, discipline_id));

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

research_content_eci_history_api::research_content_eci_history_api(const deip::app::api_context& ctx)
    : _impl(new detail::research_content_eci_history_api_impl(ctx.app))
{
}

research_content_eci_history_api::~research_content_eci_history_api()
{
}

void research_content_eci_history_api::on_api_startup() {
}

std::vector<applied_research_content_eci_operation> research_content_eci_history_api::get_eci_history_by_content_and_discipline(
    const research_content_id_type& research_content_id, const discipline_id_type& discipline_id) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        return _impl->get_eci_history_by_content_and_discipline<research_content_eci_operations_history_object>(research_content_id, discipline_id);
    });
}

} // namespace research_content_eci_history
} // namespace deip