#include <deip/protocol/config.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>

#include <deip/fo_history/fo_history_plugin.hpp>
#include <deip/fo_history/fo_history_api.hpp>

#include <deip/fo_history/withdrawal_request_history_object.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <boost/algorithm/string.hpp>

namespace deip {
namespace fo_history {

using namespace deip::protocol;
using namespace deip::chain;

namespace detail {

class fo_history_plugin_impl
{
public:
    fo_history_plugin_impl(fo_history_plugin& _plugin)
        : _self(_plugin)
    {
        chain::database& db = database();

        db.add_plugin_index<withdrawal_request_history_index>();
        
        db.post_apply_operation.connect([&](const operation_notification& note) { on_operation(note); });
    }
    virtual ~fo_history_plugin_impl()
    {
    }

    deip::chain::database& database()
    {
        return _self.database();
    }

    void on_operation(const operation_notification& note);

    fo_history_plugin& _self;
};


void fo_history_plugin_impl::on_operation(const operation_notification& note)
{
    deip::chain::database& db = database();

    if (note.op.which() == operation::tag<create_award_withdrawal_request_operation>::value)
    {
        create_award_withdrawal_request_operation op = note.op.get<create_award_withdrawal_request_operation>();
        db.create<withdrawal_request_history_object>([&](withdrawal_request_history_object& hist_o) {
            fc::from_string(hist_o.payment_number, op.payment_number);
            fc::from_string(hist_o.award_number, op.award_number);

            const string subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;
            fc::from_string(hist_o.subaward_number, subaward_number);

            hist_o.requester = op.requester;
            hist_o.certifier = account_name_type();
            hist_o.approver = account_name_type();
            hist_o.rejector = account_name_type();
            hist_o.payer = account_name_type();

            hist_o.amount = op.amount;

            fc::from_string(hist_o.description, op.description);
            fc::from_string(hist_o.attachment, op.attachment);
            
            hist_o.status = static_cast<uint16_t>(award_withdrawal_request_status::pending);
            hist_o.version = 1;

            hist_o.trx_id = note.trx_id;
            hist_o.block = note.block;
            hist_o.trx_in_block = note.trx_in_block;
            hist_o.op_in_trx = note.op_in_trx;
        });
    }

    else if (note.op.which() == operation::tag<certify_award_withdrawal_request_operation>::value)
    {
        certify_award_withdrawal_request_operation op = note.op.get<certify_award_withdrawal_request_operation>();

        const auto& idx = db.get_index<withdrawal_request_history_index>().indices().get<by_award_payment_and_version>();
        auto it_pair = idx.equal_range(std::make_tuple(op.award_number, op.payment_number));

        const withdrawal_request_history_object& withdrawal = *(--it_pair.second);

        db.create<withdrawal_request_history_object>([&](withdrawal_request_history_object& hist_o) {
            fc::from_string(hist_o.payment_number, op.payment_number);
            fc::from_string(hist_o.award_number, op.award_number);

            const string subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;
            fc::from_string(hist_o.subaward_number, subaward_number);

            hist_o.requester = withdrawal.requester;
            hist_o.certifier = op.certifier;
            hist_o.approver = withdrawal.approver;
            hist_o.rejector = withdrawal.rejector;
            hist_o.payer = withdrawal.payer;

            hist_o.amount = withdrawal.amount;

            hist_o.description = withdrawal.description;
            hist_o.attachment = withdrawal.attachment;

            hist_o.status = static_cast<uint16_t>(award_withdrawal_request_status::certified);
            hist_o.version = withdrawal.version + 1;

            hist_o.trx_id = note.trx_id;
            hist_o.block = note.block;
            hist_o.trx_in_block = note.trx_in_block;
            hist_o.op_in_trx = note.op_in_trx;
        });
    }

    else if (note.op.which() == operation::tag<approve_award_withdrawal_request_operation>::value)
    {
        approve_award_withdrawal_request_operation op = note.op.get<approve_award_withdrawal_request_operation>();

        const auto& idx = db.get_index<withdrawal_request_history_index>().indices().get<by_award_payment_and_version>();
        auto it_pair = idx.equal_range(std::make_tuple(op.award_number, op.payment_number));

        const withdrawal_request_history_object& withdrawal = *(--it_pair.second);

        db.create<withdrawal_request_history_object>([&](withdrawal_request_history_object& hist_o) {
            fc::from_string(hist_o.payment_number, op.payment_number);
            fc::from_string(hist_o.award_number, op.award_number);

            const string subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;
            fc::from_string(hist_o.subaward_number, subaward_number);

            hist_o.requester = withdrawal.requester;
            hist_o.certifier = withdrawal.certifier;
            hist_o.approver = op.approver;
            hist_o.rejector = withdrawal.rejector;
            hist_o.payer = withdrawal.payer;

            hist_o.amount = withdrawal.amount;

            hist_o.description = withdrawal.description;
            hist_o.attachment = withdrawal.attachment;

            hist_o.status = static_cast<uint16_t>(award_withdrawal_request_status::approved);
            hist_o.version = withdrawal.version + 1;

            hist_o.trx_id = note.trx_id;
            hist_o.block = note.block;
            hist_o.trx_in_block = note.trx_in_block;
            hist_o.op_in_trx = note.op_in_trx;
        });
    }

    else if (note.op.which() == operation::tag<reject_award_withdrawal_request_operation>::value)
    {
        reject_award_withdrawal_request_operation op = note.op.get<reject_award_withdrawal_request_operation>();

        const auto& idx = db.get_index<withdrawal_request_history_index>().indices().get<by_award_payment_and_version>();
        auto it_pair = idx.equal_range(std::make_tuple(op.award_number, op.payment_number));

        const withdrawal_request_history_object& withdrawal = *(--it_pair.second);

        db.create<withdrawal_request_history_object>([&](withdrawal_request_history_object& hist_o) {
            fc::from_string(hist_o.payment_number, op.payment_number);
            fc::from_string(hist_o.award_number, op.award_number);

            const string subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;
            fc::from_string(hist_o.subaward_number, subaward_number);

            hist_o.requester = withdrawal.requester;
            hist_o.certifier = withdrawal.certifier;
            hist_o.approver = withdrawal.approver;
            hist_o.rejector = op.rejector;
            hist_o.payer = withdrawal.payer;

            hist_o.amount = withdrawal.amount;

            hist_o.description = withdrawal.description;
            hist_o.attachment = withdrawal.attachment;

            hist_o.status = static_cast<uint16_t>(award_withdrawal_request_status::rejected);
            hist_o.version = withdrawal.version + 1;

            hist_o.trx_id = note.trx_id;
            hist_o.block = note.block;
            hist_o.trx_in_block = note.trx_in_block;
            hist_o.op_in_trx = note.op_in_trx;
        });
    }

    else if (note.op.which() == operation::tag<pay_award_withdrawal_request_operation>::value)
    {
        pay_award_withdrawal_request_operation op = note.op.get<pay_award_withdrawal_request_operation>();

        const auto& idx = db.get_index<withdrawal_request_history_index>().indices().get<by_award_payment_and_version>();
        auto it_pair = idx.equal_range(std::make_tuple(op.award_number, op.payment_number));

        const withdrawal_request_history_object& withdrawal = *(--it_pair.second);

        db.create<withdrawal_request_history_object>([&](withdrawal_request_history_object& hist_o) {
            fc::from_string(hist_o.payment_number, op.payment_number);
            fc::from_string(hist_o.award_number, op.award_number);

            const string subaward_number = op.subaward_number.valid() ? *op.subaward_number : op.award_number;
            fc::from_string(hist_o.subaward_number, subaward_number);

            hist_o.requester = withdrawal.requester;
            hist_o.certifier = withdrawal.certifier;
            hist_o.approver = withdrawal.approver;
            hist_o.rejector = withdrawal.rejector;
            hist_o.payer = op.payer;

            hist_o.amount = withdrawal.amount;

            hist_o.description = withdrawal.description;
            hist_o.attachment = withdrawal.attachment;

            hist_o.status = static_cast<uint16_t>(award_withdrawal_request_status::paid);
            hist_o.version = withdrawal.version + 1;

            hist_o.trx_id = note.trx_id;
            hist_o.block = note.block;
            hist_o.trx_in_block = note.trx_in_block;
            hist_o.op_in_trx = note.op_in_trx;
        });
    }
}

} // end namespace detail

fo_history_plugin::fo_history_plugin(application* app)
    : plugin(app)
    , my(new detail::fo_history_plugin_impl(*this))
{
    ilog("Loading funding opportunity history plugin");
}

fo_history_plugin::~fo_history_plugin()
{
}

std::string fo_history_plugin::plugin_name() const
{
    return FO_HISTORY_PLUGIN_NAME;
}

void fo_history_plugin::plugin_set_program_options(boost::program_options::options_description& cli, boost::program_options::options_description& cfg)
{

}

void fo_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    ilog("Intializing funding opportunity history plugin");
}

void fo_history_plugin::plugin_startup()
{
    ilog("fo_history plugin: plugin_startup() begin");
    app().register_api_factory<fo_history_api>("fo_history_api");
    ilog("fo_history plugin: plugin_startup() end");
}

} // namespace fo_history
} // namespace deip

DEIP_DEFINE_PLUGIN(fo_history, deip::fo_history::fo_history_plugin)