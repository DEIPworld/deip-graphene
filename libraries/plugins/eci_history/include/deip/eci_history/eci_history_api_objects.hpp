#pragma once
#include <deip/app/deip_api_objects.hpp>

namespace deip {
namespace eci_history {

struct research_content_eci_history_api_obj
{
    research_content_eci_history_api_obj(){};
    research_content_eci_history_api_obj(const int64_t& id,
                                         const int64_t& discipline_id,
                                         const uint16_t contribution_type,
                                         const chain::share_type& eci,
                                         const chain::share_type& delta,
                                         const fc::time_point_sec& timestamp,
                                         const app::research_content_api_obj research_content,
                                         const app::research_api_obj research,
                                         const app::research_group_api_obj research_group,
                                         const fc::optional<app::review_api_obj>& review_opt,
                                         const fc::optional<app::review_vote_api_obj>& review_vote_opt)
        : id(id)
        , discipline_id(discipline_id)
        , contribution_type(contribution_type)
        , eci(eci)
        , delta(delta)
        , timestamp(timestamp)
        , research_content(research_content)
        , research(research)
        , research_group(research_group)
    {
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

    chain::share_type eci;
    chain::share_type delta;

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
    research_eci_history_api_obj(const int64_t& id,
                                 const int64_t& discipline_id,
                                 const uint16_t contribution_type,
                                 const chain::share_type& eci,
                                 const chain::share_type& delta,
                                 const fc::time_point_sec& timestamp,
                                 const app::research_api_obj research,
                                 const app::research_group_api_obj research_group,
                                 const fc::optional<app::research_content_api_obj> research_content_opt,
                                 const fc::optional<app::review_api_obj>& review_opt,
                                 const fc::optional<app::review_vote_api_obj>& review_vote_opt)
        : id(id)
        , discipline_id(discipline_id)
        , contribution_type(contribution_type)
        , eci(eci)
        , delta(delta)
        , timestamp(timestamp)
        , research(research)
        , research_group(research_group)
    {

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

    chain::share_type eci;
    chain::share_type delta;

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
    account_eci_history_api_obj(const int64_t& id,
                                const int64_t& discipline_id,
                                const std::string& account,
                                const uint16_t contribution_type,
                                const int64_t contribution_id,
                                const uint16_t event_contribution_type,
                                const int64_t event_contribution_id,
                                const chain::share_type& eci,
                                const chain::share_type& delta,
                                const fc::time_point_sec& timestamp,
                                const fc::optional<app::research_content_api_obj> research_content_opt,
                                const fc::optional<app::research_api_obj> research_opt,
                                const fc::optional<app::research_group_api_obj> research_group_opt,
                                const fc::optional<app::review_api_obj>& review_opt,
                                const fc::optional<app::review_vote_api_obj>& review_vote_opt)
        : id(id)
        , discipline_id(discipline_id)
        , account(account)
        , contribution_type(contribution_type)
        , contribution_id(contribution_id)
        , event_contribution_type(event_contribution_type)
        , event_contribution_id(event_contribution_id)
        , eci(eci)
        , delta(delta)
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

    int64_t id;
    int64_t discipline_id;
    std::string account;

    uint16_t contribution_type;
    uint16_t contribution_id;

    uint16_t event_contribution_type;
    int64_t event_contribution_id;

    chain::share_type eci;
    chain::share_type delta;

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
                              const share_type& past_eci,
                              const percent& percentile_rank,
                              const fc::optional<percent>& growth_rate_opt,
                              const share_type& assessment_criteria_sum_weight,
                              const share_type& past_assessment_criteria_sum_weight,
                              const std::set<std::pair<int64_t, uint16_t>>& contributions_list,
                              const std::set<external_id_type>& researches_list,
                              const fc::time_point_sec& timestamp)
        : discipline_external_id(discipline_external_id)
        , account(account)
        , eci(eci)
        , past_eci(past_eci)
        , percentile_rank(percentile_rank)
        , assessment_criteria_sum_weight(assessment_criteria_sum_weight)
        , past_assessment_criteria_sum_weight(past_assessment_criteria_sum_weight)
        , timestamp(timestamp)
    {
        if (growth_rate_opt.valid())
        {
            growth_rate = *growth_rate_opt;
        }

        contributions.insert(contributions_list.begin(), contributions_list.end());
        researches.insert(researches_list.begin(), researches_list.end());
    }

    external_id_type discipline_external_id;
    account_name_type account;
    share_type eci = 0;
    share_type past_eci = 0;
    percent percentile_rank;
    fc::optional<percent> growth_rate;
    share_type assessment_criteria_sum_weight = 0;
    share_type past_assessment_criteria_sum_weight = 0;
    std::set<std::pair<int64_t, uint16_t>> contributions;
    std::set<external_id_type> researches;
    fc::time_point_sec timestamp;
};


struct discipline_eci_stats_api_obj
{
    discipline_eci_stats_api_obj(){};
    discipline_eci_stats_api_obj(const external_id_type& discipline_external_id,
                                 const string& discipline_name,
                                 const share_type& eci,
                                 const share_type& total_eci,
                                 const percent& share,
                                 std::map<uint16_t, uint16_t> criterias,
                                 const fc::time_point_sec& timestamp)
        : discipline_external_id(discipline_external_id)
        , discipline_name(discipline_name)
        , eci(eci)
        , total_eci(total_eci)
        , share(share)
        , timestamp(timestamp)
    {
        assessment_criterias.insert(criterias.begin(), criterias.end());
    }

    external_id_type discipline_external_id;
    string discipline_name;
    share_type eci = share_type(0);
    share_type total_eci = share_type(0);
    percent share;
    fc::time_point_sec timestamp;
    std::map<uint16_t, uint16_t> assessment_criterias;
};


}
}


FC_REFLECT(deip::eci_history::research_content_eci_history_api_obj, 
  (id)
  (discipline_id)
  (contribution_type)
  (eci)
  (delta)
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
  (eci)
  (delta)
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
  (past_eci)
  (percentile_rank)
  (growth_rate)
  (assessment_criteria_sum_weight)
  (past_assessment_criteria_sum_weight)
  (contributions)
  (researches)
  (timestamp)
)


FC_REFLECT(deip::eci_history::discipline_eci_stats_api_obj, 
  (discipline_external_id)
  (discipline_name)
  (eci)
  (total_eci)
  (share)
  (timestamp)
  (assessment_criterias)
)
