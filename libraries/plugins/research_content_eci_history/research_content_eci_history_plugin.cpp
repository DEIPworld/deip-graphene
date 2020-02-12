#include <deip/research_content_eci_history/research_content_eci_history_api.hpp>
#include <deip/research_content_eci_history/research_content_eci_history_object.hpp>
#include <deip/research_content_eci_history/research_content_eci_history_plugin.hpp>

#include <deip/protocol/config.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>

#include <deip/research_content_eci_history/research_content_eci_operation_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <boost/algorithm/string.hpp>

#define DEIP_NAMESPACE_PREFIX "deip::protocol::"

namespace deip {
namespace research_content_eci_history {

namespace detail {

using namespace deip::protocol;

class research_content_eci_history_plugin_impl
{
public:
    research_content_eci_history_plugin_impl(research_content_eci_history_plugin& _plugin)
        : _self(_plugin)
    {
        chain::database& db = database();

        db.add_plugin_index<research_content_eci_operation_index>();
        db.add_plugin_index<research_content_eci_operations_history_index>();

        db.pre_apply_operation.connect([&](const operation_notification& note) { on_operation(note); });
    }
    virtual ~research_content_eci_history_plugin_impl()
    {
    }

    deip::chain::database& database()
    {
        return _self.database();
    }

    const research_content_eci_operation_object& create_operation_obj(const operation_notification& note);
    void on_operation(const operation_notification& note);

    research_content_eci_history_plugin& _self;
    flat_set<string> _op_list;
};

class research_content_eci_operation_visitor
{
    database& _db;
    const research_content_eci_operation_object& _obj;

    const int64_t& _research_content_id;
    const int64_t& _discipline_id;
    const share_type& _new_eci_amount;
    const share_type& _delta;
    const uint16_t& _action;
    const int64_t& _action_object_id;
    const uint32_t& _timestamp;


public:
    using result_type = void;
    research_content_eci_operation_visitor(database& db, const research_content_eci_operation_object& obj,
                                           const int64_t& research_content_id,
                                           const int64_t& discipline_id,
                                           const share_type& new_eci_amount,
                                           const share_type& delta,
                                           const uint16_t& action,
                                           const int64_t& action_object_id,
                                           const uint32_t& timestamp)
        : _db(db)
        , _obj(obj)
        , _research_content_id(research_content_id)
        , _discipline_id(discipline_id)
        , _new_eci_amount(new_eci_amount)
        , _delta(delta)
        , _action(action)
        , _action_object_id(action_object_id)
        , _timestamp(timestamp)
    {
    }

    template <typename Op> void operator()(const Op&) const
    {
        push_history<research_content_eci_operations_history_object>(_obj);
    }

    void operator()(const research_content_eci_history_operation& op) const
    {
        push_history<research_content_eci_operations_history_object>(_obj);
    }

private:
    template <typename research_content_eci_history_object_type> void push_history(const research_content_eci_operation_object& op) const
    {
        _db.create<research_content_eci_history_object_type>([&](research_content_eci_history_object_type& re_hist) {
            re_hist.op = op.id;
            re_hist.research_content_id = _research_content_id;
            re_hist.discipline_id = _discipline_id;
            re_hist.new_eci_amount = _new_eci_amount;
            re_hist.delta = _delta;
            re_hist.action = _action;
            re_hist.action_object_id = _action_object_id;
            re_hist.timestamp = _timestamp;
        });
    }
};

const research_content_eci_operation_object& research_content_eci_history_plugin_impl::create_operation_obj(const operation_notification& note)
{
    deip::chain::database& db = database();
    return db.create<research_content_eci_operation_object>([&](research_content_eci_operation_object& obj) {
        obj.trx_id = note.trx_id;
        obj.block = note.block;
        obj.timestamp = db.head_block_time();
        auto size = fc::raw::pack_size(note.op);
        obj.serialized_op.resize(size);
        fc::datastream<char*> ds(obj.serialized_op.data(), size);
        fc::raw::pack(ds, note.op);
    });
}

void research_content_eci_history_plugin_impl::on_operation(const operation_notification& note)
{
    deip::chain::database& db = database();

    const research_content_eci_operation_object& new_obj = create_operation_obj(note);

    if (note.op.which() == operation::tag<research_content_eci_history_operation>::value)
    {

        research_content_eci_history_operation op = note.op.get<research_content_eci_history_operation>();
        int64_t research_content_id = op.research_content_id;
        int64_t discipline_id = op.discipline_id;
        share_type new_eci_amount = op.new_eci_amount;
        share_type delta = op.delta;
        uint16_t action = op.action;
        int64_t action_object_id = op.action_object_id;
        uint32_t timestamp = op.timestamp;

        note.op.visit(research_content_eci_operation_visitor(db, new_obj, research_content_id, discipline_id, new_eci_amount, delta, action, action_object_id, timestamp));
    }
}

} // end namespace detail

research_content_eci_history_plugin::research_content_eci_history_plugin(application* app)
    : plugin(app)
    , my(new detail::research_content_eci_history_plugin_impl(*this))
{
     ilog("Loading research content eci plugin" );
}

research_content_eci_history_plugin::~research_content_eci_history_plugin()
{
}

std::string research_content_eci_history_plugin::plugin_name() const
{
    return RC_ECI_HISTORY_PLUGIN_NAME;
}

void research_content_eci_history_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                                   boost::program_options::options_description& cfg)
{

}

void research_content_eci_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
     ilog("Intializing research_content_eci history plugin");
}

void research_content_eci_history_plugin::plugin_startup()
{
    ilog("research_content_eci_history plugin: plugin_startup() begin");

    app().register_api_factory<research_content_eci_history_api>("research_content_eci_history_api");

    ilog("research_content_eci_history plugin: plugin_startup() end");
}

}
}

DEIP_DEFINE_PLUGIN(research_content_eci_history, deip::research_content_eci_history::research_content_eci_history_plugin)