#pragma once
#include <deip/protocol/base.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct approve_award_withdrawal_request_operation : public base_operation
{
    external_id_type payment_number;
    external_id_type award_number;
    fc::optional<external_id_type> subaward_number;

    account_name_type approver;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(approver);
    }

};


}
}

FC_REFLECT( deip::protocol::approve_award_withdrawal_request_operation, 
  (payment_number)
  (award_number)
  (subaward_number)
  (approver) 
)