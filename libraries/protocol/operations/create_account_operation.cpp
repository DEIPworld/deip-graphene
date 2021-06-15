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
    if (trait.which() == account_trait::tag<research_group_trait>::value)
    {
        const auto rg_trait = trait.get<research_group_trait>();

        if (rg_trait.description.size() != 0)
        {
            FC_ASSERT(rg_trait.description.size() <= DEIP_MAX_MEMO_SIZE);
            FC_ASSERT(fc::is_utf8(rg_trait.description), "Research group description is not valid UTF-8 string");
        }

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

    for (const auto& active_override : active_overrides)
    {
        FC_ASSERT(active_override.first < operation::count());
        active_override.second.validate();       
        FC_ASSERT(active_override.second.key_auths.size() == 0); // disabled for now
    }
}

bool create_account_operation::is_research_group_account() const
{
    return std::count_if(traits.begin(), traits.end(), 
      [&](const account_trait& trait) {
        return trait.which() == account_trait::tag<research_group_trait>::value;
      }) != 0;
}

bool create_account_operation::is_user_account() const
{
    return traits.size() == 0;
}

} // namespace protocol
} // namespace deip

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::account_trait)