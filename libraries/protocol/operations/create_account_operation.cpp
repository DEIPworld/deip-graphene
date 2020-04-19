#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void validate_account_trait(const account_name_type& creator, const account_trait& trait)
{
    bool is_validated = false;
    if (trait.which() == account_trait::tag<research_group_v1_0_0_trait>::value)
    {
        const auto research_group_trait = trait.get<research_group_v1_0_0_trait>();

        validate_permlink(research_group_trait.permlink);
        FC_ASSERT(research_group_trait.name.size() > 0, "Research group name is required");
        FC_ASSERT(fc::is_utf8(research_group_trait.name), "Research group name is not valid UTF-8 string");
        FC_ASSERT(fc::is_utf8(research_group_trait.description), "Research group description is not valid UTF-8 string");
        FC_ASSERT(std::none_of(research_group_trait.invitees.begin(), research_group_trait.invitees.end(), [&](const invitee_type& invitee) { return invitee.account == creator; }),
          "Research group creator should not be specified in invitees list.", ("c", creator));

        is_validated = true;
    }

    FC_ASSERT(is_validated, "Account trait is unknown or not validated");
}

void create_account_operation::validate() const
{
    validate_account_name(new_account_name);
    owner.validate();
    active.validate();

    if (json_metadata.size() > 0)
    {
        FC_ASSERT(fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8");
        FC_ASSERT(fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON");
    }

    FC_ASSERT(fee >= asset(DEIP_MIN_ACCOUNT_CREATION_FEE, DEIP_SYMBOL),
              "Insufficient Fee: ${f} required, ${p} provided.",
              ("f", asset(DEIP_MIN_ACCOUNT_CREATION_FEE, DEIP_SYMBOL))("p", fee));
    
    for(auto& trait : traits)
    {
        validate_account_trait(creator, trait);
    }
}

} // namespace protocol
} // namespace deip

namespace fc {

std::string account_trait_name_from_type(const std::string& type_name)
{
    auto start = type_name.find_last_of(':') + 1;
    auto end = type_name.find_last_of('_');
    auto result = type_name.substr(start, end - start);
    return result;
}

} // namespace fc

DEFINE_ACCOUNT_TRAIT_TYPE(deip::protocol::account_trait)