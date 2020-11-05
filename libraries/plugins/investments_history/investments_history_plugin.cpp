#include <deip/protocol/config.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/investments_history/investments_history_api.hpp>
#include <deip/investments_history/investments_history_plugin.hpp>
#include <deip/investments_history/account_revenue_income_history_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <boost/algorithm/string.hpp>


namespace deip {
namespace investments_history {

using namespace deip::protocol;
using namespace deip::chain;

namespace detail {

class investments_history_plugin_impl
{
public:
    investments_history_plugin_impl(investments_history_plugin& _plugin)
        : _self(_plugin)
    {
    }

    virtual ~investments_history_plugin_impl()
    {
    }

    deip::chain::database& database()
    {
        return _self.database();
    }

    void pre_operation(const operation_notification& op_obj);
    void post_operation(const operation_notification& op_obj);

    investments_history_plugin& _self;
};

struct post_operation_visitor
{
    investments_history_plugin& _plugin;

    post_operation_visitor(investments_history_plugin& plugin)
        : _plugin(plugin)
    {
    }

    typedef void result_type;

    template <typename T> void operator()(const T&) const
    {
    }

    void operator()(const account_revenue_income_history_operation& op) const
    {
        _plugin.database().create<account_revenue_income_history_object>([&](account_revenue_income_history_object& hist_o) {
               hist_o.account = op.account;
               hist_o.security_token_symbol = op.security_token.symbol;
               hist_o.security_token = op.security_token;
               hist_o.revenue = op.revenue;
               hist_o.timestamp = op.timestamp;
        });
    }

};

void investments_history_plugin_impl::pre_operation(const operation_notification& note)
{
}

void investments_history_plugin_impl::post_operation(const operation_notification& note)
{
    note.op.visit(post_operation_visitor(_self));
}

} // end namespace detail

investments_history_plugin::investments_history_plugin(application* app)
    : plugin(app)
    , my(new detail::investments_history_plugin_impl(*this))
{
    ilog("Loading Investments history plugin");
}

investments_history_plugin::~investments_history_plugin()
{
}

std::string investments_history_plugin::plugin_name() const
{
    return INVESTMENTS_HISTORY_PLUGIN_NAME;
}

void investments_history_plugin::plugin_set_program_options(boost::program_options::options_description& cli, boost::program_options::options_description& cfg)
{

}

void investments_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    ilog("Intializing Investemnts history plugin");

    chain::database& db = database();

    db.add_plugin_index<account_revenue_income_history_index>();

    db.pre_apply_operation.connect([&](const operation_notification& note) { my->pre_operation(note); });
    db.post_apply_operation.connect([&](const operation_notification& note) { my->post_operation(note); });
}

void investments_history_plugin::plugin_startup()
{
    ilog("investments_history plugin: plugin_startup() begin");
    app().register_api_factory<investments_history_api>("investments_history_api");
    ilog("investments_history plugin: plugin_startup() end");
}

} // namespace investments_history
} // namespace deip

DEIP_DEFINE_PLUGIN(investments_history, deip::investments_history::investments_history_plugin)