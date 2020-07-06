#include <deip/protocol/config.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/services/dbs_expertise_contribution.hpp>
#include <deip/eci_history/eci_history_api.hpp>
#include <deip/eci_history/eci_history_plugin.hpp>
#include <deip/eci_history/research_eci_history_object.hpp>
#include <deip/eci_history/research_content_eci_history_object.hpp>
#include <deip/eci_history/account_eci_history_object.hpp>
#include <deip/eci_history/discipline_eci_history_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <boost/algorithm/string.hpp>

    namespace deip {
namespace eci_history {

using namespace deip::protocol;
using namespace deip::chain;

namespace detail {

class eci_history_plugin_impl
{
public:
    eci_history_plugin_impl(eci_history_plugin& _plugin)
        : _self(_plugin)
    {
    }

    virtual ~eci_history_plugin_impl()
    {
    }

    deip::chain::database& database()
    {
        return _self.database();
    }

    void pre_operation(const operation_notification& op_obj);
    void post_operation(const operation_notification& op_obj);

    eci_history_plugin& _self;
};

struct post_operation_visitor
{
    eci_history_plugin& _plugin;

    post_operation_visitor(eci_history_plugin& plugin)
        : _plugin(plugin)
    {
    }

    typedef void result_type;

    template <typename T> void operator()(const T&) const
    {
    }

    void operator()(const research_content_eci_history_operation& op) const
    {
        _plugin.database().create<research_content_eci_history_object>([&](research_content_eci_history_object& hist_o) {
            hist_o.research_content_id = op.research_content_id;
            hist_o.discipline_id = op.discipline_id;
            hist_o.eci = op.diff.current();
            hist_o.delta = op.diff.diff();
            hist_o.contribution_type = op.diff.contribution_type;
            hist_o.contribution_id = op.diff.contribution_id;
            hist_o.timestamp = op.diff.timestamp;

            for (const auto& criteria : op.diff.assessment_criterias)
            {
                hist_o.assessment_criterias.insert(std::make_pair(criteria.first, criteria.second));
            }
        });
    }

    void operator()(const research_eci_history_operation& op) const
    {
        _plugin.database().create<research_eci_history_object>([&](research_eci_history_object& hist_o) {
            hist_o.research_id = op.research_id;
            hist_o.discipline_id = op.discipline_id;
            hist_o.eci = op.diff.current();
            hist_o.delta = op.diff.diff();
            hist_o.contribution_type = op.diff.contribution_type;
            hist_o.contribution_id = op.diff.contribution_id;
            hist_o.timestamp = op.diff.timestamp;

            for (const auto& criteria : op.diff.assessment_criterias)
            {
                hist_o.assessment_criterias.insert(std::make_pair(criteria.first, criteria.second));
            }
        });
    }

    void operator()(const account_eci_history_operation& op) const
    {
        const auto& research_service = _plugin.database().obtain_service<chain::dbs_research>();
        const auto& reviews_service = _plugin.database().obtain_service<chain::dbs_review>();
        const auto& review_votes_service = _plugin.database().obtain_service<chain::dbs_review_vote>();

        const uint16_t event_contribution_type = op.diff.contribution_type;
        const uint16_t event_contribution_id = op.diff.contribution_id;

        uint16_t contribution_type = event_contribution_type;
        int64_t contribution_id = event_contribution_id;

        const auto event_type = static_cast<expertise_contribution_type>(event_contribution_type);
        const auto recipient_type = static_cast<reward_recipient_type>(op.recipient_type);

        switch (recipient_type)
        {
            case reward_recipient_type::author: 
            {
                switch (event_type)
                {
                    case expertise_contribution_type::publication: 
                    {
                        break; // rewarded account is author of research content
                    }

                    case expertise_contribution_type::review: // rewarded account is author of research content
                    {
                        const auto& review = reviews_service.get_review(review_id_type(event_contribution_id));
                        
                        contribution_type = static_cast<uint16_t>(expertise_contribution_type::publication);
                        contribution_id = review.research_content_id._id;
                    
                        break;
                    }

                    case expertise_contribution_type::review_support: // rewarded account is author of research content
                    {
                        const auto& review_vote = review_votes_service.get_review_vote(review_vote_id_type(event_contribution_id));
                        const auto& review = reviews_service.get_review(review_vote.review_id);

                        contribution_type = static_cast<uint16_t>(expertise_contribution_type::publication);
                        contribution_id = review.research_content_id._id;

                        break;
                    }

                    default: 
                    {
                        break;
                    }
                }

                break;
            }
            case reward_recipient_type::reviewer: 
            {
                switch (event_type)
                {
                    case expertise_contribution_type::review:
                    {
                        const auto& review = reviews_service.get_review(review_id_type(event_contribution_id));

                        if (review.author == op.account) // rewarded account is review author
                        {
                            break;
                        }

                        const auto& research_content_reviews = reviews_service.get_reviews_by_research_content(review.research_content_id);
                        const auto& review_itr = std::find_if(research_content_reviews.begin(), research_content_reviews.end(),
                          [&](const review_object& rw) { return rw.author == op.account && rw.id != review.id; });

                        if (review_itr != research_content_reviews.end()) // rewarded account is author of another review for the same research content
                        {
                            const review_object& research_content_review = *review_itr;
                            contribution_type = static_cast<uint16_t>(expertise_contribution_type::review);
                            contribution_id = research_content_review.id._id;

                            break;
                        }
                    
                        break;
                    }

                    case expertise_contribution_type::review_support:
                    {
                        const auto& review_vote = review_votes_service.get_review_vote(review_vote_id_type(event_contribution_id));
                        const auto& review = reviews_service.get_review(review_vote.review_id);

                        if (review.author == op.account) // rewarded account is author of review that has been supported
                        {
                            contribution_type = static_cast<uint16_t>(expertise_contribution_type::review);
                            contribution_id = review.id._id;

                            break;
                        }


                        const auto& research_content_reviews = reviews_service.get_reviews_by_research_content(review.research_content_id);
                        const auto& review_itr = std::find_if(research_content_reviews.begin(), research_content_reviews.end(),
                          [&](const review_object& rw) { return rw.author == op.account && rw.id != review.id; });

                        if (review_itr != research_content_reviews.end()) // rewarded account is author of another review for the same research content
                        {
                            const review_object& research_content_review = *review_itr;
                            contribution_type = static_cast<uint16_t>(expertise_contribution_type::review);
                            contribution_id = research_content_review.id._id;

                            break;
                        }
                        
                        break;
                    }

                    default: 
                    {
                        break;
                    }
                }

                break;
            }

            case reward_recipient_type::review_supporter: 
            {
                switch (event_type)
                {
                    case expertise_contribution_type::review:
                    {
                        const auto& review = reviews_service.get_review(review_id_type(event_contribution_id));
                        const auto& research_content_reviews_votes = review_votes_service.get_review_votes_by_researh_content(review.research_content_id);

                        const auto& review_vote_itr = std::find_if(research_content_reviews_votes.begin(), research_content_reviews_votes.end(),
                            [&](const review_vote_object& vote) { return vote.voter == op.account; });

                        if (review_vote_itr != research_content_reviews_votes.end()) // rewarded account is review supporter of any review within the same research content
                        {
                            const review_vote_object& research_content_review_vote = *review_vote_itr;
                            contribution_type = static_cast<uint16_t>(expertise_contribution_type::review_support);
                            contribution_id = research_content_review_vote.id._id; // bug, as votes can be left for multiple reviews within the same research content

                            break;
                        }

                        break;
                    }

                    case expertise_contribution_type::review_support: 
                    {
                        break; // rewarded account is review supporter
                    }

                    default: 
                    {
                        break;
                    }
                }
            }
            default:
            {
                break;
            }
        }

        const auto& researches = research_service.get_researches_by_member(op.account);

        _plugin.database().create<account_eci_history_object>([&](account_eci_history_object& hist_o) {
            hist_o.account = op.account;
            hist_o.discipline_id = op.discipline_id;
            hist_o.eci = op.diff.current();
            hist_o.delta = op.diff.diff();
            hist_o.contribution_type = contribution_type;
            hist_o.contribution_id = contribution_id;
            hist_o.event_contribution_type = event_contribution_type;
            hist_o.event_contribution_id = event_contribution_id;

            hist_o.timestamp = op.diff.timestamp;

            for (const auto& criteria : op.diff.assessment_criterias)
            {
                hist_o.assessment_criterias.insert(std::make_pair(criteria.first, criteria.second));
            }

            for (const research_object& research : researches)
            {
                hist_o.researches.insert(research.external_id);
            }
        });
    }

    void operator()(const disciplines_eci_history_operation& op) const
    {
        const auto& disciplines_service = _plugin.database().obtain_service<chain::dbs_discipline>();
        const auto& expert_tokens_service = _plugin.database().obtain_service<chain::dbs_expert_token>();
        const auto& expertise_contribution_service = _plugin.database().obtain_service<chain::dbs_expertise_contribution>();

        const auto& disciplines = disciplines_service.lookup_disciplines(discipline_id_type(1), DEIP_API_BULK_FETCH_LIMIT);

        std::map<discipline_eci_history_id_type, share_type> disciplines_stats;
        share_type total_expertise = share_type(0);

        if (!op.is_initial)
        {
            for (const discipline_object& discipline : disciplines)
            {
                const auto& expertise_contributions = expertise_contribution_service.get_expertise_contributions_by_discipline(discipline.id);
                share_type total_eci = share_type(0);
                flat_map<uint16_t, assessment_criteria_value> total_assessment_criterias;

                for (const expertise_contribution_object& exp_contribution : expertise_contributions)
                {
                    total_eci += exp_contribution.eci;
                    for (const auto& criteria : exp_contribution.assessment_criterias)
                    {
                        if (total_assessment_criterias.find(criteria.first) == total_assessment_criterias.end())
                        {
                            total_assessment_criterias.insert(std::make_pair(criteria.first, criteria.second));
                        }
                        else
                        {
                            total_assessment_criterias.at(criteria.first) += criteria.second;
                        }
                    }
                }

                const auto& hist = _plugin.database().create<discipline_eci_history_object>([&](discipline_eci_history_object& hist_o) {
                    hist_o.discipline_id = discipline.id;
                    hist_o.eci = total_eci;
                    hist_o.timestamp = op.timestamp;

                    for (const auto& criteria : total_assessment_criterias)
                    {
                        hist_o.assessment_criterias.insert(std::make_pair(criteria.first, criteria.second));
                    }
                });

                disciplines_stats.insert(std::make_pair(hist.id, total_eci));
                total_expertise += total_eci;
            }
        }
        else
        {
            for (const discipline_object& discipline : disciplines)
            {
                const auto& hist = _plugin.database().create<discipline_eci_history_object>([&](discipline_eci_history_object& hist_o) {
                    hist_o.discipline_id = discipline.id;
                    hist_o.eci = share_type(0);
                    hist_o.timestamp = op.timestamp;
                });

                disciplines_stats.insert(std::make_pair(hist.id, share_type(0)));
            }
        }

        for (const auto& stat : disciplines_stats)
        {
            const auto& expertise = stat.second;
            const auto& share = total_expertise.value != 0
                ? share_type(std::round( (((double(expertise.value) / double(total_expertise.value)) * double(100)) * DEIP_1_PERCENT) ))
                : share_type(0);

            const auto& hist = _plugin.database().get<discipline_eci_history_object, by_id>(stat.first);

            _plugin.database().modify(hist, [&](discipline_eci_history_object& hist_o) {
                hist_o.share = percent(share);
                hist_o.total_eci = total_expertise;
            });
        }
    }
};

void eci_history_plugin_impl::pre_operation(const operation_notification& note)
{
}

void eci_history_plugin_impl::post_operation(const operation_notification& note)
{
    note.op.visit(post_operation_visitor(_self));
}

} // end namespace detail

eci_history_plugin::eci_history_plugin(application* app)
    : plugin(app)
    , my(new detail::eci_history_plugin_impl(*this))
{
    ilog("Loading ECI history plugin");
}

eci_history_plugin::~eci_history_plugin()
{
}

std::string eci_history_plugin::plugin_name() const
{
    return ECI_HISTORY_PLUGIN_NAME;
}

void eci_history_plugin::plugin_set_program_options(boost::program_options::options_description& cli, boost::program_options::options_description& cfg)
{

}

void eci_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    ilog("Intializing ECI history plugin");

    chain::database& db = database();

    db.add_plugin_index<account_eci_history_index>();
    db.add_plugin_index<research_eci_history_index>();
    db.add_plugin_index<research_content_eci_history_index>();
    db.add_plugin_index<discipline_eci_history_index>();

    db.pre_apply_operation.connect([&](const operation_notification& note) { my->pre_operation(note); });
    db.post_apply_operation.connect([&](const operation_notification& note) { my->post_operation(note); });
}

void eci_history_plugin::plugin_startup()
{
    ilog("eci_history plugin: plugin_startup() begin");
    app().register_api_factory<eci_history_api>("eci_history_api");
    ilog("eci_history plugin: plugin_startup() end");
}

} // namespace eci_history
} // namespace deip

DEIP_DEFINE_PLUGIN(eci_history, deip::eci_history::eci_history_plugin)