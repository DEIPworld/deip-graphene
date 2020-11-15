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

        for (const auto& approval : proposal.approvals)
        {
            approvals.insert(approval);
        }

        rejectors.insert(proposal.rejector);

        for (const auto& key : proposal.signers)
        {
            signers.insert(key);
        }

        if (proposal.review_period_time.valid())
        {
            review_period_time = *proposal.review_period_time;
        }
    }

    int64_t id;

    external_id_type external_id;
    account_name_type proposer;

    uint8_t status;

    time_point_sec expiration_time;
    time_point_sec created_at;
    string fail_reason;

    flat_set<account_name_type> required_approvals;
    flat_set<account_name_type> approvals;
    flat_set<account_name_type> rejectors;

    flat_set<public_key_type> signers;
    transaction proposed_transaction;
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
  (signers)
  (proposed_transaction)
  (review_period_time)
)
