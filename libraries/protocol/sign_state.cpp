
#include <deip/protocol/sign_state.hpp>

namespace deip {
namespace protocol {

bool sign_state::signed_by(const public_key_type& k)
{
    auto itr = provided_signatures.find(k);
    if (itr == provided_signatures.end())
    {
        auto pk = available_keys.find(k);
        if (pk != available_keys.end())
            return provided_signatures[k] = true;
        return false;
    }
    return itr->second = true;
}

bool sign_state::check_authority(account_name_type id)
{
    if (approved_by.find(id) != approved_by.end()) return true;
    return check_authority(get_active(id)) || check_authority(get_owner(id));
}

bool sign_state::check_authority(const authority& auth, uint32_t depth)
{
    uint32_t total_weight = 0;
    for (const auto& k : auth.key_auths)
    {
        if (signed_by(k.first))
        {
            total_weight += k.second;
            if (total_weight >= auth.weight_threshold)
                return true;
        }
    }

    for (const auto& a : auth.account_auths)
    {
        if (approved_by.find(a.first) == approved_by.end())
        {
            if (depth == max_recursion)
                continue;
            if (check_authority(get_active(a.first), depth + 1) || check_authority(get_owner(a.first), depth + 1))
            {
                approved_by.insert(a.first);
                total_weight += a.second;
                if (total_weight >= auth.weight_threshold)
                    return true;
            }
        }
        else
        {
            total_weight += a.second;
            if (total_weight >= auth.weight_threshold)
                return true;
        }
    }
    return total_weight >= auth.weight_threshold;
}

bool sign_state::remove_unused_signatures()
{
    vector<public_key_type> remove_sigs;
    for (const auto& sig : provided_signatures)
        if (!sig.second)
            remove_sigs.push_back(sig.first);

    for (auto& sig : remove_sigs)
        provided_signatures.erase(sig);

    return remove_sigs.size() != 0;
}

sign_state::sign_state(const flat_set<public_key_type>& sigs,
                       const authority_getter& active_getter,
                       const authority_getter& owner_getter,
                       const flat_set<public_key_type>& keys)
    : get_active(active_getter)
    , get_owner(owner_getter)
    , available_keys(keys)
{
    for (const auto& key : sigs)
        provided_signatures[key] = false;
}
}
} // deip::protocol
