#pragma once
#include <deip/app/deip_api_objects.hpp>
#include <deip/proposal_history/proposal_history_objects.hpp>
#include <deip/proposal_history/proposal_state_object.hpp>

namespace deip {
namespace proposal_history {

struct proposal_state_api_obj
{
    proposal_state_api_obj(){};
    proposal_state_api_obj(const proposal_state_object& proposal)
        : id(proposal.id._id)
        , external_id(proposal.external_id)
        , proposer(proposal.proposer)
        , status(proposal.status)
        , expiration_time(proposal.expiration_time)
        , created_at(proposal.created_at)
        , fail_reason(fc::to_string(proposal.fail_reason))
        , proposed_transaction(proposal.proposed_transaction)
    {
        for (const auto& required_approval : proposal.required_approvals)
        {
            required_approvals.insert(required_approval);
        }

        for (const auto& pair : proposal.approvals)
        {
            approvals.insert(pair);
        }

        for (const auto& pair : proposal.rejectors)
        {
            rejectors.insert(pair);
        }

        if (proposal.review_period_time.valid())
        {
            review_period_time = *proposal.review_period_time;
        }

        for (const auto& approval : proposal.active_approvals)
        {
            active_approvals.insert(approval);
        }

        for (const auto& approval : proposal.owner_approvals)
        {
            owner_approvals.insert(approval);
        }

        for (const auto& approval : proposal.key_approvals)
        {
            key_approvals.insert(approval);
        }

        std::stringstream ss;
        fc::raw::pack(ss, proposal.proposed_transaction);
        std::string packed_trx = ss.str();
        serialized_proposed_transaction = fc::base64_encode( packed_trx );
    }

    int64_t id;

    external_id_type external_id;
    account_name_type proposer;

    uint8_t status;

    time_point_sec expiration_time;
    time_point_sec created_at;
    string fail_reason;

    flat_set<account_name_type> required_approvals;

    flat_map<account_name_type, tx_info> approvals;
    flat_map<account_name_type, tx_info> rejectors;


    flat_set<account_name_type> active_approvals;
    flat_set<account_name_type> owner_approvals;
    flat_set<public_key_type> key_approvals;

    transaction proposed_transaction;
    string serialized_proposed_transaction;
    optional<time_point_sec> review_period_time;

};

} // namespace proposal_history
}


FC_REFLECT(deip::proposal_history::proposal_state_api_obj, 
  (id)
  (external_id)
  (proposer)
  (status)
  (expiration_time)
  (created_at)
  (fail_reason)
  (required_approvals)
  (approvals)
  (rejectors)
  (active_approvals)
  (owner_approvals)
  (key_approvals)
  (proposed_transaction)
  (serialized_proposed_transaction)
  (review_period_time)
)
