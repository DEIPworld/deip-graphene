#include <deip/research_content_reference_history/research_content_reference_history_api.hpp>
#include <deip/research_content_reference_history/research_content_reference_history_object.hpp>
#include <deip/research_content_reference_history/research_content_reference_history_plugin.hpp>

#include <deip/protocol/config.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>

#include <deip/research_content_reference_history/research_content_reference_operation_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <boost/algorithm/string.hpp>

#define DEIP_NAMESPACE_PREFIX "deip::protocol::"

namespace deip {
namespace research_content_references_history {

namespace detail {

using namespace deip::protocol;

class research_content_reference_history_plugin_impl
{
public:
    research_content_reference_history_plugin_impl(research_content_reference_history_plugin& _plugin)
        : _self(_plugin)
    {
        chain::database& db = database();

        db.add_plugin_index<research_content_reference_operation_index>();
        db.add_plugin_index<research_content_reference_operations_history_index>();

        db.pre_apply_operation.connect([&](const operation_notification& note) { on_operation(note); });
    }
    virtual ~research_content_reference_history_plugin_impl()
    {
    }

    deip::chain::database& database()
    {
        return _self.database();
    }

    const research_content_reference_operation_object& create_operation_obj(const operation_notification& note);
    void on_operation(const operation_notification& note);

    research_content_reference_history_plugin& _self;
    flat_set<string> _op_list;
};

class research_content_reference_operation_visitor
{
    database& _db;
    const research_content_reference_operation_object& _obj;
    const int64_t& _research_content_id;
    const external_id_type& _research_content_external_id;
    const int64_t& _research_id;
    const external_id_type& _research_external_id;
    const std::string& _content;
    const int64_t& _research_content_reference_id;
    const external_id_type& _research_content_reference_external_id;
    const int64_t& _research_reference_id;
    const external_id_type& _research_reference_external_id;
    const std::string& _content_reference;

public:
    using result_type = void;
    research_content_reference_operation_visitor(database& db,
                                                 const research_content_reference_operation_object& obj,
                                                 const int64_t& research_content_id,
                                                 const external_id_type& research_content_external_id,
                                                 const int64_t& research_id,
                                                 const external_id_type& research_external_id,
                                                 const std::string& content,
                                                 const int64_t& research_content_reference_id,
                                                 const external_id_type& research_content_reference_external_id,
                                                 const int64_t& research_reference_id,
                                                 const external_id_type& research_reference_external_id,
                                                 const std::string& content_reference)
        : _db(db)
        , _obj(obj)
        , _research_content_id(research_content_id)
        , _research_content_external_id(research_content_external_id)
        , _research_id(research_id)
        , _research_external_id(research_external_id)
        , _content(content)
        , _research_content_reference_id(research_content_reference_id)
        , _research_content_reference_external_id(research_content_reference_external_id)
        , _research_reference_id(research_reference_id)
        , _research_reference_external_id(research_reference_external_id)
        , _content_reference(content_reference)
    {
    }

    template <typename Op> void operator()(const Op&) const
    {
        push_history<research_content_reference_operations_history_object>(_obj);
    }

private:
    template <typename research_content_reference_history_object_type> void push_history(const research_content_reference_operation_object& op) const
    {
        _db.create<research_content_reference_history_object_type>([&](research_content_reference_history_object_type& research_content_ref_history) {
            research_content_ref_history.op = op.id;
            research_content_ref_history.research_content_id = _research_content_id;
            research_content_ref_history.research_content_external_id = _research_content_external_id;
            research_content_ref_history.research_id = _research_id;
            research_content_ref_history.research_external_id = _research_external_id;
            fc::from_string(research_content_ref_history.content, _content);
            research_content_ref_history.research_content_reference_id = _research_content_reference_id;
            research_content_ref_history.research_content_reference_external_id = _research_content_reference_external_id;
            research_content_ref_history.research_reference_id = _research_reference_id;
            research_content_ref_history.research_reference_external_id = _research_reference_external_id;
            fc::from_string(research_content_ref_history.content_reference, _content_reference);
        });
    }
};

const research_content_reference_operation_object& research_content_reference_history_plugin_impl::create_operation_obj(const operation_notification& note)
{
    deip::chain::database& db = database();
    return db.create<research_content_reference_operation_object>([&](research_content_reference_operation_object& obj) {
        obj.trx_id = note.trx_id;
        obj.block = note.block;
        obj.timestamp = db.head_block_time();
        auto size = fc::raw::pack_size(note.op);
        obj.serialized_op.resize(size);
        fc::datastream<char*> ds(obj.serialized_op.data(), size);
        fc::raw::pack(ds, note.op);
    });
}

void research_content_reference_history_plugin_impl::on_operation(const operation_notification& note)
{
    deip::chain::database& db = database();

    const research_content_reference_operation_object& new_obj = create_operation_obj(note);

    if (note.op.which() == operation::tag<research_content_reference_history_operation>::value) {

        research_content_reference_history_operation op = note.op.get<research_content_reference_history_operation>();
        note.op.visit(research_content_reference_operation_visitor(
          db, 
          new_obj, 
          op.research_content_id,
          op.research_content_external_id, 
          op.research_id, 
          op.research_external_id, 
          op.content, 
          op.research_content_reference_id, 
          op.research_content_reference_external_id, 
          op.research_reference_id, 
          op.research_reference_external_id, 
          op.content_reference)
        );
    }
}

} // end namespace detail

research_content_reference_history_plugin::research_content_reference_history_plugin(application* app)
    : plugin(app)
    , my(new detail::research_content_reference_history_plugin_impl(*this))
{
    // ilog("Loading account history plugin" );
}

research_content_reference_history_plugin::~research_content_reference_history_plugin()
{
}

std::string research_content_reference_history_plugin::plugin_name() const
{
    return CR_HISTORY_PLUGIN_NAME;
}

void research_content_reference_history_plugin::plugin_set_program_options(
    boost::program_options::options_description& cli, boost::program_options::options_description& cfg)
{

}

void research_content_reference_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
     ilog("Intializing cr history plugin");
}

void research_content_reference_history_plugin::plugin_startup()
{
    ilog("research_content_references_history plugin: plugin_startup() begin");

    app().register_api_factory<research_content_reference_history_api>("research_content_reference_history_api");

    ilog("research_content_references_history plugin: plugin_startup() end");
}

}
}

DEIP_DEFINE_PLUGIN(research_content_references_history, deip::research_content_references_history::research_content_reference_history_plugin)