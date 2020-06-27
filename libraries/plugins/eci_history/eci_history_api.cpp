#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/chain/services/dbs_expertise_contribution.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/eci_history/eci_history_api.hpp>
#include <deip/eci_history/eci_history_plugin.hpp>
#include <deip/eci_history/research_eci_history_object.hpp>
#include <deip/eci_history/research_content_eci_history_object.hpp>
#include <deip/eci_history/account_eci_history_object.hpp>

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

        const auto& db = _app.chain_database();
        const auto& hist_idx = db->get_index<research_content_eci_history_index>().indices().get<by_research_content_and_discipline>();

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

        auto itr_pair = hist_idx.equal_range(std::make_tuple(research_content_id, discipline_id));
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

    std::vector<research_eci_history_api_obj> get_eci_history_by_research_and_discipline(
      const research_id_type& research_id,
      const discipline_id_type& discipline_id) const
    {
        std::vector<research_eci_history_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& hist_idx = db->get_index<research_eci_history_index>().indices().get<by_research_and_discipline>();

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

        auto itr_pair = hist_idx.equal_range(std::make_tuple(research_id, discipline_id));
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

    std::vector<account_eci_history_api_obj> get_eci_history_by_account_and_discipline(
      const account_name_type& account,
      const discipline_id_type& discipline_id) const
    {
        std::vector<account_eci_history_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& hist_idx = db->get_index<account_eci_history_index>().indices().get<by_account_and_discipline>();

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

        auto itr_pair = hist_idx.equal_range(std::make_tuple(account, discipline_id));
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