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

namespace deip {
namespace eci_history {

using deip::chain::expertise_contribution_type;

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

    std::vector<research_content_eci_history_api_obj> get_eci_history_by_research_content_and_discipline(const research_content_id_type& research_content_id,
                                                                                                         const discipline_id_type& discipline_id) const
    {
        std::vector<research_content_eci_history_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& research_content_hist_idx = db->get_index<research_content_eci_history_index>().indices().get<by_research_content_and_discipline>();
        const auto& research_service = db->obtain_service<chain::dbs_research>();
        const auto& research_content_service = db->obtain_service<chain::dbs_research_content>();
        const auto& research_groups_service = db->obtain_service<chain::dbs_research_group>();
        const auto& reviews_service = db->obtain_service<chain::dbs_review>();
        const auto& review_votes_service = db->obtain_service<chain::dbs_review_vote>();

        const auto& research_content_opt = research_content_service.get_research_content_if_exists(research_content_id);
        if (!research_content_opt.valid())
        {
            return result;
        }

        const auto& research_content = (*research_content_opt).get();
        const auto& research = research_service.get_research(research_content.research_id);
        const auto& research_group = research_groups_service.get_research_group(research.research_group_id);

        const auto& research_group_api = app::research_group_api_obj(research_group);
        const auto& research_api = app::research_api_obj(research, {}, research_group_api);
        const auto& research_content_api = app::research_content_api_obj(research_content);

        auto itr_pair = research_content_hist_idx.equal_range(std::make_tuple(research_content_id, discipline_id));
        auto itr = itr_pair.first;
        const auto itr_end = itr_pair.second;
        while (itr != itr_end)
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

            result.push_back(research_content_eci_history_api_obj(
              hist.id._id, 
              hist.discipline_id._id,
              hist.contribution_type,
              hist.eci,
              hist.delta,
              hist.timestamp,
              research_content_api,
              research_api,
              research_group_api,
              review_api_opt,
              review_vote_api_opt)
            );

            ++itr;
        }

        return result;
    }

    std::vector<research_eci_history_api_obj> get_eci_history_by_research_and_discipline(const research_id_type& research_id,
                                                                                         const discipline_id_type& discipline_id) const
    {
        std::vector<research_eci_history_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& research_hist_idx = db->get_index<research_eci_history_index>().indices().get<by_research_and_discipline>();
        const auto& research_service = db->obtain_service<chain::dbs_research>();
        const auto& research_content_service = db->obtain_service<chain::dbs_research_content>();
        const auto& research_groups_service = db->obtain_service<chain::dbs_research_group>();
        const auto& reviews_service = db->obtain_service<chain::dbs_review>();
        const auto& review_votes_service = db->obtain_service<chain::dbs_review_vote>();

        const auto& research_opt = research_service.get_research_if_exists(research_id);
        if (!research_opt.valid())
        {
            return result;
        }

        const auto& research = (*research_opt).get();
        const auto& research_group = research_groups_service.get_research_group(research.research_group_id);

        const auto& research_group_api = app::research_group_api_obj(research_group);
        const auto& research_api = app::research_api_obj(research, {}, research_group);

        auto itr_pair = research_hist_idx.equal_range(std::make_tuple(research_id, discipline_id));
        auto itr = itr_pair.first;
        const auto itr_end = itr_pair.second;
        while (itr != itr_end)
        {
            const auto& hist = *itr;
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

            result.push_back(research_eci_history_api_obj(
              hist.id._id, 
              hist.discipline_id._id,
              hist.contribution_type,
              hist.eci,
              hist.delta,
              hist.timestamp,
              research_api,
              research_group_api,
              research_content_api_opt,
              review_api_opt,
              review_vote_api_opt
            ));

            ++itr;
        }

        return result;
    }

    std::vector<account_eci_history_api_obj> get_eci_history_by_account_and_discipline(const account_name_type& account,
                                                                                       const discipline_id_type& discipline_id) const
    {
        std::vector<account_eci_history_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& account_hist_idx = db->get_index<account_eci_history_index>().indices().get<by_account_and_discipline>();
        const auto& accounts_service = db->obtain_service<chain::dbs_account>();
        const auto& research_service = db->obtain_service<chain::dbs_research>();
        const auto& research_content_service = db->obtain_service<chain::dbs_research_content>();
        const auto& research_groups_service = db->obtain_service<chain::dbs_research_group>();
        const auto& reviews_service = db->obtain_service<chain::dbs_review>();
        const auto& review_votes_service = db->obtain_service<chain::dbs_review_vote>();

        const auto& account_opt = accounts_service.get_account_if_exists(account);
        if (!account_opt.valid())
        {
            return result;
        }

        auto itr_pair = account_hist_idx.equal_range(std::make_tuple(account, discipline_id));
        auto itr = itr_pair.first;
        const auto itr_end = itr_pair.second;
        while (itr != itr_end)
        {
            const auto& hist = *itr;
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

            result.push_back(account_eci_history_api_obj(
              hist.id._id, 
              hist.discipline_id._id,
              hist.account,
              hist.contribution_type,
              hist.contribution_id,
              hist.event_contribution_type,
              hist.event_contribution_id,
              hist.eci,
              hist.delta,
              hist.timestamp,
              research_content_api_opt,
              research_api_opt,
              research_group_api_opt,
              review_api_opt,
              review_vote_api_opt
            ));

            ++itr;
        }

        return result;
    }

    std::map<account_name_type, account_eci_stats_api_obj>get_accounts_eci_stats(const fc::optional<external_id_type> discipline_filter,
                                                                                 const fc::optional<uint16_t> contribution_type_filter,
                                                                                 const fc::optional<uint16_t> assessment_criteria_type_filter) const
    {
        const auto& db = _app.chain_database();
        const auto& account_hist_idx = db->get_index<account_eci_history_index>().indices().get<by_account_and_discipline>();
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

          return true;
        };

        std::vector<chain::share_type> eci_scores;

        for (const account_object& acc : accounts)
        {
            if (discipline.id != discipline_id_type(0))
            {
                if (!expertise_tokens_service.expert_token_exists_by_account_and_discipline(acc.name, discipline.id))
                {
                    continue;
                }

                auto itr_pair = account_hist_idx.equal_range(std::make_tuple(acc.name, discipline.id));
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

                        stats.eci += hist.delta;
                        stats.timestamp = hist.timestamp;
                        stats.researches.insert(hist.researches.begin(), hist.researches.end());
                        stats.contributions.insert(std::make_pair(hist.contribution_id, hist.contribution_type));

                        if (assessment_criteria_type_filter.valid())
                        {
                            const uint16_t& criteria_type = *assessment_criteria_type_filter;
                            stats.assessment_criteria_sum_weight += share_type(hist.assessment_criterias.at(criteria_type));
                        }
                        else
                        {
                            for (const auto& assessment_criteria : hist.assessment_criterias)
                            {
                                stats.assessment_criteria_sum_weight += share_type(assessment_criteria.second);
                            }
                        }
                    }

                    ++itr;
                }

                if (result.find(acc.name) != result.end())
                {
                    const auto& stats = result[acc.name];
                    share_type eci_score = assessment_criteria_type_filter.valid()
                        ? (stats.eci + stats.assessment_criteria_sum_weight)
                        : stats.eci;

                    eci_scores.push_back(eci_score);
                }
            } 
            else
            {
                for (const account_object& acc : accounts)
                {
                    const auto& expertise_tokens = expertise_tokens_service.get_expert_tokens_by_account_name(acc.name);

                    for (const expert_token_object& expert_token : expertise_tokens)
                    {
                        auto itr_pair = account_hist_idx.equal_range(std::make_tuple(acc.name, expert_token.discipline_id));
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

                                stats.eci += hist.delta;
                                if (stats.timestamp < hist.timestamp)
                                {
                                    stats.timestamp = hist.timestamp;
                                }
                                stats.researches.insert(hist.researches.begin(), hist.researches.end());
                                stats.contributions.insert(std::make_pair(hist.contribution_id, hist.contribution_type));

                                if (assessment_criteria_type_filter.valid())
                                {
                                    const uint16_t& criteria_type = *assessment_criteria_type_filter;
                                    stats.assessment_criteria_sum_weight += share_type(hist.assessment_criterias.at(criteria_type));
                                }
                                else 
                                {
                                    for (const auto& assessment_criteria : hist.assessment_criterias)
                                    {
                                        stats.assessment_criteria_sum_weight += share_type(assessment_criteria.second);
                                    }
                                }
                            }

                            ++itr;
                        }
                    }

                    if (result.find(acc.name) != result.end())
                    {
                        const auto& stats = result[acc.name];
                        share_type eci_score = assessment_criteria_type_filter.valid()
                            ? (stats.eci + stats.assessment_criteria_sum_weight)
                            : stats.eci;

                        eci_scores.push_back(eci_score);
                    }
                }
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
            const share_type x = assessment_criteria_type_filter.valid() ? (stats.eci + stats.assessment_criteria_sum_weight) : stats.eci;
            std::sort(eci_scores.begin(), eci_scores.end());
            const auto& x_itr = std::find(eci_scores.begin(), eci_scores.end(), x);
            const int M = std::distance(eci_scores.begin(), x_itr);
            const int R = std::count_if(eci_scores.begin(), eci_scores.end(), [&](const chain::share_type& eci_score) { 
              if (assessment_criteria_type_filter.valid())
              {
                  const share_type x_score = stats.eci + stats.assessment_criteria_sum_weight;
                  return eci_score == x_score;
              } 
              else 
              {
                  return eci_score == stats.eci;
              }
            });
            const int Y = eci_scores.size();
            const chain::share_type percentile_rank = share_type(std::round(((double(M) + (double(0.5) * double(R))) / double(Y)) * double(100) * DEIP_1_PERCENT));
            stats.percentile_rank = percent(percentile_rank);

            const chain::share_type growth_rate = share_type(0);
            stats.growth_rate = percent(DEIP_1_PERCENT * growth_rate);
        }

        return result;
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

std::vector<research_content_eci_history_api_obj> eci_history_api::get_eci_history_by_research_content_and_discipline(const research_content_id_type& research_content_id,
                                                                                                                      const discipline_id_type& discipline_id) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        return _impl->get_eci_history_by_research_content_and_discipline(research_content_id, discipline_id);
    });
}

std::vector<research_eci_history_api_obj> eci_history_api::get_eci_history_by_research_and_discipline(const research_id_type& research_id,
                                                                                                      const discipline_id_type& discipline_id) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { 
        return _impl->get_eci_history_by_research_and_discipline(research_id, discipline_id); 
    });
}

std::vector<account_eci_history_api_obj> eci_history_api::get_eci_history_by_account_and_discipline(const account_name_type& account,
                                                                                                    const discipline_id_type& discipline_id) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { 
        return _impl->get_eci_history_by_account_and_discipline(account, discipline_id); 
    });
}

std::map<account_name_type, account_eci_stats_api_obj> eci_history_api::get_accounts_eci_stats(const fc::optional<external_id_type> discipline_filter,
                                                                                               const fc::optional<uint16_t> contribution_type_filter,
                                                                                               const fc::optional<uint16_t> assessment_criteria_type_filter) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock(
        [&]() { return _impl->get_accounts_eci_stats(discipline_filter, contribution_type_filter, assessment_criteria_type_filter); });
}


} // namespace research_eci_history
} // namespace deip