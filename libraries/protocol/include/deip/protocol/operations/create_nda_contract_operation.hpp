#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

struct create_nda_contract_operation : public base_operation
{
    account_name_type contract_creator;

    account_name_type party_a;
    int64_t party_a_research_group_id;

    account_name_type party_b;
    int64_t party_b_research_group_id;

    std::set<account_name_type> disclosing_party;

    string title;
    string contract_hash;

    optional<fc::time_point_sec> start_date;
    fc::time_point_sec end_date;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(contract_creator);
    }

};

}
}

FC_REFLECT( deip::protocol::create_nda_contract_operation,
    (contract_creator)
    (party_a)
    (party_a_research_group_id)
    (party_b)
    (party_b_research_group_id)
    (disclosing_party)
    (title)
    (contract_hash)
    (start_date)
    (end_date)
)
