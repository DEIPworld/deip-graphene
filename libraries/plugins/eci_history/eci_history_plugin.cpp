#include <deip/protocol/config.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>
#include <deip/chain/services/dbs_expertise_contribution.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/eci_history/eci_history_api.hpp>
#include <deip/eci_history/eci_history_plugin.hpp>
#include <deip/eci_history/research_eci_history_object.hpp>
#include <deip/eci_history/research_content_eci_history_object.hpp>
#include <deip/eci_history/account_eci_history_object.hpp>

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
        switch (event_type)
        {
            case expertise_contribution_type::review: 
            {
                const auto& review = reviews_service.get_review(review_id_type(event_contribution_id));
                const auto& research = research_service.get_research(review.research_external_id);
                const auto& research_content_reviews = reviews_service.get_reviews_by_research_content(review.research_content_id);
                const auto& research_content_reviews_votes = review_votes_service.get_review_votes_by_researh_content(review.research_content_id);

                if (research.members.find(op.account) != research.members.end()) // rewarded account is research author
                {
                    contribution_type = static_cast<uint16_t>(expertise_contribution_type::publication);
                    contribution_id = review.research_content_id._id;

                    break;
                }


                const auto& review_itr = std::find_if(research_content_reviews.begin(), research_content_reviews.end(),
                  [&](const review_object& rw) { return rw.author == op.account && rw.author != review.author; });

                if (review_itr != research_content_reviews.end()) // rewarded account is other reviewer of the same research content
                {
                    const review_object& research_content_review = *review_itr;
                    contribution_type = static_cast<uint16_t>(expertise_contribution_type::review);
                    contribution_id = research_content_review.id._id;

                    break;
                }


                const auto& review_vote_itr = std::find_if(research_content_reviews_votes.begin(), research_content_reviews_votes.end(),
                  [&](const review_vote_object& vote) { return (vote.voter == op.account && vote.voter != review.author) || (vote.voter == op.account && vote.review_id != review.id); });

                if (review_vote_itr != research_content_reviews_votes.end()) // rewarded account is review voter
                {
                    const review_vote_object& research_content_review_vote = *review_vote_itr;
                    contribution_type = static_cast<uint16_t>(expertise_contribution_type::review_support);
                    contribution_id = research_content_review_vote.id._id;

                    break;
                }

                break;
            }

            case expertise_contribution_type::review_support: 
            {
                const auto& review_vote = review_votes_service.get_review_vote(review_vote_id_type(event_contribution_id));
                const auto& review = reviews_service.get_review(review_vote.review_id);
                const auto& research = research_service.get_research(review.research_external_id);
                const auto& review_votes = review_votes_service.get_review_votes(review.id);

                if (research.members.find(op.account) != research.members.end()) // rewarded account is research author
                {
                    contribution_type = static_cast<uint16_t>(expertise_contribution_type::publication);
                    contribution_id = review.research_content_id._id;

                    break;
                }

                if (review.author == op.account) // rewarded account is reviewer
                {
                    contribution_type = static_cast<uint16_t>(expertise_contribution_type::review);
                    contribution_id = review.id._id;

                    break;
                }

                break;
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