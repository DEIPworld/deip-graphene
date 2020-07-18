#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/chain/services/dbs_expertise_contribution.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/eci_history/eci_history_api.hpp>
#include <deip/eci_history/eci_history_plugin.hpp>
#include <deip/eci_history/research_eci_history_object.hpp>
#include <deip/eci_history/research_content_eci_history_object.hpp>
#include <deip/eci_history/account_eci_history_object.hpp>
#include <deip/eci_history/discipline_eci_history_object.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace deip {
namespace eci_history {

using deip::chain::expertise_contribution_type;
using namespace boost::gregorian;

namespace detail {
  
class eci_history_api_impl
{
public:
    deip::app::application& _app;

public:
    eci_history_api_impl(deip::app::application& app)
        : _app(app)
    {
    }

    std::vector<research_content_eci_history_api_obj> get_research_content_eci_history(const external_id_type& research_content_external_id,
                                                                                       const research_content_eci_history_id_type& cursor,
                                                                                       const eci_filter& filter) const
    {
        std::vector<research_content_eci_history_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& research_content_hist_idx = db->get_index<research_content_eci_history_index>().indices().get<by_research_content_and_cursor>();
        const auto& research_service = db->obtain_service<chain::dbs_research>();
        const auto& research_content_service = db->obtain_service<chain::dbs_research_content>();
        const auto& research_groups_service = db->obtain_service<chain::dbs_research_group>();

        const auto& research_content_opt = research_content_service.get_research_content_if_exists(research_content_external_id);
        if (!research_content_opt.valid())
        {
            return result;
        }

        const research_content_object& research_content = *research_content_opt;

        const auto& research = research_service.get_research(research_content.research_id);
        const auto& research_group = research_groups_service.get_research_group(research.research_group_id);

        const auto& research_group_api = app::research_group_api_obj(research_group);
        const auto& research_api = app::research_api_obj(research, {}, research_group_api);
        const auto& research_content_api = app::research_content_api_obj(research_content);

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = research_content_hist_idx.lower_bound(std::make_tuple(research_content.id, cursor)); limit-- && itr != research_content_hist_idx.end() && itr->research_content_id == research_content.id; ++itr)
        {
            const auto& hist = *itr;

            if (filter_record(filter, hist.discipline_external_id, hist.timestamp, hist.contribution_type, hist.assessment_criterias))
            {
                fc::optional<app::research_content_api_obj> research_content_api_opt;
                fc::optional<app::research_api_obj> research_api_opt;
                fc::optional<app::research_group_api_obj> research_group_api_opt;
                fc::optional<app::review_api_obj> review_api_opt;
                fc::optional<app::review_vote_api_obj> review_vote_api_opt;
                extract_contribution_optional_objects(hist.contribution_type, hist.contribution_id, research_content_api_opt, research_api_opt, research_group_api_opt, review_api_opt, review_vote_api_opt);

                const auto& previous_eci = result.size() > 0 ? result[result.size() - 1].eci : share_type(0);
                const auto& delta = get_modified_eci_delta(hist.delta, hist.assessment_criterias, filter.assessment_criteria_type);
                const auto& eci = previous_eci + delta;

                result.push_back(research_content_eci_history_api_obj(
                  eci, 
                  delta,
                  hist,
                  research_content_api,
                  research_api,
                  research_group_api,
                  review_api_opt,
                  review_vote_api_opt
                ));
            }
            else
            {
                limit++;
            }
        }

        return result;
    }


    fc::optional<research_content_eci_stats_api_obj> get_research_content_eci_stats(const external_id_type& research_content_external_id,
                                                                                    const eci_filter& filter) const
    {
        const auto& db = _app.chain_database();
        const auto& research_content_service = db->obtain_service<chain::dbs_research_content>();
        
        fc::optional<research_content_eci_stats_api_obj> result;
        const auto& research_content_opt = research_content_service.get_research_content_if_exists(research_content_external_id);

        if (!research_content_opt.valid())
        {
            return result;
        }

        const auto& stats = get_research_contents_eci_stats(filter);
        if (stats.find(research_content_external_id) != stats.end())
        {
            result = stats.at(research_content_external_id);
        }

        return result;
    }


    std::map<external_id_type, research_content_eci_stats_api_obj> get_research_contents_eci_stats(const eci_filter& filter) const
    {
        const auto& db = _app.chain_database();
        const auto& research_content_hist_idx = db->get_index<research_content_eci_history_index>().indices().get<by_research_content_id>();
        const auto& research_content_service = db->obtain_service<chain::dbs_research_content>();

        std::map<external_id_type, research_content_eci_stats_api_obj> result;

        const auto& research_contents = research_content_service.lookup_research_contents(research_content_id_type(0), DEIP_API_BULK_FETCH_LIMIT);
        std::vector<share_type> eci_scores;

        for (const research_content_object& research_content : research_contents)
        {
            auto itr_pair = research_content_hist_idx.equal_range(research_content.id);
            auto& itr = itr_pair.first;
            const auto& itr_end = itr_pair.second;

            vector<research_content_eci_stats_api_obj> history;

            while (itr != itr_end)
            {
                const auto& hist = *itr;

                if (filter_record(filter, hist.discipline_external_id, hist.timestamp, hist.contribution_type, hist.assessment_criterias))
                {
                    if (result.find(research_content.external_id) == result.end())
                    {
                        auto stats = research_content_eci_stats_api_obj();
                        stats.research_content_external_id = research_content.external_id;
                        result.insert(std::make_pair(research_content.external_id, stats));
                    }

                    auto& stats = result[research_content.external_id];

                    stats.previous_eci = stats.eci;
                    const auto& delta = get_modified_eci_delta(hist.delta, hist.assessment_criterias, filter.assessment_criteria_type);
                    stats.eci += delta;
                    stats.timestamp = hist.timestamp;
                    stats.contributions.insert(std::make_pair(hist.contribution_id, hist.contribution_type));

                    history.push_back(stats);
                }

                ++itr;
            }

            if (result.find(research_content.external_id) != result.end())
            {
                auto& stats = result[research_content.external_id];
                eci_scores.push_back(stats.eci);

                const auto& last_growth = calculate_growth_rate(stats.previous_eci, stats.eci);
                if (last_growth.valid())
                {
                    stats.last_growth_rate = *last_growth;
                }

                const auto& start_point_itr = std::find_if(history.begin(), history.end(),
                  [&](const research_content_eci_stats_api_obj& hist) { return hist.eci != share_type(0); });

                if (start_point_itr != history.end())
                {
                    const auto& start_point = *start_point_itr;
                    stats.starting_eci = start_point.eci;

                    const auto& growth_rate = calculate_growth_rate(start_point.eci, stats.eci);
                    if (growth_rate.valid())
                    {
                        stats.growth_rate = *growth_rate;
                    }
                }
            }
        }
        
        for (auto& pair : result)
        {
            auto& stats = pair.second;
            const auto& percentile_rank = calculate_percentile_rank(stats.eci, eci_scores);
            stats.percentile_rank = percentile_rank;
        }

        return result;
    }

    std::vector<research_eci_history_api_obj> get_research_eci_history(const external_id_type& research_external_id,
                                                                       const research_eci_history_id_type& cursor,
                                                                       const eci_filter& filter) const
    {
        std::vector<research_eci_history_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& research_hist_idx = db->get_index<research_eci_history_index>().indices().get<by_research_and_cursor>();
        const auto& research_service = db->obtain_service<chain::dbs_research>();
        const auto& research_groups_service = db->obtain_service<chain::dbs_research_group>();

        const auto& research_opt = research_service.get_research_if_exists(research_external_id);
        if (!research_opt.valid())
        {
            return result;
        }

        const research_object& research = *research_opt;

        const auto& research_group = research_groups_service.get_research_group(research.research_group_id);
        const auto& research_group_api = app::research_group_api_obj(research_group);
        const auto& research_api = app::research_api_obj(research, {}, research_group);

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = research_hist_idx.lower_bound(std::make_tuple(research.id, cursor)); limit-- && itr != research_hist_idx.end() && itr->research_id == research.id; ++itr)
        {
            const auto& hist = *itr;

            if (filter_record(filter, hist.discipline_external_id, hist.timestamp, hist.contribution_type, hist.assessment_criterias))
            {
                fc::optional<app::research_content_api_obj> research_content_api_opt;
                fc::optional<app::research_api_obj> research_api_opt;
                fc::optional<app::research_group_api_obj> research_group_api_opt;
                fc::optional<app::review_api_obj> review_api_opt;
                fc::optional<app::review_vote_api_obj> review_vote_api_opt;
                extract_contribution_optional_objects(hist.contribution_type, hist.contribution_id, research_content_api_opt, research_api_opt, research_group_api_opt, review_api_opt, review_vote_api_opt);

                const auto& previous_eci = result.size() > 0 ? result[result.size() - 1].eci : share_type(0);
                const auto& delta = get_modified_eci_delta(hist.delta, hist.assessment_criterias, filter.assessment_criteria_type);
                const auto& eci = previous_eci + delta;

                result.push_back(research_eci_history_api_obj(
                  eci, 
                  delta,
                  hist,
                  research_api,
                  research_group_api,
                  research_content_api_opt,
                  review_api_opt,
                  review_vote_api_opt
                ));
            }
            else 
            {
                limit++;
            }
        }

        return result;
    }

    fc::optional<research_eci_stats_api_obj> get_research_eci_stats(const external_id_type& research_external_id,
                                                                    const eci_filter& filter) const
    {
        const auto& db = _app.chain_database();
        const auto& research_service = db->obtain_service<chain::dbs_research>();
        
        fc::optional<research_eci_stats_api_obj> result;
        const auto& research_opt = research_service.get_research_if_exists(research_external_id);

        if (!research_opt.valid())
        {
            return result;
        }

        const auto& stats = get_researches_eci_stats(filter);
        if (stats.find(research_external_id) != stats.end())
        {
            result = stats.at(research_external_id);
        }

        return result;
    }



    std::map<external_id_type, research_eci_stats_api_obj> get_researches_eci_stats(const eci_filter& filter) const
    {
        const auto& db = _app.chain_database();
        const auto& research_hist_idx = db->get_index<research_eci_history_index>().indices().get<by_research_id>();
        const auto& research_service = db->obtain_service<chain::dbs_research>();

        std::map<external_id_type, research_eci_stats_api_obj> result;

        const auto& researches = research_service.lookup_researches(research_id_type(0), DEIP_API_BULK_FETCH_LIMIT);
        std::vector<share_type> eci_scores;

        for (const research_object& research : researches)
        {
            auto itr_pair = research_hist_idx.equal_range(research.id);
            auto& itr = itr_pair.first;
            const auto& itr_end = itr_pair.second;

            vector<research_eci_stats_api_obj> history;

            while (itr != itr_end)
            {
                const auto& hist = *itr;

                if (filter_record(filter, hist.discipline_external_id, hist.timestamp, hist.contribution_type, hist.assessment_criterias))
                {
                    if (result.find(research.external_id) == result.end())
                    {
                        auto stats = research_eci_stats_api_obj();
                        stats.research_external_id = research.external_id;
                        result.insert(std::make_pair(research.external_id, stats));
                    }

                    auto& stats = result[research.external_id];

                    stats.previous_eci = stats.eci;
                    const auto& delta = get_modified_eci_delta(hist.delta, hist.assessment_criterias, filter.assessment_criteria_type);
                    stats.eci += delta;
                    stats.timestamp = hist.timestamp;
                    stats.contributions.insert(std::make_pair(hist.contribution_id, hist.contribution_type));

                    history.push_back(stats);
                }

                ++itr;
            }

            if (result.find(research.external_id) != result.end())
            {
                auto& stats = result[research.external_id];
                eci_scores.push_back(stats.eci);

                const auto& last_growth = calculate_growth_rate(stats.previous_eci, stats.eci);
                if (last_growth.valid())
                {
                    stats.last_growth_rate = *last_growth;
                }

                const auto& start_point_itr = std::find_if(history.begin(), history.end(),
                  [&](const research_eci_stats_api_obj& hist) { return hist.eci != share_type(0); });

                if (start_point_itr != history.end())
                {
                    const auto& start_point = *start_point_itr;
                    stats.starting_eci = start_point.eci;

                    const auto& growth_rate = calculate_growth_rate(start_point.eci, stats.eci);
                    if (growth_rate.valid())
                    {
                        stats.growth_rate = *growth_rate;
                    }
                }
            }
        }
        
        for (auto& pair : result)
        {
            auto& stats = pair.second;
            const auto& percentile_rank = calculate_percentile_rank(stats.eci, eci_scores);
            stats.percentile_rank = percentile_rank;
        }

        return result;
    }

    std::vector<account_eci_history_api_obj> get_account_eci_history(const account_name_type& account,
                                                                     const account_eci_history_id_type& cursor,
                                                                     const eci_filter& filter) const
    {
        std::vector<account_eci_history_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& account_hist_idx = db->get_index<account_eci_history_index>().indices().get<by_account_and_cursor>();
        const auto& accounts_service = db->obtain_service<chain::dbs_account>();

        if (!accounts_service.account_exists(account))
        {
            return result;
        }

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = account_hist_idx.lower_bound(std::make_tuple(account, cursor)); limit-- && itr != account_hist_idx.end() && itr->account == account; ++itr)
        {
            const auto& hist = *itr;

            if (filter_record(filter, hist.discipline_external_id, hist.timestamp, hist.contribution_type, hist.assessment_criterias))
            {
                fc::optional<app::research_content_api_obj> research_content_api_opt;
                fc::optional<app::research_api_obj> research_api_opt;
                fc::optional<app::research_group_api_obj> research_group_api_opt;
                fc::optional<app::review_api_obj> review_api_opt;
                fc::optional<app::review_vote_api_obj> review_vote_api_opt;
                extract_contribution_optional_objects(hist.contribution_type, hist.contribution_id, research_content_api_opt, research_api_opt, research_group_api_opt, review_api_opt, review_vote_api_opt);

                const auto& previous_eci = result.size() > 0 ? result[result.size() - 1].eci : share_type(0);
                const auto& delta = get_modified_eci_delta(hist.delta, hist.assessment_criterias, filter.assessment_criteria_type);
                const auto& eci = previous_eci + delta;

                result.push_back(account_eci_history_api_obj(
                  eci,
                  delta,
                  hist,
                  research_content_api_opt,
                  research_api_opt,
                  research_group_api_opt,
                  review_api_opt,
                  review_vote_api_opt
                ));
            }
            else
            {
                limit++;
            }
        }

        return result;
    }

    fc::optional<account_eci_stats_api_obj> get_account_eci_stats(const account_name_type& account,
                                                                  const eci_filter& filter) const
    {
        const auto& db = _app.chain_database();
        const auto& account_service = db->obtain_service<chain::dbs_account>();

        fc::optional<account_eci_stats_api_obj> result;
        const auto& account_opt = account_service.get_account_if_exists(account);

        if (!account_opt.valid())
        {
            return result;
        }

        const auto& stats = get_accounts_eci_stats(filter);
        if (stats.find(account) != stats.end())
        {
            result = stats.at(account);
        }

        return result;
    }


    std::map<account_name_type, account_eci_stats_api_obj> get_accounts_eci_stats(const eci_filter& filter) const
    {
        const auto& db = _app.chain_database();
        const auto& account_hist_idx = db->get_index<account_eci_history_index>().indices().get<by_account>();
        const auto& accounts_service = db->obtain_service<chain::dbs_account>();

        std::map<account_name_type, account_eci_stats_api_obj> result;

        const auto& accounts = accounts_service.lookup_user_accounts(account_name_type("a"), DEIP_API_BULK_FETCH_LIMIT);
        std::vector<share_type> eci_scores;

        for (const account_object& acc : accounts)
        {
            auto itr_pair = account_hist_idx.equal_range(acc.name);
            auto& itr = itr_pair.first;
            const auto& itr_end = itr_pair.second;

            vector<account_eci_stats_api_obj> history;

            while (itr != itr_end)
            {
                const auto& hist = *itr;

                if (filter_record(filter, hist.discipline_external_id, hist.timestamp, hist.contribution_type, hist.assessment_criterias))
                {
                    if (result.find(acc.name) == result.end())
                    {
                        auto stats = account_eci_stats_api_obj();
                        stats.account = acc.name;
                        stats.discipline_external_id = hist.discipline_external_id;
                        result.insert(std::make_pair(acc.name, stats));
                    }

                    auto& stats = result[acc.name];

                    stats.previous_eci = stats.eci;
                    const auto& delta = get_modified_eci_delta(hist.delta, hist.assessment_criterias, filter.assessment_criteria_type);
                    stats.eci += delta;
                    stats.timestamp = hist.timestamp;
                    stats.researches.insert(hist.researches.begin(), hist.researches.end());
                    stats.contributions.insert(std::make_pair(hist.contribution_id, hist.contribution_type));

                    history.push_back(stats);
                }

                ++itr;
            }

            if (result.find(acc.name) != result.end())
            {
                auto& stats = result[acc.name];
                eci_scores.push_back(stats.eci);

                const auto& last_growth = calculate_growth_rate(stats.previous_eci, stats.eci);
                if (last_growth.valid())
                {
                    stats.last_growth_rate = *last_growth;
                }

                const auto& start_point_itr = std::find_if(history.begin(), history.end(),
                  [&](const account_eci_stats_api_obj& hist) { return hist.eci != share_type(0); });

                if (start_point_itr != history.end())
                {
                    const auto& start_point = *start_point_itr;
                    stats.starting_eci = start_point.eci;

                    const auto& growth_rate = calculate_growth_rate(start_point.eci, stats.eci);
                    if (growth_rate.valid())
                    {
                        stats.growth_rate = *growth_rate;
                    }
                }
            }
        }
        
        for (auto& pair : result)
        {
            auto& stats = pair.second;
            const auto& percentile_rank = calculate_percentile_rank(stats.eci, eci_scores);
            stats.percentile_rank = percentile_rank;
        }

        return result;
    }

    std::vector<discipline_eci_history_api_obj> get_discipline_eci_history(const eci_filter& filter) const
    {
        std::vector<discipline_eci_history_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& discipline_hist_idx = db->get_index<discipline_eci_history_index>().indices().get<by_id>();
        const auto& disciplines_service = db->obtain_service<chain::dbs_discipline>();

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = discipline_hist_idx.lower_bound(discipline_eci_history_id_type(0)); limit-- && itr != discipline_hist_idx.end(); ++itr)
        {
            const discipline_eci_history_object& hist = *itr;

            if (filter_record(filter, hist.discipline_external_id, hist.timestamp, 0, {}))
            {
                for (const auto& diff : hist.contributions)
                {
                    if (filter.assessment_criteria_type.valid())
                    {
                        const uint16_t& assessment_criteria_type = *filter.assessment_criteria_type;
                        if (diff.assessment_criterias.find(assessment_criteria_type) == diff.assessment_criterias.end())
                        {
                            continue;
                        }
                    }

                    if (filter.contribution_type.valid())
                    {
                        const uint16_t& contribution_type = *filter.contribution_type;
                        if (contribution_type != diff.contribution_type)
                        {
                            continue;
                        }
                    }

                    fc::optional<app::research_content_api_obj> research_content_api_opt;
                    fc::optional<app::research_api_obj> research_api_opt;
                    fc::optional<app::research_group_api_obj> research_group_api_opt;
                    fc::optional<app::review_api_obj> review_api_opt;
                    fc::optional<app::review_vote_api_obj> review_vote_api_opt;
                    extract_contribution_optional_objects(diff.contribution_type, diff.contribution_id, research_content_api_opt, research_api_opt, research_group_api_opt, review_api_opt, review_vote_api_opt);

                    const auto& previous_eci = result.size() > 0 ? result[result.size() - 1].eci : share_type(0);
                    const auto& delta = get_modified_eci_delta(diff.diff(), diff.assessment_criterias, filter.assessment_criteria_type);
                    const auto& eci = previous_eci + delta;

                    result.push_back(discipline_eci_history_api_obj(
                      hist.discipline_external_id,
                      hist,
                      eci,
                      delta,
                      diff.contribution_type,
                      diff.contribution_id,
                      research_content_api_opt,
                      research_api_opt,
                      research_group_api_opt,
                      review_api_opt,
                      review_vote_api_opt
                    ));
                }          
            }
        }

        return result;
    }


    std::map<external_id_type, std::vector<discipline_eci_stats_api_obj>> get_disciplines_eci_stats_history(const fc::optional<fc::time_point_sec> from_filter,
                                                                                                            const fc::optional<fc::time_point_sec> to_filter,
                                                                                                            const fc::optional<uint16_t> step_filter) const
    {
        const auto& db = _app.chain_database();
        const auto& discipline_hist_idx = db->get_index<discipline_eci_history_index>().indices().get<by_discipline>();
        const auto& disciplines_service = db->obtain_service<chain::dbs_discipline>();

        std::map<external_id_type, std::vector<discipline_eci_stats_api_obj>> result;

        std::multimap<discipline_id_type, date> selection;
        eci_stat_period_step step = step_filter.valid() ? static_cast<eci_stat_period_step>(*step_filter) : eci_stat_period_step::unknown;

        auto filter = [&](const discipline_eci_history_object& hist) -> bool {
            if (from_filter.valid())
            {
                const fc::time_point_sec& from = *from_filter;
                if (hist.timestamp < from)
                {
                    return false;
                }
            }

            if (to_filter.valid())
            {
                const fc::time_point_sec& to = *to_filter;
                if (hist.timestamp > to)
                {
                    return false;
                }
            }

            if (step != eci_stat_period_step::unknown)
            {
                const string& hist_timestamp = hist.timestamp.to_non_delimited_iso_string();
                date hist_date = date_from_iso_string(string(hist_timestamp.substr(0, hist_timestamp.find("T"))));

                auto entries_pair = selection.equal_range(hist.discipline_id);
                if (entries_pair.first != entries_pair.second)
                {
                    auto& last_entry = *(--entries_pair.second);
                    date bound = step == eci_stat_period_step::day 
                      ? last_entry.second
                      : step == eci_stat_period_step::month
                        ? last_entry.second.end_of_month()
                        : last_entry.second;

                    if (hist_date > bound)
                    {
                        selection.insert(std::make_pair(hist.discipline_id, hist_date));
                        return true;
                    } 
                    else
                    {
                       return false;
                    }
                }
                else
                {
                    selection.insert(std::make_pair(hist.discipline_id, hist_date));
                    return true;
                }
            }

            return true;
        };

        const auto& disciplines = disciplines_service.lookup_disciplines(discipline_id_type(1), DEIP_API_BULK_FETCH_LIMIT);

        for (const discipline_object& discipline : disciplines)
        {
            std::vector<discipline_eci_stats_api_obj> records;

            auto itr_pair = discipline_hist_idx.equal_range(discipline.id);
            auto itr = itr_pair.first;
            const auto itr_end = itr_pair.second;
            while (itr != itr_end)
            {
                const discipline_eci_history_object& hist = *itr;

                if (filter(hist))
                {
                    std::map<uint16_t, assessment_criteria_value> assessment_criterias = extract_assessment_criterias(hist.assessment_criterias);
                    records.push_back(discipline_eci_stats_api_obj(
                        discipline.external_id,
                        fc::to_string(discipline.name),
                        hist.eci,
                        share_type(0),
                        share_type(0),
                        hist.total_eci,
                        hist.percentage,
                        {}, 
                        {},
                        assessment_criterias,
                        hist.timestamp
                    ));
                }

                ++itr;
            }

            result.insert(std::make_pair(discipline.external_id, records));
        }

        for (auto& res : result)
        {
            auto& records = res.second;
            const auto& start_point_itr = std::find_if(records.begin(), records.end(),
                [&](const discipline_eci_stats_api_obj& hist) { return hist.eci != share_type(0); });

            fc::optional<discipline_eci_stats_api_obj> start_point_opt;
            if (start_point_itr != records.end())
            {
                start_point_opt = *start_point_itr;
            }

            for (auto i = 0; i < records.size(); i++)
            {
                if (i == 0) continue;

                const auto& previous = records[i - 1];
                auto& current = records[i];

                const auto& last_growth_rate = calculate_growth_rate(previous.eci, current.eci);
                if (last_growth_rate.valid())
                {
                    current.last_growth_rate = *last_growth_rate;
                    current.previous_eci = previous.eci;
                }

                if (start_point_opt.valid())
                {
                    const auto& start_point = *start_point_opt;
                    current.starting_eci = start_point.eci;

                    const auto& growth_rate = calculate_growth_rate(start_point.eci, current.eci);
                    if (growth_rate.valid())
                    {
                        current.growth_rate = *growth_rate;
                    }
                }
            }
        }

        return result;
    }


    std::map<external_id_type, discipline_eci_stats_api_obj> get_disciplines_eci_last_stats() const
    {
        const auto& db = _app.chain_database();
        const auto& discipline_hist_idx = db->get_index<discipline_eci_history_index>().indices().get<by_discipline>();
        const auto& disciplines_service = db->obtain_service<chain::dbs_discipline>();

        std::map<external_id_type, discipline_eci_stats_api_obj> result;

        const auto& disciplines = disciplines_service.lookup_disciplines(discipline_id_type(1), DEIP_API_BULK_FETCH_LIMIT);
        for (const discipline_object& discipline : disciplines)
        {
            fc::optional<percent> last_growth_rate;
            share_type previous_eci = share_type(0);

            auto itr_pair = discipline_hist_idx.equal_range(discipline.id);
            if (itr_pair.first == itr_pair.second) 
            {
                continue;
            }

            const discipline_eci_history_object& hist = *(--itr_pair.second);
            if (itr_pair.second != itr_pair.first) 
            {
                const discipline_eci_history_object& previous = *(--itr_pair.second);
                const auto& last_growth = calculate_growth_rate(previous.eci, hist.eci);
                if (last_growth.valid())
                {
                    last_growth_rate = *last_growth;
                    previous_eci = previous.eci;
                }
            }

            std::map<uint16_t, assessment_criteria_value> assessment_criterias = extract_assessment_criterias(hist.assessment_criterias);
            result.insert(std::make_pair(discipline.external_id, discipline_eci_stats_api_obj(
                discipline.external_id,
                fc::to_string(discipline.name),
                hist.eci,
                previous_eci,
                previous_eci,
                hist.total_eci,
                hist.percentage,
                last_growth_rate,
                last_growth_rate,
                assessment_criterias,
                hist.timestamp
            )));
        }

        return result;
    }

private:
    const share_type get_modified_eci_delta(const share_type& delta,
                                            const flat_map<uint16_t, assessment_criteria_value>& assessment_criterias,
                                            const fc::optional<uint16_t> assessment_criteria_opt) const
    {
        if (assessment_criteria_opt.valid())
        {
            const uint16_t& assessment_criteria_type = *assessment_criteria_opt;
            const auto& itr = assessment_criterias.find(assessment_criteria_type);
            if (itr != assessment_criterias.end())
            {
                const assessment_criteria_value val = assessment_criterias.at(assessment_criteria_type);
                const share_type factor = val > 0 ? share_type(1) : share_type(-1);
                const std::string str("0." + std::to_string(abs(val)));
                const double modifier = std::stod(str);
                const double modified = abs(double(delta.value)) - (abs(double(delta.value)) * modifier);
                return share_type(std::round(modified)) * factor;
            }
        }

        return delta;
    }

    const fc::optional<percent> calculate_growth_rate(const share_type& past, const share_type& present) const
    {
        /*
            Growth Rate = [(Vpresent - Vpast) / Vpast] * 100
            Vpresent = present value
            VPast = past value
        */

        fc::optional<percent> result;
        if (past != share_type(0))
        {
            const share_type growth_rate = share_type(std::round(((double(present.value) - double(past.value)) / double(past.value)) * double(100) * DEIP_1_PERCENT));
            result = percent(growth_rate);
        }

        return result;
    }

    const percent calculate_percentile_rank(const share_type eci, vector<share_type> eci_scores) const
    {
        /*
            Percentile Rank = [(M + (0.5 * R)) / Y] * 100
            M = Number of Ranks below x
            R = Number of Ranks equals x
            Y = Total Number of Ranks
        */

        const share_type x_score = eci;
        std::sort(eci_scores.begin(), eci_scores.end());
        const auto& x_score_itr = std::find(eci_scores.begin(), eci_scores.end(), x_score);
        const int M = std::distance(eci_scores.begin(), x_score_itr);
        const int R = std::count_if(eci_scores.begin(), eci_scores.end(),
            [&](const share_type& eci_score) { return eci_score == x_score; });
        const int Y = eci_scores.size();
        const share_type percentile_rank = share_type(std::round(((double(M) + (double(0.5) * double(R))) / double(Y)) * double(100) * DEIP_1_PERCENT));
        return percent(percentile_rank);
    }

    const bool filter_record(const eci_filter& filter,
                             const external_id_type& discipline,
                             const fc::time_point_sec& timestamp,
                             const uint16_t& contribution_type,
                             const flat_map<uint16_t, assessment_criteria_value>& assessment_criterias) const
    {
        if (filter.discipline.valid())
        {
            const external_id_type& filter_discipline = *filter.discipline;
            if (discipline != filter_discipline)
            {
                return false;
            }
        }

        if (filter.from.valid())
        {
            const fc::time_point_sec& filter_from = *filter.from;
            if (timestamp < filter_from)
            {
                return false;
            }
        }

        if (filter.to.valid())
        {
            const fc::time_point_sec& filter_to = *filter.to;
            if (timestamp > filter_to)
            {
                return false;
            }
        }

        if (filter.contribution_type.valid())
        {
            const uint16_t& filter_contribution_type = *filter.contribution_type;
            if (contribution_type != filter_contribution_type)
            {
                return false;
            }
        }

        if (filter.assessment_criteria_type.valid())
        {
            const uint16_t& filter_assessment_criteria_type = *filter.assessment_criteria_type;
            if (assessment_criterias.find(filter_assessment_criteria_type) == assessment_criterias.end())
            {
                return false;
            }
        }

        return true;
    }

    const std::map<uint16_t, assessment_criteria_value> extract_assessment_criterias(const flat_map<uint16_t, assessment_criteria_value> assessment_criterias) const
    {
        std::map<uint16_t, assessment_criteria_value> result;
        for (uint16_t i = static_cast<uint16_t>(assessment_criteria::FIRST); i <= static_cast<uint16_t>(assessment_criteria::LAST); i++)
        {
            if (assessment_criterias.find(i) != assessment_criterias.end())
            {
                result.insert(std::make_pair(i, assessment_criterias.at(i)));
            }
            else
            {
                result.insert(std::make_pair(i, assessment_criteria_value(0)));
            }
        }

        return result;
    }

    const void extract_contribution_optional_objects(const uint16_t& type,
                                                     const int64_t& contribution_id,
                                                     fc::optional<app::research_content_api_obj>& research_content_api_opt,
                                                     fc::optional<app::research_api_obj>& research_api_opt,
                                                     fc::optional<app::research_group_api_obj>& research_group_api_opt,
                                                     fc::optional<app::review_api_obj>& review_api_opt,
                                                     fc::optional<app::review_vote_api_obj>& review_vote_api_opt) const
    {
        const auto& db = _app.chain_database();
        const auto& research_service = db->obtain_service<chain::dbs_research>();
        const auto& research_content_service = db->obtain_service<chain::dbs_research_content>();
        const auto& research_groups_service = db->obtain_service<chain::dbs_research_group>();
        const auto& reviews_service = db->obtain_service<chain::dbs_review>();
        const auto& review_votes_service = db->obtain_service<chain::dbs_review_vote>();

        const expertise_contribution_type& contribution_type = static_cast<expertise_contribution_type>(type);

        switch (contribution_type)
        {
            case expertise_contribution_type::publication: 
            {
                const auto& research_content = research_content_service.get_research_content(research_content_id_type(contribution_id));
                research_content_api_opt = app::research_content_api_obj(research_content);

                const auto& research = research_service.get_research(research_content.research_id);
                const auto& research_group = research_groups_service.get_research_group(research.research_group_id);

                research_api_opt = app::research_api_obj(research, {}, research_group);
                research_group_api_opt = app::research_group_api_obj(research_group);

                break;
            }

            case expertise_contribution_type::review: 
            {
                const auto& review = reviews_service.get_review(review_id_type(contribution_id));
                review_api_opt = app::review_api_obj(review, {});

                const auto& research_content = research_content_service.get_research_content(review.research_content_id);
                research_content_api_opt = app::research_content_api_obj(research_content);
                
                const auto& research = research_service.get_research(research_content.research_id);
                const auto& research_group = research_groups_service.get_research_group(research.research_group_id);

                research_api_opt = app::research_api_obj(research, {}, research_group);
                research_group_api_opt = app::research_group_api_obj(research_group);

                break;
            }

            case expertise_contribution_type::review_support: 
            {
                const auto& review_vote = review_votes_service.get_review_vote(review_vote_id_type(contribution_id));
                review_vote_api_opt = app::review_vote_api_obj(review_vote);

                const auto& review = reviews_service.get_review(review_vote.review_id);
                review_api_opt = app::review_api_obj(review, {});

                const auto& research_content = research_content_service.get_research_content(review.research_content_id);
                research_content_api_opt = app::research_content_api_obj(research_content);

                const auto& research = research_service.get_research(research_content.research_id);
                const auto& research_group = research_groups_service.get_research_group(research.research_group_id);

                research_api_opt = app::research_api_obj(research, {}, research_group);
                research_group_api_opt = app::research_group_api_obj(research_group);

                break;
            }

            default: 
            {
                break;
            }
        }
    }
};
} // namespace detail

eci_history_api::eci_history_api(const deip::app::api_context& ctx)
    : _impl(new detail::eci_history_api_impl(ctx.app))
{
}

eci_history_api::~eci_history_api()
{
}

void eci_history_api::on_api_startup() 
{
}

std::vector<research_content_eci_history_api_obj> eci_history_api::get_research_content_eci_history(const external_id_type& research_content_external_id,
                                                                                                    const research_content_eci_history_id_type& cursor,
                                                                                                    const fc::optional<external_id_type> discipline_filter,
                                                                                                    const fc::optional<fc::time_point_sec> from_filter,
                                                                                                    const fc::optional<fc::time_point_sec> to_filter,
                                                                                                    const fc::optional<uint16_t> contribution_type_filter,
                                                                                                    const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    eci_filter filter(discipline_filter, from_filter, to_filter, contribution_type_filter, assessment_criteria_type_filter);

    return db->with_read_lock([&]() { return _impl->get_research_content_eci_history(research_content_external_id, cursor, filter); });
}


fc::optional<research_content_eci_stats_api_obj> eci_history_api::get_research_content_eci_stats(const external_id_type& research_content_external_id,
                                                                                                 const fc::optional<external_id_type> discipline_filter,
                                                                                                 const fc::optional<fc::time_point_sec> from_filter,
                                                                                                 const fc::optional<fc::time_point_sec> to_filter,
                                                                                                 const fc::optional<uint16_t> contribution_type_filter,
                                                                                                 const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    eci_filter filter(discipline_filter, from_filter, to_filter, contribution_type_filter, assessment_criteria_type_filter);

    return db->with_read_lock([&]() { return _impl->get_research_content_eci_stats(research_content_external_id, filter); });
}


std::map<external_id_type, research_content_eci_stats_api_obj> eci_history_api::get_research_contents_eci_stats(const fc::optional<external_id_type> discipline_filter,
                                                                                                                const fc::optional<fc::time_point_sec> from_filter,
                                                                                                                const fc::optional<fc::time_point_sec> to_filter,
                                                                                                                const fc::optional<uint16_t> contribution_type_filter,
                                                                                                                const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    eci_filter filter(discipline_filter, from_filter, to_filter, contribution_type_filter, assessment_criteria_type_filter);

    return db->with_read_lock([&]() { return _impl->get_research_contents_eci_stats(filter); });
}


std::vector<research_eci_history_api_obj> eci_history_api::get_research_eci_history(const external_id_type& research_external_id,
                                                                                    const research_eci_history_id_type& cursor,
                                                                                    const fc::optional<external_id_type> discipline_filter,
                                                                                    const fc::optional<fc::time_point_sec> from_filter,
                                                                                    const fc::optional<fc::time_point_sec> to_filter,
                                                                                    const fc::optional<uint16_t> contribution_type_filter,
                                                                                    const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    eci_filter filter(discipline_filter, from_filter, to_filter, contribution_type_filter, assessment_criteria_type_filter);

    return db->with_read_lock([&]() { return _impl->get_research_eci_history(research_external_id, cursor, filter); });
}

fc::optional<research_eci_stats_api_obj> eci_history_api::get_research_eci_stats(const external_id_type& research_external_id,
                                                                                 const fc::optional<external_id_type> discipline_filter,
                                                                                 const fc::optional<fc::time_point_sec> from_filter,
                                                                                 const fc::optional<fc::time_point_sec> to_filter,
                                                                                 const fc::optional<uint16_t> contribution_type_filter,
                                                                                 const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    eci_filter filter(discipline_filter, from_filter, to_filter, contribution_type_filter, assessment_criteria_type_filter);

    return db->with_read_lock([&]() { return _impl->get_research_eci_stats(research_external_id, filter); });
}


std::map<external_id_type, research_eci_stats_api_obj> eci_history_api::get_researches_eci_stats(const fc::optional<external_id_type> discipline_filter,
                                                                                                 const fc::optional<fc::time_point_sec> from_filter,
                                                                                                 const fc::optional<fc::time_point_sec> to_filter,
                                                                                                 const fc::optional<uint16_t> contribution_type_filter,
                                                                                                 const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    eci_filter filter(discipline_filter, from_filter, to_filter, contribution_type_filter, assessment_criteria_type_filter);

    return db->with_read_lock([&]() { return _impl->get_researches_eci_stats(filter); });
}


std::vector<account_eci_history_api_obj> eci_history_api::get_account_eci_history(const account_name_type& account,
                                                                                  const account_eci_history_id_type& cursor,
                                                                                  const fc::optional<external_id_type> discipline_filter,
                                                                                  const fc::optional<fc::time_point_sec> from_filter,
                                                                                  const fc::optional<fc::time_point_sec> to_filter,
                                                                                  const fc::optional<uint16_t> contribution_type_filter,
                                                                                  const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    eci_filter filter(discipline_filter, from_filter, to_filter, contribution_type_filter, assessment_criteria_type_filter);

    return db->with_read_lock([&]() { return _impl->get_account_eci_history(account, cursor, filter); });
}


fc::optional<account_eci_stats_api_obj> eci_history_api::get_account_eci_stats(const account_name_type& account,                           
                                                                               const fc::optional<external_id_type> discipline_filter,
                                                                               const fc::optional<fc::time_point_sec> from_filter,
                                                                               const fc::optional<fc::time_point_sec> to_filter,
                                                                               const fc::optional<uint16_t> contribution_type_filter,
                                                                               const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    eci_filter filter(discipline_filter, from_filter, to_filter, contribution_type_filter, assessment_criteria_type_filter);

    return db->with_read_lock([&]() { return _impl->get_account_eci_stats(account, filter); });
}


std::map<account_name_type, account_eci_stats_api_obj> eci_history_api::get_accounts_eci_stats(const fc::optional<external_id_type> discipline_filter,
                                                                                               const fc::optional<fc::time_point_sec> from_filter,
                                                                                               const fc::optional<fc::time_point_sec> to_filter,
                                                                                               const fc::optional<uint16_t> contribution_type_filter,
                                                                                               const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    eci_filter filter(discipline_filter, from_filter, to_filter, contribution_type_filter, assessment_criteria_type_filter);

    return db->with_read_lock([&]() { return _impl->get_accounts_eci_stats(filter); });
}


std::vector<discipline_eci_history_api_obj> eci_history_api::get_discipline_eci_history(const fc::optional<external_id_type> discipline_filter,
                                                                                        const fc::optional<fc::time_point_sec> from_filter,
                                                                                        const fc::optional<fc::time_point_sec> to_filter,
                                                                                        const fc::optional<uint16_t> contribution_type_filter,
                                                                                        const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    eci_filter filter(discipline_filter, from_filter, to_filter, contribution_type_filter, assessment_criteria_type_filter);

    return db->with_read_lock([&]() { return _impl->get_discipline_eci_history(filter); });
}

std::map<external_id_type, std::vector<discipline_eci_stats_api_obj>> eci_history_api::get_disciplines_eci_stats_history(const fc::optional<fc::time_point_sec> from_filter,
                                                                                                                         const fc::optional<fc::time_point_sec> to_filter,
                                                                                                                         const fc::optional<uint16_t> step_filter) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { 
        return _impl->get_disciplines_eci_stats_history(
            from_filter,
            to_filter, 
            step_filter); 
        });
}

std::map<external_id_type, discipline_eci_stats_api_obj> eci_history_api::get_disciplines_eci_last_stats() const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->get_disciplines_eci_last_stats(); });
}

} // namespace research_eci_history
} // namespace deip