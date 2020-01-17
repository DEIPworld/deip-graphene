#include <deip/content_references_history/cr_history_api.hpp>
#include <deip/content_references_history/cr_history_object.hpp>
#include <deip/content_references_history/cr_history_plugin.hpp>

#include <deip/protocol/config.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>

#include <deip/content_references_history/cr_operation_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <boost/algorithm/string.hpp>

#define DEIP_NAMESPACE_PREFIX "deip::protocol::"

namespace deip {
namespace cr_history {

namespace detail {

using namespace deip::protocol;

class cr_history_plugin_impl
{
public:
    cr_history_plugin_impl(cr_history_plugin& _plugin)
        : _self(_plugin)
    {
        chain::database& db = database();

        db.add_plugin_index<cr_operation_index>();
        db.add_plugin_index<cr_operations_history_index>();

        db.pre_apply_operation.connect([&](const operation_notification& note) { on_operation(note); });
    }
    virtual ~cr_history_plugin_impl()
    {
    }

    deip::chain::database& database()
    {
        return _self.database();
    }

    const cr_operation_object& create_operation_obj(const operation_notification& note);
    void on_operation(const operation_notification& note);

    cr_history_plugin& _self;
    flat_set<string> _op_list;
};

class cr_operation_visitor
{
    database& _db;
    const cr_operation_object& _obj;
    const int64_t& _research_content_id;
    const int64_t& _research_id;
    const std::string& _content;
    const int64_t& _research_content_reference_id;
    const int64_t& _research_reference_id;
    const std::string& _content_reference;

public:
    using result_type = void;
    cr_operation_visitor(database& db, const cr_operation_object& obj,
                                       const int64_t& research_content_id,
                                       const int64_t& research_id,
                                       const std::string& content,
                                       const int64_t& research_content_reference_id,
                                       const int64_t& research_reference_id,
                                       const std::string& content_reference)
        : _db(db)
        , _obj(obj)
        , _research_content_id(research_content_id)
        , _research_id(research_id)
        , _content(content)
        , _research_content_reference_id(research_content_reference_id)
        , _research_reference_id(research_reference_id)
        , _content_reference(content_reference)
    {
    }

    template <typename Op> void operator()(const Op&) const
    {
        push_history<cr_operations_history_object>(_obj);
    }

    void operator()(const token_sale_contribution_to_history_operation& op) const
    {
        push_history<cr_operations_history_object>(_obj);
    }

private:
    template <typename cr_history_object_type> void push_history(const cr_operation_object& op) const
    {
        _db.create<cr_history_object_type>([&](cr_history_object_type& cr_hist) {
            cr_hist.op = op.id;
            cr_hist.research_content_id = _research_content_id;
            cr_hist.research_id = _research_id;
            fc::from_string(cr_hist.content, _content);
            cr_hist.research_content_reference_id = _research_content_reference_id;
            cr_hist.research_reference_id = _research_reference_id;
            fc::from_string(cr_hist.content_reference, _content_reference);
        });
    }
};

const cr_operation_object& cr_history_plugin_impl::create_operation_obj(const operation_notification& note)
{
    deip::chain::database& db = database();
    return db.create<cr_operation_object>([&](cr_operation_object& obj) {
        obj.trx_id = note.trx_id;
        obj.block = note.block;
        obj.timestamp = db.head_block_time();
        auto size = fc::raw::pack_size(note.op);
        obj.serialized_op.resize(size);
        fc::datastream<char*> ds(obj.serialized_op.data(), size);
        fc::raw::pack(ds, note.op);
    });
}

void cr_history_plugin_impl::on_operation(const operation_notification& note)
{
    deip::chain::database& db = database();

    const cr_operation_object& new_obj = create_operation_obj(note);

    if (note.op.which() == operation::tag<content_reference_history_operation>::value) {

        content_reference_history_operation op = note.op.get<content_reference_history_operation>();
        int64_t research_content_id = op.research_content_id;
        int64_t research_id = op.research_id;
        std::string content = op.content;
        int64_t research_content_reference_id = op.research_content_reference_id;
        int64_t research_reference_id = op.research_reference_id;
        std::string content_reference = op.content_reference;

        note.op.visit(cr_operation_visitor(db, new_obj, research_content_id, research_id, content, research_content_reference_id, research_reference_id, content_reference));
    }
}

} // end namespace detail

cr_history_plugin::cr_history_plugin(application* app)
    : plugin(app)
    , my(new detail::cr_history_plugin_impl(*this))
{
    // ilog("Loading account history plugin" );
}

cr_history_plugin::~cr_history_plugin()
{
}

std::string cr_history_plugin::plugin_name() const
{
    return CR_HISTORY_PLUGIN_NAME;
}

void cr_history_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                                    boost::program_options::options_description& cfg)
{

}

void cr_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
     ilog("Intializing cr history plugin");
}

void cr_history_plugin::plugin_startup()
{
    ilog("cr_history plugin: plugin_startup() begin");

    app().register_api_factory<cr_history_api>("cr_history_api");

    ilog("cr_history plugin: plugin_startup() end");
}

}
}

DEIP_DEFINE_PLUGIN(cr_history, deip::cr_history::cr_history_plugin)