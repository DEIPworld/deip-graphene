#pragma once

#include <fc/api.hpp>
#include <deip/token_sale_contribution_history/applied_tsc_operation.hpp>

namespace deip {
namespace app {
struct api_context;
}
} // namespace deip

namespace deip {
namespace tsc_history {

namespace detail {
class tsc_history_api_impl;
}

class tsc_history_api
{
public:
    tsc_history_api(const deip::app::api_context& ctx);
    ~tsc_history_api();

    void on_api_startup();

    std::vector<applied_tsc_operation>
    get_contributions_history_by_contributor(const account_name_type& contributor) const;

    std::vector<applied_tsc_operation>
    get_contributions_history_by_contributor_and_research(const account_name_type& contributor, const research_id_type& research_id) const;

    std::vector<applied_tsc_operation>
    get_contributions_history_by_research(const research_id_type& research_id) const;

    std::vector<applied_tsc_operation>
    get_contributions_history_by_token_sale(const research_token_sale_id_type& research_token_sale_id) const;

private:
    std::unique_ptr<detail::tsc_history_api_impl> _impl;
};
} // namespace token_sale_contribution_history
} // namespace deip

FC_API(deip::tsc_history::tsc_history_api,
      (get_contributions_history_by_contributor)
      (get_contributions_history_by_contributor_and_research)
      (get_contributions_history_by_research)
      (get_contributions_history_by_token_sale)
)