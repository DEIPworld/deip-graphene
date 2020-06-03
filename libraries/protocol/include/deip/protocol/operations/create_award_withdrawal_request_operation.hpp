#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/asset.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct create_award_withdrawal_request_operation : public base_operation
{
    external_id_type payment_number;
    external_id_type award_number;
    fc::optional<external_id_type> subaward_number;
    account_name_type requester;
    asset amount;
    string description;
    string attachment;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(requester);
    }
};

}
}

FC_REFLECT( deip::protocol::create_award_withdrawal_request_operation, 
  (payment_number)
  (award_number)
  (subaward_number)
  (requester)
  (amount)
  (description)
  (attachment)
  (extensions)
)
