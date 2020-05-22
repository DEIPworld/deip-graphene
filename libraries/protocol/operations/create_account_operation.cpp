#include <deip/protocol/deip_operations.hpp>
#include <deip/protocol/operations.hpp>
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

        FC_ASSERT(!research_group_trait.name.empty(), "Research group name is required");
        FC_ASSERT(fc::is_utf8(research_group_trait.name), "Research group name is not valid UTF-8 string");
        FC_ASSERT(fc::is_utf8(research_group_trait.description), "Research group description is not valid UTF-8 string");
        is_validated = true;
    }

    FC_ASSERT(is_validated, "Account trait is unknown or not validated");
}

void create_account_operation::validate() const
{
    validate_account_name(new_account_name);
    owner.validate();
    active.validate();

    if (json_metadata.valid())
    {
        const auto& metadata = *json_metadata;
        FC_ASSERT(fc::is_utf8(metadata), "JSON Metadata not formatted in UTF8");
        FC_ASSERT(fc::json::is_valid(metadata), "JSON Metadata not valid JSON");
    }

    FC_ASSERT(fee >= DEIP_MIN_ACCOUNT_CREATION_FEE,
              "Insufficient Fee: ${f} required, ${p} provided.",
              ("f", DEIP_MIN_ACCOUNT_CREATION_FEE)("p", fee));
    
    for(auto& trait : traits)
    {
        validate_account_trait(creator, trait);
    }
}

bool create_account_operation::is_research_group_account() const
{
    return std::count_if(traits.begin(), traits.end(), 
      [&](const account_trait& trait) {
        return trait.which() == account_trait::tag<research_group_v1_0_0_trait>::value;
      }) != 0;
}

bool create_account_operation::is_user_account() const
{
    return traits.size() == 0;
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