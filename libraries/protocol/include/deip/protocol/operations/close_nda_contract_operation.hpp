#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct close_nda_contract_operation : public base_operation
{
    int64_t contract_id;
    account_name_type closer;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(closer);
    }

};

}
}

FC_REFLECT( deip::protocol::close_nda_contract_operation,
    (contract_id)
    (closer)
)
