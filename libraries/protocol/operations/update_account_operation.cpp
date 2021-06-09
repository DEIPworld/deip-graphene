
#include <deip/protocol/deip_operations.hpp>
#include <deip/protocol/operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void update_account_operation::validate() const
{
    validate_account_name(account);

    if (owner.valid())
        owner->validate();
    if (active.valid())
        active->validate();

    if (json_metadata.valid())
    {
        const auto& json = *json_metadata;
        if (json.size() > 0)
        {
            FC_ASSERT(fc::is_utf8(json), "JSON Metadata not formatted in UTF8");
            FC_ASSERT(fc::json::is_valid(json), "JSON Metadata not valid JSON");
        }
    }

    if (active_overrides.valid())
    {
        const auto& auth_overrides = *active_overrides;
        for (const auto& active_override : auth_overrides)
        {
            FC_ASSERT(active_override.first < operation::count());
            if (active_override.second.valid())
            {
                const auto& auth_override = *active_override.second;
                auth_override.validate();
                FC_ASSERT(auth_override.key_auths.size() == 0); // disabled for now
            }
        }
    }
    
}

} // namespace protocol
} // namespace deip

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::update_account_extension)