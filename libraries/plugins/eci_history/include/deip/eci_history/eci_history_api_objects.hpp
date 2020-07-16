#pragma once
#include <deip/app/deip_api_objects.hpp>
#include <deip/eci_history/eci_history_objects.hpp>
#include <deip/eci_history/account_eci_history_object.hpp>
#include <deip/eci_history/research_eci_history_object.hpp>
#include <deip/eci_history/research_content_eci_history_object.hpp>

namespace deip {
namespace eci_history {

struct research_content_eci_history_api_obj
{
    research_content_eci_history_api_obj(){};
    research_content_eci_history_api_obj(const share_type& eci,
                                         const share_type& delta,
                                         const research_content_eci_history_object& hist,
                                         const app::research_content_api_obj& research_content,
                                         const app::research_api_obj& research,
                                         const app::research_group_api_obj& research_group,
                                         const fc::optional<app::review_api_obj>& review_opt = {},
                                         const fc::optional<app::review_vote_api_obj>& review_vote_opt = {})
        : id(hist.id._id)
        , discipline_id(hist.discipline_id._id)
        , contribution_type(hist.contribution_type)
        , contribution_id(hist.contribution_id)
        , eci(eci)
        , delta(delta)
        , timestamp(hist.timestamp)
        , research_content(research_content)
        , research(research)
        , research_group(research_group)
    {
        for (const auto& criteria : hist.assessment_criterias)
        {
            assessment_criterias.insert(std::make_pair(criteria.first, criteria.second));
        }

        if (review_opt.valid())
        {
            review = *review_opt;
        }

        if (review_vote_opt.valid())
        {
            review_vote = *review_vote_opt;
        }
    }

    int64_t id;
    int64_t discipline_id;
    uint16_t contribution_type;
    int64_t contribution_id;

    share_type eci;
    share_type delta;

    std::map<uint16_t, assessment_criteria_value> assessment_criterias;
    fc::time_point_sec timestamp;

    app::research_content_api_obj research_content;
    app::research_api_obj research;
    app::research_group_api_obj research_group;

    fc::optional<app::review_api_obj> review;
    fc::optional<app::review_vote_api_obj> review_vote;

};

struct research_eci_history_api_obj
{
    research_eci_history_api_obj(){};
    research_eci_history_api_obj(const share_type& eci,
                                 const share_type& delta,
                                 const research_eci_history_object& hist,
                                 const app::research_api_obj& research,
                                 const app::research_group_api_obj& research_group,
                                 const fc::optional<app::research_content_api_obj>& research_content_opt = {},
                                 const fc::optional<app::review_api_obj>& review_opt = {},
                                 const fc::optional<app::review_vote_api_obj>& review_vote_opt = {})
        : id(hist.id._id)
        , discipline_id(hist.discipline_id._id)
        , contribution_type(hist.contribution_type)
        , contribution_id(hist.contribution_id)
        , eci(eci)
        , delta(delta)
        , timestamp(hist.timestamp)
        , research(research)
        , research_group(research_group)
    {
        for (const auto& criteria : hist.assessment_criterias)
        {
            assessment_criterias.insert(std::make_pair(criteria.first, criteria.second));
        }

        if (research_content_opt.valid())
        {
            research_content = *research_content_opt;
        }

        if (review_opt.valid())
        {
            review = *review_opt;
        }

        if (review_vote_opt.valid())
        {
            review_vote = *review_vote_opt;
        }
    }

    int64_t id;
    int64_t discipline_id;
    uint16_t contribution_type;
    int64_t contribution_id;

    share_type eci;
    share_type delta;

    std::map<uint16_t, assessment_criteria_value> assessment_criterias;
    fc::time_point_sec timestamp;

    app::research_api_obj research;
    app::research_group_api_obj research_group;

    fc::optional<app::research_content_api_obj> research_content;
    fc::optional<app::review_api_obj> review;
    fc::optional<app::review_vote_api_obj> review_vote;
};

struct account_eci_history_api_obj
{
    account_eci_history_api_obj(){};
    account_eci_history_api_obj(const share_type& eci,
                                const share_type& delta,
                                const account_eci_history_object& hist,
                                const fc::optional<app::research_content_api_obj>& research_content_opt = {},
                                const fc::optional<app::research_api_obj>& research_opt = {},
                                const fc::optional<app::research_group_api_obj>& research_group_opt = {},
                                const fc::optional<app::review_api_obj>& review_opt = {},
                                const fc::optional<app::review_vote_api_obj>& review_vote_opt = {})
        : id(hist.id._id)
        , discipline_id(hist.discipline_id._id)
        , account(hist.account)
        , contribution_type(hist.contribution_type)
        , contribution_id(hist.contribution_id)
        , event_contribution_type(hist.event_contribution_type)
        , event_contribution_id(hist.event_contribution_id)
        , eci(eci)
        , delta(delta)
        , timestamp(hist.timestamp)
    {
        for (const auto& criteria : hist.assessment_criterias)
        {
            assessment_criterias.insert(std::make_pair(criteria.first, criteria.second));
        }

        if (research_content_opt.valid())
        {
            research_content = *research_content_opt;
        }

        if (research_opt.valid())
        {
            research = *research_opt;
        }

        if (research_group_opt.valid())
        {
            research_group = *research_group_opt;
        }

        if (review_opt.valid())
        {
            review = *review_opt;
        }

        if (review_vote_opt.valid())
        {
            review_vote = *review_vote_opt;
        }
    }

    int64_t id;
    int64_t discipline_id;
    std::string account;

    uint16_t contribution_type;
    int64_t contribution_id;

    uint16_t event_contribution_type;
    int64_t event_contribution_id;

    share_type eci;
    share_type delta;
    std::map<uint16_t, assessment_criteria_value> assessment_criterias;
    fc::time_point_sec timestamp;

    fc::optional<app::research_content_api_obj> research_content;
    fc::optional<app::research_api_obj> research;
    fc::optional<app::research_group_api_obj> research_group;
    fc::optional<app::review_api_obj> review;
    fc::optional<app::review_vote_api_obj> review_vote;
};


struct account_eci_stats_api_obj
{
    account_eci_stats_api_obj(){};
    account_eci_stats_api_obj(const external_id_type& discipline_external_id,
                              const account_name_type& account,
                              const share_type& eci,
                              const share_type& previous_eci,
                              const share_type& starting_eci,
                              const percent& percentile_rank,
                              const fc::optional<percent>& growth_rate_opt,
                              const fc::optional<percent>& last_growth_rate_opt,
                              const std::set<std::pair<int64_t, uint16_t>>& contributions_list,
                              const std::set<external_id_type>& researches_list,
                              const fc::time_point_sec& timestamp,
                              const std::vector<account_eci_history_api_obj>& records_list)
        : discipline_external_id(discipline_external_id)
        , account(account)
        , eci(eci)
        , previous_eci(previous_eci)
        , starting_eci(starting_eci)
        , percentile_rank(percentile_rank)
        , timestamp(timestamp)
    {
        if (growth_rate_opt.valid())
        {
            growth_rate = *growth_rate_opt;
        }

        if (last_growth_rate_opt.valid())
        {
            last_growth_rate = *growth_rate_opt;
        }

        contributions.insert(contributions_list.begin(), contributions_list.end());
        researches.insert(researches_list.begin(), researches_list.end());
    }

    external_id_type discipline_external_id;
    account_name_type account;
    share_type eci;
    share_type previous_eci;
    share_type starting_eci;
    percent percentile_rank;
    fc::optional<percent> growth_rate;
    fc::optional<percent> last_growth_rate;
    std::set<std::pair<int64_t, uint16_t>> contributions;
    std::set<external_id_type> researches;
    fc::time_point_sec timestamp;
};


struct discipline_eci_history_api_obj
{
    discipline_eci_history_api_obj(){};
    discipline_eci_history_api_obj(const external_id_type& discipline_external_id,
                                   const share_type& eci,
                                   const share_type& delta,
                                   const uint16_t& contribution_type,
                                   const int64_t& contribution_id,
                                   const fc::time_point_sec& timestamp,
                                   const fc::optional<app::research_content_api_obj> research_content_opt = {},
                                   const fc::optional<app::research_api_obj> research_opt = {},
                                   const fc::optional<app::research_group_api_obj> research_group_opt = {},
                                   const fc::optional<app::review_api_obj>& review_opt = {},
                                   const fc::optional<app::review_vote_api_obj>& review_vote_opt = {})
        : discipline_external_id(discipline_external_id)
        , eci(eci)
        , delta(delta)
        , contribution_type(contribution_type)
        , contribution_id(contribution_id)
        , timestamp(timestamp)
    {

        if (research_content_opt.valid())
        {
            research_content = *research_content_opt;
        }

        if (research_opt.valid())
        {
            research = *research_opt;
        }

        if (research_group_opt.valid())
        {
            research_group = *research_group_opt;
        }

        if (review_opt.valid())
        {
            review = *review_opt;
        }

        if (review_vote_opt.valid())
        {
            review_vote = *review_vote_opt;
        }
    }

    external_id_type discipline_external_id;
    share_type eci;
    share_type delta;

    uint16_t contribution_type;
    int64_t contribution_id;

    fc::time_point_sec timestamp;

    fc::optional<app::research_content_api_obj> research_content;
    fc::optional<app::research_api_obj> research;
    fc::optional<app::research_group_api_obj> research_group;
    fc::optional<app::review_api_obj> review;
    fc::optional<app::review_vote_api_obj> review_vote;
};


struct discipline_eci_stats_api_obj
{
    discipline_eci_stats_api_obj(){};
    discipline_eci_stats_api_obj(const external_id_type& discipline_external_id,
                                 const string& discipline_name,
                                 const share_type& eci,
                                 const share_type& previous_eci,
                                 const share_type& starting_eci,
                                 const share_type& total_eci,
                                 const percent& percentage,
                                 const fc::optional<percent>& growth_rate_opt,
                                 const fc::optional<percent>& last_growth_rate_opt,
                                 const std::map<uint16_t, assessment_criteria_value> criterias,
                                 const fc::time_point_sec& timestamp)
        : discipline_external_id(discipline_external_id)
        , discipline_name(discipline_name)
        , eci(eci)
        , previous_eci(previous_eci)
        , starting_eci(starting_eci)
        , total_eci(total_eci)
        , percentage(percentage)
        , timestamp(timestamp)
    {
        if (growth_rate_opt.valid())
        {
            growth_rate = *growth_rate_opt;
        }

        if (last_growth_rate_opt.valid())
        {
            last_growth_rate = *last_growth_rate_opt;
        }

        assessment_criterias.insert(criterias.begin(), criterias.end());
    }

    external_id_type discipline_external_id;
    string discipline_name;
    share_type eci;
    share_type previous_eci;
    share_type starting_eci;
    share_type total_eci;
    percent percentage;
    fc::optional<percent> growth_rate;
    fc::optional<percent> last_growth_rate;
    std::map<uint16_t, assessment_criteria_value> assessment_criterias;
    fc::time_point_sec timestamp;
};


}
}


FC_REFLECT(deip::eci_history::research_content_eci_history_api_obj, 
  (id)
  (discipline_id)
  (contribution_type)
  (contribution_id)
  (eci)
  (delta)
  (assessment_criterias)
  (timestamp)
  (research_content)
  (research)
  (research_group)
  (review)
  (review_vote)
)

FC_REFLECT(deip::eci_history::research_eci_history_api_obj, 
  (id)
  (discipline_id)
  (contribution_type)
  (contribution_id)
  (eci)
  (delta)
  (assessment_criterias)
  (timestamp)
  (research)
  (research_group)
  (research_content)
  (review)
  (review_vote)
)

FC_REFLECT(deip::eci_history::account_eci_history_api_obj, 
  (id)
  (discipline_id)
  (account)
  (contribution_type)
  (contribution_id)
  (event_contribution_type)
  (event_contribution_id)
  (eci)
  (delta)
  (assessment_criterias)
  (timestamp)
  (research)
  (research_group)
  (research_content)
  (review)
  (review_vote)
)

FC_REFLECT(deip::eci_history::account_eci_stats_api_obj,
  (discipline_external_id)
  (account)
  (eci)
  (previous_eci)
  (starting_eci)
  (percentile_rank)
  (growth_rate)
  (last_growth_rate)
  (contributions)
  (researches)
  (timestamp)
)

FC_REFLECT(deip::eci_history::discipline_eci_history_api_obj, 
  (discipline_external_id)
  (eci)
  (delta)
  (contribution_type)
  (contribution_id)
  (timestamp)
  (research_content)
  (research)
  (research_group)
  (review)
  (review_vote)
)


FC_REFLECT(deip::eci_history::discipline_eci_stats_api_obj, 
  (discipline_external_id)
  (discipline_name)
  (eci)
  (previous_eci)
  (starting_eci)
  (total_eci)
  (percentage)
  (growth_rate)
  (last_growth_rate)
  (assessment_criterias)
  (timestamp)
)
