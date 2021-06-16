#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_asset.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/proposal_history/proposal_history_api.hpp>
#include <deip/proposal_history/proposal_history_plugin.hpp>
#include <deip/proposal_history/proposal_state_object.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace deip {
namespace proposal_history {

using namespace boost::gregorian;

namespace detail {

class proposal_history_api_impl
{
public:
    deip::app::application& _app;

public:
    proposal_history_api_impl(deip::app::application& app)
        : _app(app)
    {
    }

    std::vector<proposal_state_api_obj> get_proposals_by_signer(const account_name_type& account) const
    {
        std::vector<proposal_state_api_obj> results;

        const auto& db = _app.chain_database();
        const auto& proposal_state_idx = db->get_index<proposal_history_index>().indices().get<by_external_id>();
        const auto& proposal_lookup_idx = db->get_index<proposal_lookup_index>().indices().get<lookup_by_account>();

        uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
        for (auto itr = proposal_lookup_idx.lower_bound(account); limit-- && itr != proposal_lookup_idx.end() && itr->account == account; ++itr)
        {
            const auto& lookup = *itr;
            const auto& proposal_state_itr = proposal_state_idx.find(lookup.proposal);
            const auto& proposal_state_api = proposal_state_api_obj(*proposal_state_itr);
            results.push_back(proposal_state_api);
        }

        return results;
    }

    std::vector<proposal_state_api_obj> get_proposals_by_signers(const flat_set<account_name_type>& accounts) const
    {
        std::vector<proposal_state_api_obj> results;

        const auto& db = _app.chain_database();
        const auto& proposal_state_idx = db->get_index<proposal_history_index>().indices().get<by_external_id>();
        const auto& proposal_lookup_idx = db->get_index<proposal_lookup_index>().indices().get<lookup_by_account>();

        for (const auto& account : accounts)
        {
            uint32_t limit = DEIP_API_BULK_FETCH_LIMIT;
            for (auto itr = proposal_lookup_idx.lower_bound(account); limit-- && itr != proposal_lookup_idx.end() && itr->account == account; ++itr)
            {
                const auto& lookup = *itr;
                const auto& proposal_state_itr = proposal_state_idx.find(lookup.proposal);
                const auto& proposal_state_api = proposal_state_api_obj(*proposal_state_itr);
                results.push_back(proposal_state_api);
            }
        }

        return results;
    }

    fc::optional<proposal_state_api_obj> get_proposal_state(const external_id_type& external_id) const
    {
        fc::optional<proposal_state_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& proposal_state_idx = db->get_index<proposal_history_index>()
          .indices()
          .get<by_external_id>();

        const auto& proposal_state_itr = proposal_state_idx.find(external_id);
        if (proposal_state_itr != proposal_state_idx.end())
        {
            result = *proposal_state_itr;
        }

        return result;
    }

    std::vector<proposal_state_api_obj> get_proposals_states(const flat_set<external_id_type>& external_ids) const
    {
        vector<proposal_state_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& proposal_state_idx = db->get_index<proposal_history_index>().indices().get<by_external_id>();

        for (const auto& external_id : external_ids)
        {
            const auto& proposal_state_itr = proposal_state_idx.find(external_id);
            if (proposal_state_itr != proposal_state_idx.end())
            {
                result.push_back(*proposal_state_itr);
            }
        }

        return result;
    }

    std::vector<proposal_state_api_obj> lookup_proposals_states(const proposal_state_id_type& lower_bound, uint32_t limit) const
    {
        vector<proposal_state_api_obj> result;

        const auto& db = _app.chain_database();
        const auto& proposal_state_idx = db->get_index<proposal_history_index>().indices().get<by_id>();

        for (auto itr = proposal_state_idx.lower_bound(lower_bound); limit-- && itr != proposal_state_idx.end(); ++itr)
        {
            result.push_back(*itr);
        }

        return result;
    }

};
} // namespace detail

proposal_history_api::proposal_history_api(const deip::app::api_context& ctx)
    : _impl(new detail::proposal_history_api_impl(ctx.app))
{
}

proposal_history_api::~proposal_history_api()
{
}

void proposal_history_api::on_api_startup()
{
}

std::vector<proposal_state_api_obj> proposal_history_api::get_proposals_by_signer(const account_name_type& account) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->get_proposals_by_signer(account); });
}

std::vector<proposal_state_api_obj> proposal_history_api::get_proposals_by_signers(const flat_set<account_name_type>& accounts) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->get_proposals_by_signers(accounts); });
}

fc::optional<proposal_state_api_obj> proposal_history_api::get_proposal_state(const external_id_type& external_id) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->get_proposal_state(external_id); });
}

std::vector<proposal_state_api_obj> proposal_history_api::get_proposals_states(const flat_set<external_id_type>& external_ids) const
{
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->get_proposals_states(external_ids); });
}

std::vector<proposal_state_api_obj> proposal_history_api::lookup_proposals_states(const proposal_state_id_type& lower_bound, uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_API_BULK_FETCH_LIMIT);
    const auto db = _impl->_app.chain_database();
    return db->with_read_lock([&]() { return _impl->lookup_proposals_states(lower_bound, limit); });
}

} // namespace proposal_history
} // namespace deip