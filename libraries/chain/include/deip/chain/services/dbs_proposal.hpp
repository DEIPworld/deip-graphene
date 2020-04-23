#pragma once

#include "dbs_base_impl.hpp"

#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/proposal_object.hpp>
#include <deip/chain/schema/account_object.hpp>

namespace deip {
namespace chain {

class proposal_object;

class dbs_proposal : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_proposal() = delete;
protected:
    explicit dbs_proposal(database& db);

public:
    using proposal_ref_type = std::vector<std::reference_wrapper<const proposal_object>>;
    using proposal_optional_ref_type = fc::optional<std::reference_wrapper<const proposal_object>>;

    const proposal_object& create_proposal(const external_id_type& external_id,
                                           const deip::protocol::transaction& proposed_trx,
                                           const fc::time_point_sec& expiration_time,
                                           const account_name_type& proposer,
                                           const fc::optional<uint32_t>& review_period_seconds,
                                           const flat_set<account_name_type>& required_owner,
                                           const flat_set<account_name_type>& required_active,
                                           const flat_set<account_name_type>& required_posting);

    const proposal_object& update_proposal( const proposal_object& proposal,
                                            const flat_set<account_name_type>& owner_approvals_to_add,
                                            const flat_set<account_name_type>& active_approvals_to_add,
                                            const flat_set<account_name_type>& posting_approvals_to_add,
                                            const flat_set<account_name_type>& owner_approvals_to_remove,
                                            const flat_set<account_name_type>& active_approvals_to_remove,
                                            const flat_set<account_name_type>& posting_approvals_to_remove,
                                            const flat_set<public_key_type>& key_approvals_to_add,
                                            const flat_set<public_key_type>& key_approvals_to_remove);

    const proposal_object& get_proposal(const proposal_id_type& id) const;

    const proposal_object& get_proposal(const external_id_type& external_id) const;

    const proposal_optional_ref_type get_proposal_if_exists(const external_id_type& external_id) const;

    const proposal_ref_type get_proposals_by_research_group_id(const account_name_type& proposer) const;

    void remove(const proposal_object& proposal);

    const bool proposal_exists(const external_id_type& external_id) const;

    const bool is_proposal_expired(const proposal_object& proposal) const;

    void clear_expired_proposals();
};

} // namespace chain
} // namespace deip