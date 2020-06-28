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
    account_eci_stats_api_obj(const int64_t& discipline_id,
                              const std::string& account,
                              const chain::share_type& eci,
                              const chain::percent& percentile_rank,
                              const chain::percent& growth_rate,
                              const uint16_t& contributions_count,
                              const uint16_t& projects_count,
                              const fc::time_point_sec& timestamp)
        : discipline_id(discipline_id)
        , account(account)
        , eci(eci)
        , percentile_rank(percentile_rank)
        , growth_rate(growth_rate)
        , contributions_count(contributions_count)
        , projects_count(projects_count)
        , timestamp(timestamp)
    {

    }

    int64_t discipline_id;
    std::string account;
    chain::share_type eci;
    chain::percent percentile_rank;
    chain::percent growth_rate;
    uint16_t contributions_count;
    uint16_t projects_count;

    fc::time_point_sec timestamp;
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
  (discipline_id)
  (account)
  (eci)
  (percentile_rank)
  (growth_rate)
  (contributions_count)
  (projects_count)
  (timestamp)
)
