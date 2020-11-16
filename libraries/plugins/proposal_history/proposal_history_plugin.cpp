#include <deip/protocol/config.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/operation_notification.hpp>
#include <deip/chain/services/dbs_proposal.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/proposal_history/proposal_history_api.hpp>
#include <deip/proposal_history/proposal_history_plugin.hpp>
#include <deip/proposal_history/proposal_state_object.hpp>

#include <deip/app/api.hpp>
#include <deip/account_by_key/account_by_key_api.hpp>
#include <deip/account_by_key/account_by_key_plugin.hpp>

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
    std::shared_ptr<deip::account_by_key::detail::account_by_key_api_impl> account_by_key_api_impl;
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

        _plugin.database().create<proposal_state_object>([&](proposal_state_object& ps_o) {
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
    }

    void operator()(const update_proposal_operation& op) const
    {
        auto& db = _plugin.database();
        const auto& transactions_idx = db.get_index<transaction_index>().indices().get<by_trx_id>();
        auto& proposal_state_idx = db.get_index<proposal_history_index>().indices().get<by_external_id>();

        auto trx_itr = transactions_idx.find(note.trx_id);
        FC_ASSERT(trx_itr != transactions_idx.end());
        signed_transaction signed_trx;
        fc::raw::unpack(trx_itr->packed_trx, signed_trx);

        const auto& signature_keys = signed_trx.get_signature_keys(db.get_chain_id());

        vector<public_key_type> request;
        for (const auto& signature_key : signature_keys)
        {
            request.push_back(signature_key);
        }

        const auto& keyysss = _plugin.my->account_by_key_api_impl->get_key_references(request, false);

        const auto& prop_itr = proposal_state_idx.find(op.external_id);
        FC_ASSERT(prop_itr != proposal_state_idx.end());

        db.modify(*prop_itr, [&](proposal_state_object& ps_o) {
            
            for (const auto& signature_key : signature_keys)
            {
                ps_o.signers.insert(signature_key);
            }

            for (const auto& approver : op.active_approvals_to_add)
            {
                ps_o.approvals.insert(approver);
            }

            for (const auto& approver : op.owner_approvals_to_add)
            {
                ps_o.approvals.insert(approver);
            }

            for (const auto& revoker : op.active_approvals_to_remove)
            {
                const auto& itr = ps_o.approvals.find(revoker);
                if (itr != ps_o.approvals.end())
                {
                    ps_o.approvals.erase(itr);
                }
            }

            for (const auto& revoker : op.owner_approvals_to_remove)
            {
                const auto& itr = ps_o.approvals.find(revoker);
                if (itr != ps_o.approvals.end())
                {
                    ps_o.approvals.erase(itr);
                }
            }
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

    my->account_by_key_api_impl = std::make_shared<deip::account_by_key::detail::account_by_key_api_impl>(deip::account_by_key::detail::account_by_key_api_impl(app()));
    
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