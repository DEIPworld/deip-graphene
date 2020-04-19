#include <deip/blockchain_statistics/blockchain_statistics_api.hpp>

#include <deip/app/impacted.hpp>
#include <deip/chain/schema/account_object.hpp>

#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>

namespace deip {
namespace blockchain_statistics {

namespace detail {

using namespace deip::protocol;

class blockchain_statistics_plugin_impl
{
public:
    blockchain_statistics_plugin_impl(blockchain_statistics_plugin& plugin)
        : _self(plugin)
    {
    }
    virtual ~blockchain_statistics_plugin_impl()
    {
    }

    void on_block(const signed_block& b);
    void pre_operation(const operation_notification& o);
    void post_operation(const operation_notification& o);

    blockchain_statistics_plugin& _self;
    flat_set<uint32_t> _tracked_buckets = { 60, 3600, 21600, 86400, 604800, 2592000 };
    flat_set<bucket_id_type> _current_buckets;
    uint32_t _maximum_history_per_bucket_size = 100;
};

struct operation_process
{
    const blockchain_statistics_plugin& _plugin;
    const bucket_object& _bucket;
    chain::database& _db;

    operation_process(blockchain_statistics_plugin& bsp, const bucket_object& b)
        : _plugin(bsp)
        , _bucket(b)
        , _db(bsp.database())
    {
    }

    typedef void result_type;

    template <typename T> void operator()(const T&) const
    {
    }

    void operator()(const transfer_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) {
            b.transfers++;

            if (op.amount.symbol == DEIP_SYMBOL)
                b.deip_transferred += op.amount.amount;
            else
                b.sbd_transferred += op.amount.amount;
        });
    }

    void operator()(const create_account_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) { b.paid_accounts_created++; });
    }

//    void operator()(const vote_operation& op) const
//    {
////        _db.modify(_bucket, [&](bucket_object& b) {
////            const auto& cv_idx = _db.get_index<comment_vote_index>().indices().get<by_comment_voter>();
////            const auto& comment = _db.get_comment(op.author, op.permlink);
////            const auto& voter = _db.get_account(op.voter);
////            const auto itr = cv_idx.find(boost::make_tuple(comment.id, voter.id));
////
////            if (itr->num_changes)
////            {
////                if (comment.parent_author.size())
////                    b.new_reply_votes++;
////                else
////                    b.new_root_votes++;
////            }
////            else
////            {
////                if (comment.parent_author.size())
////                    b.changed_reply_votes++;
////                else
////                    b.changed_root_votes++;
////            }
////        });
//    }

    void operator()(const transfer_to_common_tokens_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) {
            b.transfers_to_common_tokens++;
            b.deip_to_common_tokens += op.amount.amount;
        });
    }

    void operator()(const fill_common_tokens_withdraw_operation& op) const
    {
        const auto& account = _db.get_account(op.from_account);

        _db.modify(_bucket, [&](bucket_object& b) {
            b.common_tokens_withdrawals_processed++;
            if (op.transfer)
                b.common_tokens_transferred += op.withdrawn;
            else
                b.common_tokens_withdrawn += op.withdrawn;

            if (account.common_tokens_withdraw_rate == 0)
                b.finished_common_tokens_withdrawals++;
        });
    }

};

void blockchain_statistics_plugin_impl::on_block(const signed_block& b)
{
    auto& db = _self.database();

    if (b.block_num() == 1)
    {
        db.create<bucket_object>([&](bucket_object& bo) {
            bo.open = b.timestamp;
            bo.seconds = 0;
            bo.blocks = 1;
        });
    }
    else
    {
        db.modify(db.get(bucket_id_type()), [&](bucket_object& bo) { bo.blocks++; });
    }

    _current_buckets.clear();
    _current_buckets.insert(bucket_id_type());

    const auto& bucket_idx = db.get_index<bucket_index>().indices().get<by_bucket>();

    uint32_t trx_size = 0;
    uint32_t num_trx = b.transactions.size();

    for (auto trx : b.transactions)
    {
        trx_size += fc::raw::pack_size(trx);
    }

    for (auto bucket : _tracked_buckets)
    {
        auto open = fc::time_point_sec((db.head_block_time().sec_since_epoch() / bucket) * bucket);
        auto itr = bucket_idx.find(boost::make_tuple(bucket, open));

        if (itr == bucket_idx.end())
        {
            _current_buckets.insert(db.create<bucket_object>([&](bucket_object& bo) {
                                          bo.open = open;
                                          bo.seconds = bucket;
                                          bo.blocks = 1;
                                      }).id);

            if (_maximum_history_per_bucket_size > 0)
            {
                try
                {
                    auto cutoff = fc::time_point_sec(
                        (safe<uint32_t>(db.head_block_time().sec_since_epoch())
                         - safe<uint32_t>(bucket) * safe<uint32_t>(_maximum_history_per_bucket_size))
                            .value);

                    itr = bucket_idx.lower_bound(boost::make_tuple(bucket, fc::time_point_sec()));

                    while (itr->seconds == bucket && itr->open < cutoff)
                    {
                        auto old_itr = itr;
                        ++itr;
                        db.remove(*old_itr);
                    }
                }
                catch (fc::overflow_exception& e)
                {
                }
                catch (fc::underflow_exception& e)
                {
                }
            }
        }
        else
        {
            db.modify(*itr, [&](bucket_object& bo) { bo.blocks++; });

            _current_buckets.insert(itr->id);
        }

        db.modify(*itr, [&](bucket_object& bo) {
            bo.transactions += num_trx;
            bo.bandwidth += trx_size;
        });
    }
}

void blockchain_statistics_plugin_impl::pre_operation(const operation_notification& o)
{
    auto& db = _self.database();

    for (auto bucket_id : _current_buckets)
    {
        if (o.op.which() == operation::tag<withdraw_common_tokens_operation>::value)
        {
            withdraw_common_tokens_operation op = o.op.get<withdraw_common_tokens_operation>();
            const auto& account = db.get_account(op.account);
            const auto& bucket = db.get(bucket_id);

            auto new_common_tokens_withdrawal_rate = op.total_common_tokens_amount / DEIP_COMMON_TOKENS_WITHDRAW_INTERVALS;
            if (op.total_common_tokens_amount > 0 && new_common_tokens_withdrawal_rate == 0)
                new_common_tokens_withdrawal_rate = 1;

            db.modify(bucket, [&](bucket_object& b) {
                if (account.common_tokens_withdraw_rate > 0)
                    b.modified_common_tokens_withdrawal_requests++;
                else
                    b.new_common_tokens_withdrawal_requests++;

                // TODO: Figure out how to change delta when a vesting withdraw finishes. Have until March 24th 2018 to
                // figure that out...
                b.common_tokens_withdraw_rate_delta += new_common_tokens_withdrawal_rate - account.common_tokens_withdraw_rate;
            });
        }
    }
}

void blockchain_statistics_plugin_impl::post_operation(const operation_notification& o)
{
    try
    {
        auto& db = _self.database();

        for (auto bucket_id : _current_buckets)
        {
            const auto& bucket = db.get(bucket_id);

            if (!is_virtual_operation(o.op))
            {
                db.modify(bucket, [&](bucket_object& b) { b.operations++; });
            }
            o.op.visit(operation_process(_self, bucket));
        }
    }
    FC_CAPTURE_AND_RETHROW()
}

} // detail

blockchain_statistics_plugin::blockchain_statistics_plugin(application* app)
    : plugin(app)
    , _my(new detail::blockchain_statistics_plugin_impl(*this))
{
}

blockchain_statistics_plugin::~blockchain_statistics_plugin()
{
}

void blockchain_statistics_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                                              boost::program_options::options_description& cfg)
{
    cli.add_options()(
        "chain-stats-bucket-size",
        boost::program_options::value<string>()->default_value("[60,3600,21600,86400,604800,2592000]"),
        "Track blockchain statistics by grouping orders into buckets of equal size measured in seconds specified as a "
        "JSON array of numbers")(
        "chain-stats-history-per-bucket", boost::program_options::value<uint32_t>()->default_value(100),
        "How far back in time to track history for each bucket size, measured in the number of buckets (default: 100)");
    cfg.add(cli);
}

void blockchain_statistics_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    try
    {
        ilog("chain_stats_plugin: plugin_initialize() begin");
        chain::database& db = database();

        db.applied_block.connect([&](const signed_block& b) { _my->on_block(b); });
        db.pre_apply_operation.connect([&](const operation_notification& o) { _my->pre_operation(o); });
        db.post_apply_operation.connect([&](const operation_notification& o) { _my->post_operation(o); });

        db.add_plugin_index<bucket_index>();

        if (options.count("chain-stats-bucket-size"))
        {
            const std::string& buckets = options["chain-stats-bucket-size"].as<string>();
            _my->_tracked_buckets = fc::json::from_string(buckets).as<flat_set<uint32_t>>();
        }
        if (options.count("chain-stats-history-per-bucket"))
            _my->_maximum_history_per_bucket_size = options["chain-stats-history-per-bucket"].as<uint32_t>();

        wlog("chain-stats-bucket-size: ${b}", ("b", _my->_tracked_buckets));
        wlog("chain-stats-history-per-bucket: ${h}", ("h", _my->_maximum_history_per_bucket_size));

        ilog("chain_stats_plugin: plugin_initialize() end");
    }
    FC_CAPTURE_AND_RETHROW()
}

void blockchain_statistics_plugin::plugin_startup()
{
    ilog("chain_stats plugin: plugin_startup() begin");

    app().register_api_factory<blockchain_statistics_api>("chain_stats_api");

    ilog("chain_stats plugin: plugin_startup() end");
}

const flat_set<uint32_t>& blockchain_statistics_plugin::get_tracked_buckets() const
{
    return _my->_tracked_buckets;
}

uint32_t blockchain_statistics_plugin::get_max_history_per_bucket() const
{
    return _my->_maximum_history_per_bucket_size;
}
}
} // deip::blockchain_statistics

DEIP_DEFINE_PLUGIN(blockchain_statistics, deip::blockchain_statistics::blockchain_statistics_plugin);
