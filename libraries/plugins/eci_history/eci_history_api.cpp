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
                                                                                       const fc::optional<external_id_type> discipline_filter,
                                                                                       const fc::optional<fc::time_point_sec> from_filter,
                                                                                       const fc::optional<fc::time_point_sec> to_filter,
                                                                                       const fc::optional<uint16_t> contribution_type_filter,
                                                                                       const fc::optional<uint16_t> assessment_criteria_type_filter) const
    {
        std::vector<research_content_eci_history_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& research_content_hist_idx = db->get_index<research_content_eci_history_index>().indices().get<by_research_content_and_cursor>();
        const auto& research_service = db->obtain_service<chain::dbs_research>();
        const auto& research_content_service = db->obtain_service<chain::dbs_research_content>();
        const auto& research_groups_service = db->obtain_service<chain::dbs_research_group>();
        const auto& reviews_service = db->obtain_service<chain::dbs_review>();
        const auto& review_votes_service = db->obtain_service<chain::dbs_review_vote>();
        const auto& disciplines_service = db->obtain_service<chain::dbs_discipline>();

        const auto& research_content_opt = research_content_service.get_research_content_if_exists(research_content_external_id);
        if (!research_content_opt.valid())
        {
            return result;
        }

        if (discipline_filter.valid() && !disciplines_service.discipline_exists(*discipline_filter))
        {
            return result;
        }

        const research_content_object& research_content = *research_content_opt;

        const discipline_object& discipline = discipline_filter.valid()
            ? disciplines_service.get_discipline(*discipline_filter)
            : disciplines_service.get_discipline(discipline_id_type(0));

        auto filter = [&](const research_content_eci_history_object& hist) -> bool {
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

            if (contribution_type_filter.valid())
            {
                const uint16_t& contribution_type = *contribution_type_filter;
                if (contribution_type != hist.contribution_type)
                {
                    return false;
                }
            }

            if (assessment_criteria_type_filter.valid())
            {
                const uint16_t& assessment_criteria_type = *assessment_criteria_type_filter;
                if (hist.assessment_criterias.find(assessment_criteria_type) == hist.assessment_criterias.end())
                {
                    return false;
                }
            }

            if (discipline_filter.valid())
            {
                if (discipline.id != hist.discipline_id && !discipline.is_common())
                {
                    return false;
                }
            }

            return true;
        };

        const auto& research = research_service.get_research(research_content.research_id);
        const auto& research_group = research_groups_service.get_research_group(research.research_group_id);

        const auto& research_group_api = app::research_group_api_obj(research_group);
        const auto& research_api = app::research_api_obj(research, {}, research_group_api);
        const auto& research_content_api = app::research_content_api_obj(research_content);

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = research_content_hist_idx.lower_bound(std::make_tuple(research_content.id, cursor)); limit-- && itr != research_content_hist_idx.end() && itr->research_content_id == research_content.id; ++itr)
        {
            const auto& hist = *itr;

            if (filter(hist))
            {
                const auto& hist = *itr;
                const expertise_contribution_type contribution_type = static_cast<expertise_contribution_type>(hist.contribution_type);

                fc::optional<app::review_api_obj> review_api_opt;
                fc::optional<app::review_vote_api_obj> review_vote_api_opt;

                switch (contribution_type)
                {
                    case expertise_contribution_type::publication: 
                    {
                        break;
                    }

                    case expertise_contribution_type::review: 
                    {
                        const auto& review = reviews_service.get_review(review_id_type(hist.contribution_id));
                        review_api_opt = app::review_api_obj(review, {});

                        break;
                    }

                    case expertise_contribution_type::review_support: 
                    {
                        const auto& review_vote = review_votes_service.get_review_vote(review_vote_id_type(hist.contribution_id));
                        review_vote_api_opt = app::review_vote_api_obj(review_vote);

                        const auto& review = reviews_service.get_review(review_vote.review_id);
                        review_api_opt = app::review_api_obj(review, {});

                        break;
                    }

                    default: 
                    {
                        break;
                    }
                }

                const auto& previous_eci = result.size() > 0 ? result[result.size() - 1].eci : hist.eci;
                const auto& delta = get_modified_eci_delta(hist.delta, hist.assessment_criterias, assessment_criteria_type_filter);
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

    std::vector<research_eci_history_api_obj> get_research_eci_history(const external_id_type& research_external_id,
                                                                       const research_eci_history_id_type& cursor,
                                                                       const fc::optional<external_id_type> discipline_filter,
                                                                       const fc::optional<fc::time_point_sec> from_filter,
                                                                       const fc::optional<fc::time_point_sec> to_filter,
                                                                       const fc::optional<uint16_t> contribution_type_filter,
                                                                       const fc::optional<uint16_t> assessment_criteria_type_filter) const
    {
        std::vector<research_eci_history_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& research_hist_idx = db->get_index<research_eci_history_index>().indices().get<by_research_and_cursor>();
        const auto& research_service = db->obtain_service<chain::dbs_research>();
        const auto& research_content_service = db->obtain_service<chain::dbs_research_content>();
        const auto& research_groups_service = db->obtain_service<chain::dbs_research_group>();
        const auto& reviews_service = db->obtain_service<chain::dbs_review>();
        const auto& review_votes_service = db->obtain_service<chain::dbs_review_vote>();
        const auto& disciplines_service = db->obtain_service<chain::dbs_discipline>();

        const auto& research_opt = research_service.get_research_if_exists(research_external_id);
        if (!research_opt.valid())
        {
            return result;
        }
        if (discipline_filter.valid() && !disciplines_service.discipline_exists(*discipline_filter))
        {
            return result;
        }

        const research_object& research = *research_opt;

        const discipline_object& discipline = discipline_filter.valid()
            ? disciplines_service.get_discipline(*discipline_filter)
            : disciplines_service.get_discipline(discipline_id_type(0));

        auto filter = [&](const research_eci_history_object& hist) -> bool {
            
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

            if (contribution_type_filter.valid())
            {
                const uint16_t& contribution_type = *contribution_type_filter;
                if (contribution_type != hist.contribution_type)
                {
                    return false;
                }
            }

            if (assessment_criteria_type_filter.valid())
            {
                const uint16_t& assessment_criteria_type = *assessment_criteria_type_filter;
                if (hist.assessment_criterias.find(assessment_criteria_type) == hist.assessment_criterias.end())
                {
                    return false;
                }
            }

            if (discipline_filter.valid())
            {
                if (discipline.id != hist.discipline_id && !discipline.is_common())
                {
                    return false;
                }
            }

            return true;
        };

        const auto& research_group = research_groups_service.get_research_group(research.research_group_id);
        const auto& research_group_api = app::research_group_api_obj(research_group);
        const auto& research_api = app::research_api_obj(research, {}, research_group);

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = research_hist_idx.lower_bound(std::make_tuple(research.id, cursor)); limit-- && itr != research_hist_idx.end() && itr->research_id == research.id; ++itr)
        {
            const auto& hist = *itr;

            if (filter(hist))
            {
                const expertise_contribution_type contribution_type = static_cast<expertise_contribution_type>(hist.contribution_type);

                fc::optional<app::research_content_api_obj> research_content_api_opt;
                fc::optional<app::review_api_obj> review_api_opt;
                fc::optional<app::review_vote_api_obj> review_vote_api_opt;

                switch (contribution_type)
                {
                    case expertise_contribution_type::publication: 
                    {
                        const auto& research_content = research_content_service.get_research_content(research_content_id_type(hist.contribution_id));
                        research_content_api_opt = app::research_content_api_obj(research_content);

                        break;
                    }

                    case expertise_contribution_type::review: 
                    {
                        const auto& review = reviews_service.get_review(review_id_type(hist.contribution_id));
                        review_api_opt = app::review_api_obj(review, {});

                        const auto& research_content = research_content_service.get_research_content(review.research_content_id);
                        research_content_api_opt = app::research_content_api_obj(research_content);

                        break;
                    }

                    case expertise_contribution_type::review_support: 
                    {
                        const auto& review_vote = review_votes_service.get_review_vote(review_vote_id_type(hist.contribution_id));
                        review_vote_api_opt = app::review_vote_api_obj(review_vote);

                        const auto& review = reviews_service.get_review(review_vote.review_id);
                        review_api_opt = app::review_api_obj(review, {});

                        const auto& research_content = research_content_service.get_research_content(review.research_content_id);
                        research_content_api_opt = app::research_content_api_obj(research_content);

                        break;
                    }

                    default: 
                    {
                        break;
                    }
                }

                const auto& previous_eci = result.size() > 0 ? result[result.size() - 1].eci : hist.eci;
                const auto& delta = get_modified_eci_delta(hist.delta, hist.assessment_criterias, assessment_criteria_type_filter);
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

    std::vector<account_eci_history_api_obj> get_account_eci_history(const account_name_type& account,
                                                                     const fc::optional<external_id_type> discipline_filter,
                                                                     const fc::optional<fc::time_point_sec> from_filter,
                                                                     const fc::optional<fc::time_point_sec> to_filter,
                                                                     const fc::optional<uint16_t> contribution_type_filter,
                                                                     const fc::optional<uint16_t> assessment_criteria_type_filter) const
    {
        std::vector<account_eci_history_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& account_hist_idx = db->get_index<account_eci_history_index>().indices().get<by_account>();
        const auto& accounts_service = db->obtain_service<chain::dbs_account>();
        const auto& research_service = db->obtain_service<chain::dbs_research>();
        const auto& research_content_service = db->obtain_service<chain::dbs_research_content>();
        const auto& research_groups_service = db->obtain_service<chain::dbs_research_group>();
        const auto& reviews_service = db->obtain_service<chain::dbs_review>();
        const auto& review_votes_service = db->obtain_service<chain::dbs_review_vote>();
        const auto& disciplines_service = db->obtain_service<chain::dbs_discipline>();

        if (!accounts_service.account_exists(account))
        {
            return result;
        }

        if (discipline_filter.valid() && !disciplines_service.discipline_exists(*discipline_filter))
        {
            return result;
        }

        const discipline_object& discipline = discipline_filter.valid()
            ? disciplines_service.get_discipline(*discipline_filter)
            : disciplines_service.get_discipline(discipline_id_type(0));

        auto filter = [&](const account_eci_history_object& hist) -> bool {

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

            if (contribution_type_filter.valid())
            {
                const uint16_t& contribution_type = *contribution_type_filter;
                if (contribution_type != hist.contribution_type)
                {
                    return false;
                }
            }

            if (assessment_criteria_type_filter.valid())
            {
                const uint16_t& assessment_criteria_type = *assessment_criteria_type_filter;
                if (hist.assessment_criterias.find(assessment_criteria_type) == hist.assessment_criterias.end())
                {
                    return false;
                }
            }

            if (discipline_filter.valid())
            {
                if (discipline.id != hist.discipline_id && !discipline.is_common())
                {
                    return false;
                }
            }

            return true;
        };

        auto itr_pair = account_hist_idx.equal_range(account);
        auto itr = itr_pair.first;
        const auto itr_end = itr_pair.second;
        while (itr != itr_end)
        {
            const auto& hist = *itr;

            if (filter(hist))
            {
                const expertise_contribution_type contribution_type = static_cast<expertise_contribution_type>(hist.contribution_type);

                fc::optional<app::research_content_api_obj> research_content_api_opt;
                fc::optional<app::research_api_obj> research_api_opt;
                fc::optional<app::research_group_api_obj> research_group_api_opt;
                fc::optional<app::review_api_obj> review_api_opt;
                fc::optional<app::review_vote_api_obj> review_vote_api_opt;

                switch (contribution_type)
                {
                    case expertise_contribution_type::publication: 
                    {
                        const auto& research_content = research_content_service.get_research_content(research_content_id_type(hist.contribution_id));
                        research_content_api_opt = app::research_content_api_obj(research_content);

                        const auto& research = research_service.get_research(research_content.research_id);
                        const auto& research_group = research_groups_service.get_research_group(research.research_group_id);

                        research_api_opt = app::research_api_obj(research, {}, research_group);
                        research_group_api_opt = app::research_group_api_obj(research_group);

                        break;
                    }

                    case expertise_contribution_type::review: 
                    {
                        const auto& review = reviews_service.get_review(review_id_type(hist.contribution_id));
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
                        const auto& review_vote = review_votes_service.get_review_vote(review_vote_id_type(hist.contribution_id));
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

                const auto& previous_eci = result.size() > 0 ? result[result.size() - 1].eci : hist.eci;
                const auto& delta = get_modified_eci_delta(hist.delta, hist.assessment_criterias, assessment_criteria_type_filter);
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

            ++itr;
        }

        return result;
    }

    std::map<account_name_type, account_eci_stats_api_obj>get_accounts_eci_stats(const fc::optional<external_id_type> discipline_filter,
                                                                                 const fc::optional<fc::time_point_sec> from_filter,
                                                                                 const fc::optional<fc::time_point_sec> to_filter,
                                                                                 const fc::optional<uint16_t> contribution_type_filter,
                                                                                 const fc::optional<uint16_t> assessment_criteria_type_filter) const
    {
        const auto& db = _app.chain_database();
        const auto& account_hist_idx = db->get_index<account_eci_history_index>().indices().get<by_account>();
        const auto& accounts_service = db->obtain_service<chain::dbs_account>();
        const auto& expertise_tokens_service = db->obtain_service<chain::dbs_expert_token>();
        const auto& disciplines_service = db->obtain_service<chain::dbs_discipline>();

        std::map<account_name_type, account_eci_stats_api_obj> result;

        if (discipline_filter.valid() && !disciplines_service.discipline_exists(*discipline_filter))
        {
            return result;
        }

        const discipline_object& discipline = discipline_filter.valid() 
          ? disciplines_service.get_discipline(*discipline_filter) 
          : disciplines_service.get_discipline(discipline_id_type(0));

        const auto& accounts = accounts_service.lookup_user_accounts(account_name_type("a"), DEIP_API_BULK_FETCH_LIMIT);

        auto filter = [&](const account_eci_history_object& hist) -> bool {
            
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

            if (contribution_type_filter.valid())
            {
                const uint16_t& contribution_type = *contribution_type_filter;
                if (contribution_type != hist.contribution_type)
                {
                    return false;
                }
            }

            if (assessment_criteria_type_filter.valid())
            {
                const uint16_t& assessment_criteria_type = *assessment_criteria_type_filter;
                if (hist.assessment_criterias.find(assessment_criteria_type) == hist.assessment_criterias.end())
                {
                    return false;
                }
            }

            if (discipline_filter.valid())
            {
                if (discipline.id != hist.discipline_id && !discipline.is_common())
                {
                    return false;
                }
            }

            return true;
        };

        std::vector<share_type> eci_scores;

        for (const account_object& acc : accounts)
        {
            auto itr_pair = account_hist_idx.equal_range(acc.name);
            auto& itr = itr_pair.first;
            const auto& itr_end = itr_pair.second;

            while (itr != itr_end)
            {
                const auto& hist = *itr;

                if (filter(hist))
                {
                    if (result.find(acc.name) == result.end())
                    {
                        auto stats = account_eci_stats_api_obj();
                        stats.account = acc.name;
                        stats.discipline_external_id = discipline.external_id;
                        result.insert(std::make_pair(acc.name, stats));
                    }

                    auto& stats = result[acc.name];

                    stats.past_eci = stats.eci;
                    const auto& delta = get_modified_eci_delta(hist.delta, hist.assessment_criterias, assessment_criteria_type_filter);
                    stats.eci += delta;
                    stats.timestamp = hist.timestamp;
                    stats.researches.insert(hist.researches.begin(), hist.researches.end());
                    stats.contributions.insert(std::make_pair(hist.contribution_id, hist.contribution_type));
                }

                ++itr;
            }

            if (result.find(acc.name) != result.end())
            {
                const auto& stats = result[acc.name];
                eci_scores.push_back(stats.eci);
            }
        }
        
        for (auto& pair : result)
        {
            auto& stats = pair.second;

            /*
                Percentile Rank = [(M + (0.5 * R)) / Y] * 100
                M = Number of Ranks below x
                R = Number of Ranks equals x
                Y = Total Number of Ranks
            */
            const share_type x_score = stats.eci;
            std::sort(eci_scores.begin(), eci_scores.end());
            const auto& x_score_itr = std::find(eci_scores.begin(), eci_scores.end(), x_score);
            const int M = std::distance(eci_scores.begin(), x_score_itr);
            const int R = std::count_if(eci_scores.begin(), eci_scores.end(), [&](const share_type& eci_score) { 
              return eci_score == x_score;
            });
            const int Y = eci_scores.size();
            const share_type percentile_rank = share_type(std::round(((double(M) + (double(0.5) * double(R))) / double(Y)) * double(100) * DEIP_1_PERCENT));
            stats.percentile_rank = percent(percentile_rank);


            /*
                Growth Rate = [(Vpresent - Vpast) / Vpast] * 100
                Vpresent = present value
                VPast = past value
            */
            const share_type present_eci_score = x_score;
            const share_type past_eci_score = stats.past_eci;

            if (past_eci_score != share_type(0))
            {
                const share_type growth = share_type( std::round( ((double(present_eci_score.value) - double(past_eci_score.value)) / double(past_eci_score.value)) * double(100) * DEIP_1_PERCENT ));
                stats.growth_rate = percent(growth);
            }
        }

        return result;
    }

std::vector<discipline_eci_history_api_obj> get_discipline_eci_history(const fc::optional<external_id_type> discipline_filter,
                                                                       const fc::optional<fc::time_point_sec> from_filter,
                                                                       const fc::optional<fc::time_point_sec> to_filter,
                                                                       const fc::optional<uint16_t> contribution_type_filter,
                                                                       const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    std::vector<discipline_eci_history_api_obj> result;

    const auto& db = _app.chain_database();
    const auto& discipline_hist_idx = db->get_index<discipline_eci_history_index>().indices().get<by_id>();
    const auto& research_service = db->obtain_service<chain::dbs_research>();
    const auto& research_content_service = db->obtain_service<chain::dbs_research_content>();
    const auto& research_groups_service = db->obtain_service<chain::dbs_research_group>();
    const auto& reviews_service = db->obtain_service<chain::dbs_review>();
    const auto& review_votes_service = db->obtain_service<chain::dbs_review_vote>();
    const auto& disciplines_service = db->obtain_service<chain::dbs_discipline>();


    if (discipline_filter.valid() && !disciplines_service.discipline_exists(*discipline_filter))
    {
        return result;
    }

    const discipline_object& discipline = discipline_filter.valid()
        ? disciplines_service.get_discipline(*discipline_filter)
        : disciplines_service.get_discipline(discipline_id_type(0));

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

        if (discipline_filter.valid())
        {
            if (discipline.id != hist.discipline_id && !discipline.is_common())
            {
                return false;
            }
        }

        return true;
    };

    uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;

    for (auto itr = discipline_hist_idx.lower_bound(discipline_eci_history_id_type(0)); limit-- && itr != discipline_hist_idx.end(); ++itr)
    {
        const discipline_eci_history_object& hist = *itr;

        if (filter(hist))
        {
            for (const auto& diff : hist.contributions)
            {

                if (assessment_criteria_type_filter.valid())
                {
                    const uint16_t& assessment_criteria_type = *assessment_criteria_type_filter;
                    if (diff.assessment_criterias.find(assessment_criteria_type) == diff.assessment_criterias.end())
                    {
                        continue;
                    }
                }

                if (contribution_type_filter.valid())
                {
                    const uint16_t& contribution_type = *contribution_type_filter;
                    if (contribution_type != diff.contribution_type)
                    {
                        continue;
                    }
                }

                const expertise_contribution_type contribution_type = static_cast<expertise_contribution_type>(diff.contribution_type);

                fc::optional<app::research_content_api_obj> research_content_api_opt;
                fc::optional<app::research_api_obj> research_api_opt;
                fc::optional<app::research_group_api_obj> research_group_api_opt;
                fc::optional<app::review_api_obj> review_api_opt;
                fc::optional<app::review_vote_api_obj> review_vote_api_opt;

                switch (contribution_type)
                {
                    case expertise_contribution_type::publication: 
                    {
                        const auto& research_content = research_content_service.get_research_content(research_content_id_type(diff.contribution_id));
                        research_content_api_opt = app::research_content_api_obj(research_content);

                        const auto& research = research_service.get_research(research_content.research_id);
                        const auto& research_group = research_groups_service.get_research_group(research.research_group_id);

                        research_api_opt = app::research_api_obj(research, {}, research_group);
                        research_group_api_opt = app::research_group_api_obj(research_group);

                        break;
                    }

                    case expertise_contribution_type::review: 
                    {
                        const auto& review = reviews_service.get_review(review_id_type(diff.contribution_id));
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
                        const auto& review_vote = review_votes_service.get_review_vote(review_vote_id_type(diff.contribution_id));
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

                const auto& previous_eci = result.size() > 0 ? result[result.size() - 1].eci : hist.eci;
                const auto& delta = get_modified_eci_delta(diff.diff(), diff.assessment_criterias, assessment_criteria_type_filter);
                const auto& eci = previous_eci + delta;

                result.push_back(discipline_eci_history_api_obj(
                  discipline.external_id,
                  eci,
                  delta,
                  diff.contribution_type,
                  diff.contribution_id,
                  hist.timestamp,
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
                    std::map<uint16_t, assessment_criteria_value> assessment_criterias;
                    for (uint16_t i = static_cast<uint16_t>(assessment_criteria::FIRST); i <= static_cast<uint16_t>(assessment_criteria::LAST); i++)
                    {
                        if (hist.assessment_criterias.find(i) != hist.assessment_criterias.end())
                        {
                            assessment_criterias.insert(std::make_pair(i, hist.assessment_criterias.at(i)));
                        }
                        else
                        {
                            assessment_criterias.insert(std::make_pair(i, assessment_criteria_value(0)));
                        }
                    }

                    fc::optional<percent> growth_rate;
                    records.push_back(discipline_eci_stats_api_obj(
                        discipline.external_id,
                        fc::to_string(discipline.name),
                        hist.eci,
                        hist.total_eci,
                        hist.percentage,
                        growth_rate,
                        assessment_criterias,
                        hist.timestamp
                    ));
                }

                ++itr;
            }

            result.insert(std::make_pair(discipline.external_id, records));
        }

        for (auto& result : result)
        {
            auto& records = result.second;
            for (auto i = 0; i < records.size(); i++)
            {
                if (i == 0) continue;

                const auto& past = records[i - 1];
                auto& present = records[i];

                /*
                    Growth Rate = [(Vpresent - Vpast) / Vpast] * 100
                    Vpresent = present value
                    VPast = past value
                */
                if (past.eci != share_type(0))
                {
                    const share_type growth = share_type( std::round( ((double(present.eci.value) - double(past.eci.value)) / double(past.eci.value)) * double(100) * DEIP_1_PERCENT ));
                    present.growth_rate = percent(growth);
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
            fc::optional<percent> growth_rate;

            auto itr_pair = discipline_hist_idx.equal_range(discipline.id);
            if (itr_pair.first == itr_pair.second) 
            {
                continue;
            }

            const discipline_eci_history_object& hist = *(--itr_pair.second);
            if (itr_pair.second != itr_pair.first) 
            {
                const discipline_eci_history_object& past = *(--itr_pair.second);

                /*
                    Growth Rate = [(Vpresent - Vpast) / Vpast] * 100
                    Vpresent = present value
                    VPast = past value
                */
                if (past.eci != share_type(0))
                {
                    const share_type growth = share_type( std::round( ((double(hist.eci.value) - double(past.eci.value)) / double(past.eci.value)) * double(100) * DEIP_1_PERCENT ));
                    growth_rate = percent(growth);
                }
            }

            std::map<uint16_t, assessment_criteria_value> assessment_criterias;
            for (uint16_t i = static_cast<uint16_t>(assessment_criteria::FIRST); i <= static_cast<uint16_t>(assessment_criteria::LAST); i++)
            {
                if (hist.assessment_criterias.find(i) != hist.assessment_criterias.end())
                {
                    assessment_criterias.insert(std::make_pair(i, hist.assessment_criterias.at(i)));
                }
                else
                {
                    assessment_criterias.insert(std::make_pair(i, assessment_criteria_value(0)));
                }
            }

            result.insert(std::make_pair(discipline.external_id, discipline_eci_stats_api_obj(
                discipline.external_id,
                fc::to_string(discipline.name),
                hist.eci,
                hist.total_eci,
                hist.percentage,
                growth_rate,
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
    return db->with_read_lock([&]() {
        return _impl->get_research_content_eci_history(
            research_content_external_id, 
            cursor, 
            discipline_filter, 
            from_filter, 
            to_filter, 
            contribution_type_filter,
            assessment_criteria_type_filter
        );
    });
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
    return db->with_read_lock([&]() {
        return _impl->get_research_eci_history(
            research_external_id, 
            cursor, 
            discipline_filter, 
            from_filter, 
            to_filter,
            contribution_type_filter, 
            assessment_criteria_type_filter
        );
    });
}

std::vector<account_eci_history_api_obj> eci_history_api::get_account_eci_history(const account_name_type& account,
                                                                                                    const fc::optional<external_id_type> discipline_filter,
                                                                                                    const fc::optional<fc::time_point_sec> from_filter,
                                                                                                    const fc::optional<fc::time_point_sec> to_filter,
                                                                                                    const fc::optional<uint16_t> contribution_type_filter,
                                                                                                    const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock(
        [&]() { return _impl->get_account_eci_history(account, 
                                                      discipline_filter, 
                                                      from_filter, 
                                                      to_filter, 
                                                      contribution_type_filter, 
                                                      assessment_criteria_type_filter);
        });
}

std::map<account_name_type, account_eci_stats_api_obj> eci_history_api::get_accounts_eci_stats(const fc::optional<external_id_type> discipline_filter,
                                                                                               const fc::optional<fc::time_point_sec> from_filter,
                                                                                               const fc::optional<fc::time_point_sec> to_filter,
                                                                                               const fc::optional<uint16_t> contribution_type_filter,
                                                                                               const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->get_accounts_eci_stats(discipline_filter, from_filter, to_filter, contribution_type_filter, assessment_criteria_type_filter); });
}



std::vector<discipline_eci_history_api_obj> eci_history_api::get_discipline_eci_history(const fc::optional<external_id_type> discipline_filter,
                                                                                        const fc::optional<fc::time_point_sec> from_filter,
                                                                                        const fc::optional<fc::time_point_sec> to_filter,
                                                                                        const fc::optional<uint16_t> contribution_type_filter,
                                                                                        const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->get_discipline_eci_history(discipline_filter, from_filter, to_filter, contribution_type_filter, assessment_criteria_type_filter); });
}

std::map<external_id_type, std::vector<discipline_eci_stats_api_obj>> eci_history_api::get_disciplines_eci_stats_history(const fc::optional<fc::time_point_sec> from_filter,
                                                                                                                         const fc::optional<fc::time_point_sec> to_filter,
                                                                                                                         const fc::optional<uint16_t> step_filter) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->get_disciplines_eci_stats_history(from_filter, to_filter, step_filter); });
}

std::map<external_id_type, discipline_eci_stats_api_obj> eci_history_api::get_disciplines_eci_last_stats() const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->get_disciplines_eci_last_stats(); });
}

} // namespace research_eci_history
} // namespace deip