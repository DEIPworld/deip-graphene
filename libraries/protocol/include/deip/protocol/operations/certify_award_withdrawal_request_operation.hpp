#pragma once
#include <deip/protocol/base.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct certify_award_withdrawal_request_operation : public base_operation
{
    string payment_number;
    string award_number;
    fc::optional<std::string> subaward_number;

    account_name_type certifier;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(certifier);
    }

};

}
}

FC_REFLECT( deip::protocol::certify_award_withdrawal_request_operation, 
  (payment_number)
  (award_number)
  (subaward_number)
  (certifier)
)