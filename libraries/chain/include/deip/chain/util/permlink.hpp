#pragma once

#include <deip/protocol/types.hpp>

namespace deip {
namespace chain {
namespace util {

/* [DEPRECATED] */
inline std::string generate_permlink(const string& source)
{
    std::string permlink;
    for (const char& ch : source)
    {
        if (isalnum(ch))
        {
            permlink.push_back(char(std::tolower(ch)));
        }
        if (ch == char('-'))
        {
            permlink.push_back(ch);
        }
        if (isblank(ch))
        {
            permlink.push_back(char('-'));
        }
    }

    FC_ASSERT(!permlink.empty(), "Permlink can not be empty");
    FC_ASSERT(permlink.size() <= DEIP_MAX_PERMLINK_LENGTH);
    return permlink;
}

} // namespace util
} // namespace chain
} // namespace deip
