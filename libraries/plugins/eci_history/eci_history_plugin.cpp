#include <deip/protocol/config.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>

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
        chain::database& db = database();

        db.add_plugin_index<account_eci_history_index>();
        db.add_plugin_index<research_eci_history_index>();
        db.add_plugin_index<research_content_eci_history_index>();
        db.pre_apply_operation.connect([&](const operation_notification& note) { on_operation(note); });
    }
    virtual ~eci_history_plugin_impl()
    {
    }

    deip::chain::database& database()
    {
        return _self.database();
    }

    void on_operation(const operation_notification& note);

    eci_history_plugin& _self;
};


void eci_history_plugin_impl::on_operation(const operation_notification& note)
{
    deip::chain::database& db = database();

    if (note.op.which() == operation::tag<research_content_eci_history_operation>::value)
    {
        research_content_eci_history_operation op = note.op.get<research_content_eci_history_operation>();
        db.create<research_content_eci_history_object>([&](research_content_eci_history_object& hist_o) {
            hist_o.research_content_id = op.research_content_id;
            hist_o.discipline_id = op.discipline_id;
            hist_o.eci = op.diff.current();
            hist_o.delta = op.diff.diff();
            hist_o.alteration_source_type = op.diff.alteration_source_type;
            hist_o.alteration_source_id = op.diff.alteration_source_id;
            hist_o.timestamp = op.diff.timestamp;
        });
    }

    else if (note.op.which() == operation::tag<research_eci_history_operation>::value)
    {
        research_eci_history_operation op = note.op.get<research_eci_history_operation>();
        db.create<research_eci_history_object>([&](research_eci_history_object& hist_o) {
            hist_o.research_id = op.research_id;
            hist_o.discipline_id = op.discipline_id;
            hist_o.eci = op.diff.current();
            hist_o.delta = op.diff.diff();
            hist_o.alteration_source_type = op.diff.alteration_source_type;
            hist_o.alteration_source_id = op.diff.alteration_source_id;
            hist_o.timestamp = op.diff.timestamp;
        });
    }

    else if (note.op.which() == operation::tag<account_eci_history_operation>::value)
    {
        account_eci_history_operation op = note.op.get<account_eci_history_operation>();
        db.create<account_eci_history_object>([&](account_eci_history_object& hist_o) {
            hist_o.account = op.account;
            hist_o.discipline_id = op.discipline_id;
            hist_o.eci = op.diff.current();
            hist_o.delta = op.diff.diff();
            hist_o.alteration_source_type = op.diff.alteration_source_type;
            hist_o.alteration_source_id = op.diff.alteration_source_id;
            hist_o.timestamp = op.diff.timestamp;
        });
    }
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