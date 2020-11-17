#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_proposal.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_dynamic_global_properties.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_proposal::dbs_proposal(database& db)
    : _base_type(db)
{
}

const proposal_object& dbs_proposal::get_proposal(const proposal_id_type& id) const
{
    try { return db_impl().get<proposal_object>(id); }
    FC_CAPTURE_AND_RETHROW((id))
}

const proposal_object& dbs_proposal::get_proposal(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<proposal_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    FC_ASSERT(itr != idx.end(), "Proposal ${1} does not exist.", ("1", external_id));
    return *itr;
}

const dbs_proposal::proposal_optional_ref_type dbs_proposal::get_proposal_if_exists(const external_id_type& external_id) const
{
    proposal_optional_ref_type result;

    const auto& idx = db_impl()
      .get_index<proposal_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const bool dbs_proposal::proposal_exists(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<proposal_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    return itr != idx.end();
}

const dbs_proposal::proposal_ref_type dbs_proposal::get_proposals_by_creator(const account_name_type& creator) const
{
    proposal_ref_type ret;

    const auto& idx = db_impl()
      .get_index<proposal_index>()
      .indicies()
      .get<by_proposer>();

    auto it_pair = idx.equal_range(creator);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const proposal_object& dbs_proposal::create_proposal(
  const external_id_type& external_id,
  const deip::protocol::transaction& proposed_trx,
  const fc::time_point_sec& expiration_time,
  const account_name_type& proposer,
  const fc::optional<uint32_t>& review_period_seconds,
  const flat_set<account_name_type>& required_owner,
  const flat_set<account_name_type>& required_active
) 
{
    auto& dgp_service = db_impl().obtain_service<dbs_dynamic_global_properties>();

    FC_ASSERT(proposed_trx.expiration == expiration_time,
      "Proposal expiration time must be equal to proposed transaction expiration time");

    std::vector<account_name_type> owner_active_diff;

    std::set_intersection(
      required_owner.begin(), required_owner.end(), 
      required_active.begin(), required_active.end(),
      std::inserter(owner_active_diff, owner_active_diff.begin()));

    FC_ASSERT(owner_active_diff.size() == 0,
      "Ambiguous owner and active authorities detected", 
      ("owner", required_owner)
      ("active", required_active)
    );

    const proposal_object& proposal = db_impl().create<proposal_object>([&](proposal_object& p_o) {
        p_o.external_id = external_id;
        p_o.proposed_transaction = proposed_trx;
        p_o.expiration_time = expiration_time;
        p_o.proposer = proposer;

        if (review_period_seconds)
        {
            p_o.review_period_time = expiration_time - *review_period_seconds;
        }

        p_o.required_owner_approvals.insert(required_owner.begin(), required_owner.end());
        p_o.required_active_approvals.insert(required_active.begin(), required_active.end());
    });

    dgp_service.create_recent_entity(external_id);

    return proposal;
}


const proposal_object& dbs_proposal::update_proposal(
  const proposal_object& proposal,
  const flat_set<account_name_type>& owner_approvals_to_add,
  const flat_set<account_name_type>& active_approvals_to_add,
  const flat_set<account_name_type>& owner_approvals_to_remove,
  const flat_set<account_name_type>& active_approvals_to_remove,
  const flat_set<public_key_type>& key_approvals_to_add,
  const flat_set<public_key_type>& key_approvals_to_remove 
)
{
    db_impl().modify(proposal, [&](proposal_object& p_o) {
        p_o.available_owner_approvals.insert(owner_approvals_to_add.begin(), owner_approvals_to_add.end());
        p_o.available_active_approvals.insert(active_approvals_to_add.begin(), active_approvals_to_add.end());

        for (account_name_type account : owner_approvals_to_remove)
            p_o.available_owner_approvals.erase(account);
        for (account_name_type account : active_approvals_to_remove)
            p_o.available_active_approvals.erase(account);

        for (const auto& key : key_approvals_to_add)
            p_o.available_key_approvals.insert(key);
        for (const auto& key : key_approvals_to_remove)
            p_o.available_key_approvals.erase(key);
    });
    
    return proposal;
}

void dbs_proposal::remove_proposal(const proposal_object& proposal)
{
    db_impl().remove(proposal);
}

void dbs_proposal::clear_expired_proposals()
{
    const auto block_time = db_impl().head_block_time();
    const auto& idx = db_impl()
      .get_index<proposal_index>()
      .indicies()
      .get<by_expiration>();

    while (!idx.empty() && idx.begin()->expiration_time <= block_time)
    {
        const proposal_object& proposal = *idx.begin();
        const external_id_type proposal_id = proposal.external_id;
        try
        {
            if (proposal.is_authorized_to_execute(db_impl()))
            {
                db_impl().push_proposal(proposal);
                // TODO: Do something with result so plugins can process it.
                continue;
            }
            else 
            {
                db_impl().push_virtual_operation(proposal_status_changed_operation(proposal_id, static_cast<uint8_t>(proposal_status::expired)));
            }
        }
        catch (const fc::exception& e)
        {
            db_impl().reset_current_proposed_trx();
            elog("Failed to apply proposed transaction on its expiration. Deleting it.\n${proposal}\n${error}",
                 ("proposal", proposal)("error", e.to_detail_string()));
        }
        remove_proposal(proposal);
    }
}

} // namespace chain
} // namespace deip

