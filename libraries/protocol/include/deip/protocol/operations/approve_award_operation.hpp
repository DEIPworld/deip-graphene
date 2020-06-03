#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

struct approve_award_operation : public base_operation
{
    external_id_type award_number;
    account_name_type approver;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(approver);
    }
};


}
}

FC_REFLECT(deip::protocol::approve_award_operation, (award_number)(approver)(extensions))