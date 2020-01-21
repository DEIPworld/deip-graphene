#pragma once

#include <fc/api.hpp>
#include <deip/research_eci_history/applied_research_eci_operation.hpp>

namespace deip {
namespace app {
struct api_context;
}
} // namespace deip

namespace deip {
namespace research_eci_history {

namespace detail {
class research_eci_history_api_impl;
}

class research_eci_history_api
{
public:
    research_eci_history_api(const deip::app::api_context& ctx);
    ~research_eci_history_api();

    void on_api_startup();

    std::vector<applied_research_eci_operation>
    get_eci_history_by_research_and_discipline(const research_id_type& research_id, const discipline_id_type& discipline_id) const;


private:
    std::unique_ptr<detail::research_eci_history_api_impl> _impl;
};
} // namespace research_eci_history
} // namespace deip

FC_API(deip::research_eci_history::research_eci_history_api,
       (get_eci_history_by_research_and_discipline))