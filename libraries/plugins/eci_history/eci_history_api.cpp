
#include <deip/eci_history/eci_history_api.hpp>
#include <deip/eci_history/eci_history_plugin.hpp>
#include <deip/chain/schema/expertise_contribution_object.hpp>
#include <deip/chain/schema/account_object.hpp>
#include <deip/chain/schema/research_group_object.hpp>
#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/schema/research_content_object.hpp>
#include <deip/chain/schema/review_object.hpp>
#include <deip/chain/schema/review_vote_object.hpp>
#include <deip/eci_history/research_eci_history_object.hpp>
#include <deip/eci_history/research_content_eci_history_object.hpp>
#include <deip/eci_history/account_eci_history_object.hpp>
#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <map>

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

    std::vector<research_content_eci_history_api_obj> get_eci_history_by_research_content_and_discipline(
      const research_content_id_type& research_content_id,
      const discipline_id_type& discipline_id) const
    {
        std::vector<research_content_eci_history_api_obj> result;

        const auto db = _app.chain_database();
        const auto& idx = db->get_index<research_content_eci_history_index>().indices().get<by_research_content_and_discipline>();


        const auto& research_content_idx = db->get_index<research_content_index>().indices().get<by_id>();
        auto rc_itr = research_content_idx.find(research_content_id);
        if (rc_itr == research_content_idx.end())
        {
            return result;
        }
        const auto& research_content = app::research_content_api_obj(*rc_itr);
        

        const auto& research_idx = db->get_index<research_index>().indices().get<by_id>();
        auto r_itr = research_idx.find(research_content.research_id);
        if (r_itr == research_idx.end())
        {
            return result;
        }


        const auto& research_groups_idx = db->get_index<research_group_index>().indices().get<by_id>();
        auto rg_itr = research_groups_idx.find(r_itr->research_group_id);
        if (rg_itr == research_groups_idx.end())
        {
            return result;
        }

        const auto& research_group = app::research_group_api_obj(*rg_itr);
        const auto& research = app::research_api_obj(*r_itr, {}, research_group);

        const auto& reviews_idx = db->get_index<review_index>().indices().get<by_id>();
        const auto& review_votes_idx = db->get_index<review_vote_index>().indices().get<by_id>();

        
        auto itr_pair = idx.equal_range(std::make_tuple(research_content_id, discipline_id));
        auto itr = itr_pair.first;
        const auto itr_end = itr_pair.second;
        while (itr != itr_end)
        {
            const auto& hist = *itr;
            const expertise_contribution_type contribution_type = static_cast<expertise_contribution_type>(hist.contribution_type);

            fc::optional<app::review_api_obj> review_opt;
            fc::optional<app::review_vote_api_obj> review_vote_opt;

            switch (contribution_type)
            {
                case expertise_contribution_type::publication: {

                    break;
                }

                case expertise_contribution_type::review: {

                    auto rw_itr = reviews_idx.find(hist.contribution_id);
                    if (rw_itr != reviews_idx.end())
                    {
                        review_opt = app::review_api_obj(*rw_itr, {});
                    }

                    break;
                }

                case expertise_contribution_type::review_support: {

                    auto rw_v_itr = review_votes_idx.find(hist.contribution_id);
                    if (rw_v_itr != review_votes_idx.end())
                    {
                        review_vote_opt = app::review_vote_api_obj(*rw_v_itr);
                        auto rw_itr = reviews_idx.find(review_vote_opt->review_id);
                        if (rw_itr != reviews_idx.end())
                        {
                            review_opt = app::review_api_obj(*rw_itr, {});
                        }
                    }

                    break;
                }

                default: {
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
              research_content,
              research,
              research_group,
              review_opt,
              review_vote_opt)
            );

            ++itr;
        }

        return result;
    }

    std::vector<research_eci_history_api_obj> get_eci_history_by_research_and_discipline(
      const research_id_type& research_id,
      const discipline_id_type& discipline_id) const
    {
        std::vector<research_eci_history_api_obj> result;

        const auto db = _app.chain_database();
        const auto& idx = db->get_index<research_eci_history_index>().indices().get<by_research_and_discipline>();


        const auto& research_idx = db->get_index<research_index>().indices().get<by_id>();
        auto r_itr = research_idx.find(research_id);
        if (r_itr == research_idx.end())
        {
            return result;
        }


        const auto& research_groups_idx = db->get_index<research_group_index>().indices().get<by_id>();
        auto rg_itr = research_groups_idx.find(r_itr->research_group_id);
        if (rg_itr == research_groups_idx.end())
        {
            return result;
        }

        const auto& research_group = app::research_group_api_obj(*rg_itr);
        const auto& research = app::research_api_obj(*r_itr, {}, research_group);

        const auto& research_content_idx = db->get_index<research_content_index>().indices().get<by_id>();
        const auto& reviews_idx = db->get_index<review_index>().indices().get<by_id>();
        const auto& review_votes_idx = db->get_index<review_vote_index>().indices().get<by_id>();
          

        auto itr_pair = idx.equal_range(std::make_tuple(research_id, discipline_id));
        auto itr = itr_pair.first;
        const auto itr_end = itr_pair.second;
        while (itr != itr_end)
        {
            const auto& hist = *itr;
            const expertise_contribution_type contribution_type = static_cast<expertise_contribution_type>(hist.contribution_type);

            fc::optional<app::research_content_api_obj> research_content_opt;
            fc::optional<app::review_api_obj> review_opt;
            fc::optional<app::review_vote_api_obj> review_vote_opt;

            switch (contribution_type)
            {
                case expertise_contribution_type::publication: {

                    auto rc_itr = research_content_idx.find(hist.contribution_id);
                    if (rc_itr != research_content_idx.end())
                    {
                        research_content_opt = app::research_content_api_obj(*rc_itr);
                    }

                    break;
                }

                case expertise_contribution_type::review: {

                    auto rw_itr = reviews_idx.find(hist.contribution_id);
                    if (rw_itr != reviews_idx.end())
                    {
                        review_opt = app::review_api_obj(*rw_itr, {});

                        auto rc_itr = research_content_idx.find(review_opt->research_content_id);
                        if (rc_itr != research_content_idx.end())
                        {
                            research_content_opt = app::research_content_api_obj(*rc_itr);
                        }
                    }

                    break;
                }

                case expertise_contribution_type::review_support: {

                    auto rw_v_itr = review_votes_idx.find(hist.contribution_id);
                    if (rw_v_itr != review_votes_idx.end())
                    {
                        review_vote_opt = app::review_vote_api_obj(*rw_v_itr);
                        auto rw_itr = reviews_idx.find(review_vote_opt->review_id);
                        if (rw_itr != reviews_idx.end())
                        {
                            review_opt = app::review_api_obj(*rw_itr, {});

                            auto rc_itr = research_content_idx.find(review_opt->research_content_id);
                            if (rc_itr != research_content_idx.end())
                            {
                                research_content_opt = app::research_content_api_obj(*rc_itr);
                            }
                        }
                    }

                    break;
                }

                default: {
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
              research,
              research_group,
              research_content_opt,
              review_opt,
              review_vote_opt
            ));

            ++itr;
        }

        return result;
    }

    std::vector<account_eci_history_api_obj> get_eci_history_by_account_and_discipline(
      const account_name_type& account,
      const discipline_id_type& discipline_id) const
    {
        std::vector<account_eci_history_api_obj> result;

        const auto db = _app.chain_database();
        const auto& idx = db->get_index<account_eci_history_index>().indices().get<by_account_and_discipline>();

        const auto& research_content_idx = db->get_index<research_content_index>().indices().get<by_id>();
        const auto& research_idx = db->get_index<research_index>().indices().get<by_id>();
        const auto& research_groups_idx = db->get_index<research_group_index>().indices().get<by_id>();
        const auto& reviews_idx = db->get_index<review_index>().indices().get<by_id>();
        const auto& review_votes_idx = db->get_index<review_vote_index>().indices().get<by_id>();

        auto itr_pair = idx.equal_range(std::make_tuple(account, discipline_id));
        auto itr = itr_pair.first;
        const auto itr_end = itr_pair.second;
        while (itr != itr_end)
        {
            const auto& hist = *itr;
            const expertise_contribution_type contribution_type = static_cast<expertise_contribution_type>(hist.contribution_type);

            fc::optional<app::research_content_api_obj> research_content_opt;
            fc::optional<app::research_api_obj> research_opt;
            fc::optional<app::research_group_api_obj> research_group_opt;
            fc::optional<app::review_api_obj> review_opt;
            fc::optional<app::review_vote_api_obj> review_vote_opt;

            switch (contribution_type)
            {
                case expertise_contribution_type::publication: {

                    auto rc_itr = research_content_idx.find(hist.contribution_id);
                    if (rc_itr != research_content_idx.end())
                    {
                        research_content_opt = app::research_content_api_obj(*rc_itr);

                        auto r_itr = research_idx.find(research_content_opt->research_id);
                        if (r_itr != research_idx.end())
                        {

                            auto rg_itr = research_groups_idx.find(r_itr->research_group_id);
                            if (rg_itr != research_groups_idx.end())
                            {
                                research_group_opt = app::research_group_api_obj(*rg_itr);
                                research_opt = app::research_api_obj(*r_itr, {}, *research_group_opt);
                            }
                        }
                    }

                    break;
                }

                case expertise_contribution_type::review: {

                    auto rw_itr = reviews_idx.find(hist.contribution_id);
                    if (rw_itr != reviews_idx.end())
                    {
                        review_opt = app::review_api_obj(*rw_itr, {});

                        auto rc_itr = research_content_idx.find(review_opt->research_content_id);
                        if (rc_itr != research_content_idx.end())
                        {
                            research_content_opt = app::research_content_api_obj(*rc_itr);

                            auto r_itr = research_idx.find(research_content_opt->research_id);
                            if (r_itr != research_idx.end())
                            {

                                auto rg_itr = research_groups_idx.find(r_itr->research_group_id);
                                if (rg_itr != research_groups_idx.end())
                                {
                                    research_group_opt = app::research_group_api_obj(*rg_itr);
                                    research_opt = app::research_api_obj(*r_itr, {}, *research_group_opt);
                                }
                            }
                        }
                    }

                    break;
                }

                case expertise_contribution_type::review_support: {

                    auto rw_v_itr = review_votes_idx.find(hist.contribution_id);
                    if (rw_v_itr != review_votes_idx.end())
                    {
                        review_vote_opt = app::review_vote_api_obj(*rw_v_itr);

                        auto rw_itr = reviews_idx.find(review_vote_opt->review_id);
                        if (rw_itr != reviews_idx.end())
                        {
                            review_opt = app::review_api_obj(*rw_itr, {});

                            auto rc_itr = research_content_idx.find(review_opt->research_content_id);
                            if (rc_itr != research_content_idx.end())
                            {
                                research_content_opt = app::research_content_api_obj(*rc_itr);

                                auto r_itr = research_idx.find(research_content_opt->research_id);
                                if (r_itr != research_idx.end())
                                {
                                    auto rg_itr = research_groups_idx.find(r_itr->research_group_id);
                                    if (rg_itr != research_groups_idx.end())
                                    {
                                        research_group_opt = app::research_group_api_obj(*rg_itr);
                                        research_opt = app::research_api_obj(*r_itr, {}, *research_group_opt);
                                    }
                                }
                            }
                        }
                    }

                    break;
                }

                default: {
                    break;
                }
            }

            result.push_back(account_eci_history_api_obj(
              hist.id._id, 
              hist.discipline_id._id,
              hist.account,
              hist.contribution_type,
              hist.eci,
              hist.delta,
              hist.timestamp,
              research_content_opt,
              research_opt,
              research_group_opt,
              review_opt,
              review_vote_opt
            ));

            ++itr;
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

std::vector<research_content_eci_history_api_obj> eci_history_api::get_eci_history_by_research_content_and_discipline(
  const research_content_id_type& research_content_id,
  const discipline_id_type& discipline_id) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() {
        return _impl->get_eci_history_by_research_content_and_discipline(research_content_id, discipline_id);
    });
}

std::vector<research_eci_history_api_obj> eci_history_api::get_eci_history_by_research_and_discipline(
  const research_id_type& research_id,
  const discipline_id_type& discipline_id) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { 
        return _impl->get_eci_history_by_research_and_discipline(research_id, discipline_id); 
    });
}

std::vector<account_eci_history_api_obj> eci_history_api::get_eci_history_by_account_and_discipline(
  const account_name_type& account,
  const discipline_id_type& discipline_id) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { 
        return _impl->get_eci_history_by_account_and_discipline(account, discipline_id); 
    });
}


} // namespace research_eci_history
} // namespace deip