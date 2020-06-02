#pragma once

#include <deip/protocol/authority.hpp>

namespace deip {
namespace protocol {

typedef std::function<authority(const string& account)> authority_getter;
typedef std::function<fc::optional<authority>(const string& account, const uint16_t& op_tag)> override_authority_getter;

struct sign_state
{
    /** returns true if we have a signature for this key or can
     * produce a signature for this key, else returns false.
     */
    bool signed_by(const public_key_type& k);
    bool check_authority(account_name_type id);

    /**
     *  Checks to see if we have signatures of the active authorites of
     *  the accounts specified in authority or the keys specified.
     */
    bool check_authority(const authority& au, uint32_t depth = 0);

    bool remove_unused_signatures();

    sign_state(const flat_set<public_key_type>& sigs,
               const authority_getter& active_getter,
               const authority_getter& owner_getter,
               const flat_set<public_key_type>& keys);

    const authority_getter& get_active;
    const authority_getter& get_owner;
    const flat_set<public_key_type>& available_keys;

    flat_map<public_key_type, bool> provided_signatures;
    flat_set<account_name_type> approved_by;
    uint32_t max_recursion = DEIP_MAX_SIG_CHECK_DEPTH;
};
}
} // deip::protocol
