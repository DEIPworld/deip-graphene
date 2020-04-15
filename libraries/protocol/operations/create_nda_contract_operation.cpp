#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_nda_contract_operation::validate() const
{
    validate_account_name(party_a);
    validate_account_name(party_b);
    validate_account_name(contract_creator);
    FC_ASSERT(party_a != party_b, "Parties must not be the same accounts");

    // WARNING: Currently we are not supporting sharing files by both sides within a single NDA contract
    FC_ASSERT(contract_creator == party_a, "Two-way NDA contracts are not supported currently");
    FC_ASSERT(disclosing_party.size() == 1, "Two-way NDA contracts are not supported currently");
    std::set<account_name_type>::iterator it = disclosing_party.begin();
    account_name_type disclosing_party_account = *it;
    FC_ASSERT(disclosing_party_account == party_a, "Two-way NDA contracts are not supported currently");

    FC_ASSERT(title.size() > 0 && title.size() < 200, "Contract title must be specified in length from 1 to 200 characters");
    validate_256_bits_hexadecimal_string(contract_hash);

}

} /* deip::protocol */
} /* protocol */