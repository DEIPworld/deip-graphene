#pragma once

#include <fc/api.hpp>
#include <deip/chain/schema/deip_object_types.hpp>
#include <deip/eci_history/eci_history_api_objects.hpp>

namespace deip {
namespace app {
struct api_context;
}
} // namespace deip

namespace deip {
namespace eci_history {

using namespace std;
using namespace deip::chain;

namespace detail {
class eci_history_api_impl;
}

enum class eci_stat_period_step : uint16_t
{
    unknown = 0,
    day = 1,
    month = 2,

    FIRST = day,
    LAST = month
};

class eci_history_api
{
public:
    eci_history_api(const deip::app::api_context& ctx);
    ~eci_history_api();

    void on_api_startup();

    std::vector<research_content_eci_history_api_obj> get_eci_history_by_research_content_and_discipline(const research_content_id_type& research_content_id, const discipline_id_type& discipline_id) const;

    std::vector<research_eci_history_api_obj> get_eci_history_by_research_and_discipline(const research_id_type& research_id, const discipline_id_type& discipline_id) const;

    std::vector<account_eci_history_api_obj> get_account_eci_history(const account_name_type& account,
                                                                     const fc::optional<external_id_type> discipline_filter,
                                                                     const fc::optional<fc::time_point_sec> from_filter,
                                                                     const fc::optional<fc::time_point_sec> to_filter,
                                                                     const fc::optional<uint16_t> contribution_type_filter,
                                                                     const fc::optional<uint16_t> assessment_criteria_type_filter) const;

    std::map<account_name_type, account_eci_stats_api_obj> get_accounts_eci_stats(const fc::optional<external_id_type> discipline_filter,
                                                                                  const fc::optional<fc::time_point_sec> from_filter,
                                                                                  const fc::optional<fc::time_point_sec> to_filter,
                                                                                  const fc::optional<uint16_t> contribution_type_filter,
                                                                                  const fc::optional<uint16_t> assessment_criteria_type_filter) const;

    std::map<external_id_type, std::vector<discipline_eci_stats_api_obj>> get_disciplines_eci_stats_history(const fc::optional<fc::time_point_sec> from_filter,
                                                                                                            const fc::optional<fc::time_point_sec> to_filter,
                                                                                                            const fc::optional<uint16_t> step_filter) const;

    std::map<external_id_type, discipline_eci_stats_api_obj> get_disciplines_eci_stats() const;

private:
    std::unique_ptr<detail::eci_history_api_impl> _impl;
};
} // namespace eci_history
} // namespace deip


FC_REFLECT_ENUM(deip::eci_history::eci_stat_period_step,
  (unknown)
  (day)
  (month)
)


FC_API(deip::eci_history::eci_history_api,

  (get_eci_history_by_research_content_and_discipline)
  (get_eci_history_by_research_and_discipline)
  (get_account_eci_history)
  (get_accounts_eci_stats)
  (get_disciplines_eci_stats_history)
  (get_disciplines_eci_stats)

)