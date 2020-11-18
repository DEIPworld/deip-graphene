#include <deip/protocol/config.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>
#include <deip/chain/services/dbs_proposal.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/proposal_history/proposal_history_api.hpp>
#include <deip/proposal_history/proposal_history_plugin.hpp>
#include <deip/proposal_history/proposal_state_object.hpp>

#include <fc/shared_buffer.hpp>


namespace deip {
namespace proposal_history {

using namespace deip::protocol;
using namespace deip::chain;
using namespace deip::app;

namespace detail {

class proposal_history_plugin_impl
{
public:

    proposal_history_plugin_impl(proposal_history_plugin& _plugin)
        : _self(_plugin)
    {
    }

    virtual ~proposal_history_plugin_impl()
    {
    }

    deip::chain::database& database()
    {
        return _self.database();
    }

    void pre_operation(const operation_notification& op_obj);
    void post_operation(const operation_notification& op_obj);

    proposal_history_plugin& _self;
};

struct post_operation_visitor
{
    proposal_history_plugin& _plugin;
    const operation_notification& note;

    post_operation_visitor(proposal_history_plugin& plugin, const operation_notification& _note)
        : _plugin(plugin)
        , note(_note)
    {

    }

    typedef void result_type;

    template <typename T> void operator()(const T&) const
    {
    }

    void operator()(const create_proposal_operation& op) const
    {
        const auto& db = _plugin.database();
        const auto& proposals_service = db.obtain_service<chain::dbs_proposal>();

        const auto& proposal = proposals_service.get_proposal(op.external_id);

        const auto& proposal_state = _plugin.database().create<proposal_state_object>([&](proposal_state_object& ps_o) {
            ps_o.external_id = proposal.external_id;
            ps_o.proposer = proposal.proposer;
            ps_o.status = static_cast<uint8_t>(proposal_status::pending);

            for (const auto& approver : proposal.required_active_approvals)
            {
                ps_o.required_approvals.insert(approver);
            }

            for (const auto& approver : proposal.required_owner_approvals)
            {
                ps_o.required_approvals.insert(approver);
            }

            ps_o.proposed_transaction = proposal.proposed_transaction;
            ps_o.expiration_time = proposal.expiration_time;
            ps_o.created_at = proposal.created_at;

            if (proposal.review_period_time.valid())
            {
                ps_o.review_period_time = *proposal.review_period_time;
            }
        });

        for (const account_name_type& approver : proposal_state.required_approvals)
        {
            _plugin.database().create<proposal_lookup_object>([&](proposal_lookup_object& pl_o) {
                pl_o.proposal = proposal.external_id;
                pl_o.account = approver;
            });
        }

        if (proposal_state.required_approvals.find(proposal.proposer) == proposal_state.required_approvals.end())
        {
            _plugin.database().create<proposal_lookup_object>([&](proposal_lookup_object& pl_o) {
                pl_o.proposal = proposal.external_id;
                pl_o.account = proposal.proposer;
            });
        }
    }

    void operator()(const update_proposal_operation& op) const
    {
        auto& db = _plugin.database();
        const auto& block_time = db.head_block_time();
        const auto& block_num = db.head_block_num();

        auto& proposal_state_idx = db.get_index<proposal_history_index>().indices().get<by_external_id>();

        const auto& prop_state_itr = proposal_state_idx.find(op.external_id);
        FC_ASSERT(prop_state_itr != proposal_state_idx.end());

        db.modify(*prop_state_itr, [&](proposal_state_object& ps_o) {
            flat_set<account_name_type> approvers;
            approvers.insert(op.active_approvals_to_add.begin(), op.active_approvals_to_add.end());
            approvers.insert(op.owner_approvals_to_add.begin(), op.owner_approvals_to_add.end());

            for (const auto& approver : approvers)
            {
                tx_info signer_info;
                signer_info.trx_id = note.trx_id;
                signer_info.block_num = block_num;
                signer_info.timestamp = block_time;

                ps_o.approvals.insert(std::make_pair(approver, signer_info));
            }

            flat_set<account_name_type> revokers;
            revokers.insert(op.active_approvals_to_remove.begin(), op.active_approvals_to_remove.end());
            revokers.insert(op.owner_approvals_to_remove.begin(), op.owner_approvals_to_remove.end());

            for (const auto& revoker : revokers)
            {
                const auto& itr = ps_o.approvals.find(revoker);
                if (itr != ps_o.approvals.end())
                {
                    ps_o.approvals.erase(itr);
                }
            }
        });
    }

    void operator()(const delete_proposal_operation& op) const
    {
        auto& db = _plugin.database();
        const auto& block_time = db.head_block_time();
        const auto& block_num = db.head_block_num();

        auto& proposal_state_idx = db.get_index<proposal_history_index>().indices().get<by_external_id>();

        const auto& prop_state_itr = proposal_state_idx.find(op.external_id);
        FC_ASSERT(prop_state_itr != proposal_state_idx.end());

        db.modify(*prop_state_itr, [&](proposal_state_object& ps_o) {
            tx_info signer_info;
            signer_info.trx_id = note.trx_id;
            signer_info.block_num = block_num;
            signer_info.timestamp = block_time;
            ps_o.rejectors.insert(std::make_pair(op.account, signer_info));

            ps_o.status = static_cast<uint8_t>(proposal_status::rejected);
        });
    }

    void operator()(const proposal_status_changed_operation& op) const
    {
        auto& db = _plugin.database();
        const auto& proposals_service = db.obtain_service<chain::dbs_proposal>();
        auto& proposal_state_idx = db.get_index<proposal_history_index>().indices().get<by_external_id>();

        const auto& prop_state_itr = proposal_state_idx.find(op.external_id);
        FC_ASSERT(prop_state_itr != proposal_state_idx.end());

        string fail_reason;
        if (op.status == static_cast<uint8_t>(proposal_status::failed))
        {
            const auto& proposal = proposals_service.get_proposal(op.external_id);
            fail_reason = fc::to_string(proposal.fail_reason);
        }

        db.modify(*prop_state_itr, [&](proposal_state_object& ps_o) {
            ps_o.status = op.status;
            fc::from_string(ps_o.fail_reason, fail_reason);
        });
    }
};

void proposal_history_plugin_impl::pre_operation(const operation_notification& note)
{
}

void proposal_history_plugin_impl::post_operation(const operation_notification& note)
{
    note.op.visit(post_operation_visitor(_self, note));
}

} // end namespace detail

proposal_history_plugin::proposal_history_plugin(application* app)
    : plugin(app)
    , my(new detail::proposal_history_plugin_impl(*this))
{
    ilog("Loading Proposals history plugin");
}

proposal_history_plugin::~proposal_history_plugin()
{
}

std::string proposal_history_plugin::plugin_name() const
{
    return PROPOSAL_HISTORY_PLUGIN_NAME;
}

void proposal_history_plugin::plugin_set_program_options(boost::program_options::options_description& cli, boost::program_options::options_description& cfg)
{

}


void proposal_history_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    ilog("Intializing Investemnts history plugin");

    chain::database& db = database();
    db.add_plugin_index<proposal_history_index>();
    db.add_plugin_index<proposal_lookup_index>();

    db.pre_apply_operation.connect([&](const operation_notification& note) { my->pre_operation(note); });
    db.post_apply_operation.connect([&](const operation_notification& note) { my->post_operation(note); });
}

void proposal_history_plugin::plugin_startup()
{
    ilog("proposal_history plugin: plugin_startup() begin");
    app().register_api_factory<proposal_history_api>("proposal_history_api");
    ilog("proposal_history plugin: plugin_startup() end");
}


} // namespace proposal_history
} // namespace deip

DEIP_DEFINE_PLUGIN(proposal_history, deip::proposal_history::proposal_history_plugin)