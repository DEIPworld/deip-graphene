
#include <deip/protocol/deip_operations.hpp>
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
    if (posting.valid())
        posting->validate();

    if (json_metadata.valid())
    {
        const auto& json = *json_metadata;
        if (json.size() > 0)
        {
            FC_ASSERT(fc::is_utf8(json), "JSON Metadata not formatted in UTF8");
            FC_ASSERT(fc::json::is_valid(json), "JSON Metadata not valid JSON");
        }
    }
}

} // namespace protocol
} // namespace deip