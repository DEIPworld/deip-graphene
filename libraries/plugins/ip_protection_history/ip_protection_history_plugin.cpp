#include <deip/ip_protection_history/ip_protection_history_api.hpp>
#include <deip/ip_protection_history/ip_protection_history_object.hpp>
#include <deip/ip_protection_history/ip_protection_history_plugin.hpp>

#include <deip/protocol/config.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>

#include <deip/chain/services/dbs_proposal_execution.hpp>

#include <deip/ip_protection_history/ip_protection_operation_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <boost/algorithm/string.hpp>

#define DEIP_NAMESPACE_PREFIX "deip::protocol::"

namespace deip {
namespace ip_protection_history {

namespace detail {

using namespace deip::protocol;

class ip_protection_history_plugin_impl
{
public:
    ip_protection_history_plugin_impl(ip_protection_history_plugin& _plugin)
        : _self(_plugin)
    {
        chain::database& db = database();

        db.add_plugin_index<ip_protection_operation_index>();
        db.add_plugin_index<ip_protection_operations_full_history_index>();
        db.add_plugin_index<create_research_material_history_index>();

        db.pre_apply_operation.connect([&](const operation_notification& note) { on_operation(note); });
    }
    virtual ~ip_protection_history_plugin_impl()
    {
    }

    deip::chain::database& database()
    {
        return _self.database();
    }

    const ip_protection_operation_object& create_operation_obj(const operation_notification& note);
    void on_operation(const operation_notification& note);

    ip_protection_history_plugin& _self;
    flat_set<string> _op_list;
};

class ip_protection_operation_visitor
{
    database& _db;
    const ip_protection_operation_object& _obj;
    const std::string& _content_hash;

public:
    using result_type = void;
    ip_protection_operation_visitor(database& db, const ip_protection_operation_object& obj, const std::string& _content_hash)
        : _db(db)
        , _obj(obj)
        , _content_hash(_content_hash)
    {
    }

    template <typename Op> void operator()(const Op&) const
    {
        push_history<all_ip_protection_operations_history_object>(_obj);
    }

    void operator()(const create_proposal_operation&) const
    {
        push_history<all_ip_protection_operations_history_object>(_obj);
        push_history<create_research_material_history_object>(_obj);
    }

private:
    template <typename ip_protection_history_object_type> void push_history(const ip_protection_operation_object& op) const
    {
        _db.create<ip_protection_history_object_type>([&](ip_protection_history_object_type& ip_protection_hist) {
            fc::from_string(ip_protection_hist.content_hash, _content_hash);
            ip_protection_hist.op = op.id;
        });
    }
};

const ip_protection_operation_object& ip_protection_history_plugin_impl::create_operation_obj(const operation_notification& note)
{
    deip::chain::database& db = database();
    return db.create<ip_protection_operation_object>([&](ip_protection_operation_object& obj) {
        obj.trx_id = note.trx_id;
        obj.block = note.block;
        obj.timestamp = db.head_block_time();
        auto size = fc::raw::pack_size(note.op);
        obj.serialized_op.resize(size);
        fc::datastream<char*> ds(obj.serialized_op.data(), size);
        fc::raw::pack(ds, note.op);
    });
}

void ip_protection_history_plugin_impl::on_operation(const operation_notification& note)
{
    deip::chain::database& db = database();

    const ip_protection_operation_object& new_obj = create_operation_obj(note);

    if (is_ip_protection_operation(note.op)){
        std::string content_hash;
        switch (note.op.which())
        {
            case operation::tag<create_proposal_operation>::value :
            {
                create_proposal_operation op = note.op.get<create_proposal_operation>();
                create_research_content_data_type data = fc::json::from_string(op.data).as<create_research_content_data_type>();
                content_hash = data.content;
            }
                break;

        }
        note.op.visit(ip_protection_operation_visitor(db, new_obj, content_hash));
    }
}

} // end namespace detail

ip_protection_history_plugin::ip_protection_history_plugin(application* app)
    : plugin(app)
    , my(new detail::ip_protection_history_plugin_impl(*this))
{
     ilog("Loading ip protection history plugin" );
}

ip_protection_history_plugin::~ip_protection_history_plugin()
{
}

std::string ip_protection_history_plugin::plugin_name() const
{
    return IP_PROTECTION_HISTORY_PLUGIN_NAME;
}

void ip_protection_history_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                                        boost::program_options::options_description& cfg)
{
//    cli.add_options()(
//        "track-account-range", boost::program_options::value<vector<string>>()->composing()->multitoken(),
//        "Defines a range of accounts to track as a json pair [\"from\",\"to\"] [from,to] Can be specified multiple "
//        "times")("history-whitelist-ops", boost::program_options::value<vector<string>>()->composing(),
//                 "Defines a list of operations which will be explicitly logged.")(
//        "history-blacklist-ops", boost::program_options::value<vector<string>>()->composing(),
//        "Defines a list of operations which will be explicitly ignored.");
//    cfg.add(cli);
}

void ip_protection_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
     ilog("Intializing ip protection history plugin");

//    typedef pair<account_name_type, account_name_type> pairstring;
//    LOAD_VALUE_SET(options, "track-account-range", my->_tracked_accounts, pairstring);
//
//    if (options.count("history-whitelist-ops"))
//    {
//        my->_filter_content = true;
//        my->_blacklist = false;
//
//        for (auto& arg : options.at("history-whitelist-ops").as<vector<string>>())
//        {
//            vector<string> ops;
//            boost::split(ops, arg, boost::is_any_of(" \t,"));
//
//            for (const string& op : ops)
//            {
//                if (op.size())
//                    my->_op_list.insert(DEIP_NAMESPACE_PREFIX + op);
//            }
//        }
//
//        ilog("Account History: whitelisting ops ${o}", ("o", my->_op_list));
//    }
//    else if (options.count("history-blacklist-ops"))
//    {
//        my->_filter_content = true;
//        my->_blacklist = true;
//        for (auto& arg : options.at("history-blacklist-ops").as<vector<string>>())
//        {
//            vector<string> ops;
//            boost::split(ops, arg, boost::is_any_of(" \t,"));
//
//            for (const string& op : ops)
//            {
//                if (op.size())
//                    my->_op_list.insert(DEIP_NAMESPACE_PREFIX + op);
//            }
//        }
//
//        ilog("Account History: blacklisting ops ${o}", ("o", my->_op_list));
//    }
}

void ip_protection_history_plugin::plugin_startup()
{
    ilog("ip_protection_history plugin: plugin_startup() begin");

    app().register_api_factory<ip_protection_history_api>("ip_protection_history_api");

    ilog("ip_protection_history plugin: plugin_startup() end");
}

}
}

DEIP_DEFINE_PLUGIN(ip_protection_history, deip::ip_protection_history::ip_protection_history_plugin)
