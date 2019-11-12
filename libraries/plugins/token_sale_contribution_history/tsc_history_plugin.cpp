#include <deip/token_sale_contribution_history/tsc_history_api.hpp>
#include <deip/token_sale_contribution_history/tsc_history_object.hpp>
#include <deip/token_sale_contribution_history/tsc_history_plugin.hpp>

#include <deip/protocol/config.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>

#include <deip/token_sale_contribution_history/tsc_operation_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <boost/algorithm/string.hpp>

#define DEIP_NAMESPACE_PREFIX "deip::protocol::"

namespace deip {
namespace tsc_history {

namespace detail {

using namespace deip::protocol;

class tsc_history_plugin_impl
{
public:
    tsc_history_plugin_impl(tsc_history_plugin& _plugin)
        : _self(_plugin)
    {
        chain::database& db = database();

        db.add_plugin_index<tsc_operation_index>();
        db.add_plugin_index<tsc_operations_full_history_index>();
        db.add_plugin_index<contribute_to_token_sale_history_index>();

        db.pre_apply_operation.connect([&](const operation_notification& note) { on_operation(note); });
    }
    virtual ~tsc_history_plugin_impl()
    {
    }

    deip::chain::database& database()
    {
        return _self.database();
    }

    const tsc_operation_object& create_operation_obj(const operation_notification& note);
    void on_operation(const operation_notification& note);

    tsc_history_plugin& _self;
    flat_set<string> _op_list;
};

class tsc_operation_visitor
{
    database& _db;
    const tsc_operation_object& _obj;
    const account_name_type& _contributor;
    const int64_t& _research_id;
    const int64_t& _research_token_sale_id;
    const asset& _amount;

public:
    using result_type = void;
    tsc_operation_visitor(database& db, const tsc_operation_object& obj,
                                        const account_name_type& contributor,
                                        const int64_t& research_id,
                                        const int64_t& research_token_sale_id,
                                        const asset& amount)
        : _db(db)
        , _obj(obj)
        , _contributor(contributor)
        , _research_id(research_id)
        , _research_token_sale_id(research_token_sale_id)
        , _amount(amount)
    {
    }

    template <typename Op> void operator()(const Op&) const
    {
        push_history<all_tsc_operations_history_object>(_obj);
    }

    void operator()(const token_sale_contribution_to_history_operation& op) const
    {
        push_history<all_tsc_operations_history_object>(_obj);
        push_history<contribute_to_token_sale_history_object>(_obj);
    }

private:
    template <typename tsc_history_object_type> void push_history(const tsc_operation_object& op) const
    {
        _db.create<tsc_history_object_type>([&](tsc_history_object_type& tsc_hist) {
            tsc_hist.op = op.id;
            tsc_hist.contributor = _contributor;
            tsc_hist.research_id = _research_id;
            tsc_hist.research_token_sale_id = _research_token_sale_id;
            tsc_hist.amount = _amount;
        });
    }
};

const tsc_operation_object& tsc_history_plugin_impl::create_operation_obj(const operation_notification& note)
{
    deip::chain::database& db = database();
    return db.create<tsc_operation_object>([&](tsc_operation_object& obj) {
        obj.trx_id = note.trx_id;
        obj.block = note.block;
        obj.timestamp = db.head_block_time();
        auto size = fc::raw::pack_size(note.op);
        obj.serialized_op.resize(size);
        fc::datastream<char*> ds(obj.serialized_op.data(), size);
        fc::raw::pack(ds, note.op);
    });
}

void tsc_history_plugin_impl::on_operation(const operation_notification& note)
{
    deip::chain::database& db = database();

    const tsc_operation_object& new_obj = create_operation_obj(note);

    if (note.op.which() == operation::tag<token_sale_contribution_to_history_operation>::value) {

        token_sale_contribution_to_history_operation op = note.op.get<token_sale_contribution_to_history_operation>();
        account_name_type contributor = op.contributor;
        int64_t research_id = op.research_id;
        int64_t research_token_sale_id = op.research_token_sale_id;
        asset amount = op.amount;

        note.op.visit(tsc_operation_visitor(db, new_obj, contributor, research_id, research_token_sale_id, amount));
    }
}

} // end namespace detail

tsc_history_plugin::tsc_history_plugin(application* app)
    : plugin(app)
    , my(new detail::tsc_history_plugin_impl(*this))
{
    // ilog("Loading account history plugin" );
}

tsc_history_plugin::~tsc_history_plugin()
{
}

std::string tsc_history_plugin::plugin_name() const
{
    return TSC_HISTORY_PLUGIN_NAME;
}

void tsc_history_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                                    boost::program_options::options_description& cfg)
{

}

void tsc_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
     ilog("Intializing tsc history plugin");
}

void tsc_history_plugin::plugin_startup()
{
    ilog("tsc_history plugin: plugin_startup() begin");

    app().register_api_factory<tsc_history_api>("tsc_history_api");

    ilog("tsc_history plugin: plugin_startup() end");
}

}
}

DEIP_DEFINE_PLUGIN(tsc_history, deip::tsc_history::tsc_history_plugin)