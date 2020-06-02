
#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/version.hpp>
#include <deip/protocol/asset.hpp>

namespace deip {
namespace protocol {

using deip::protocol::asset;
using deip::protocol::authority;

struct research_group_trait;

typedef fc::static_variant<
  research_group_trait
  >
  account_trait;


struct update_account_operation : public base_operation
{
    account_name_type account;
    optional<authority> owner;
    optional<authority> active;
    optional<flat_map<uint16_t, optional<authority>>> active_overrides;
    optional<public_key_type> memo_key;
    optional<string> json_metadata;
    optional<flat_set<account_trait>> traits;

    extensions_type extensions;

    void validate() const;

    void get_required_owner_authorities(flat_set<account_name_type>& a) const
    {
        if (owner.valid())
        {
            a.insert(account);
        }

        if (active_overrides.valid())
        {
            a.insert(account);
        }
    }

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        if (!owner.valid())
        {
            a.insert(account);
        }
    }
};

} // namespace protocol
} // namespace deip

FC_REFLECT(deip::protocol::update_account_operation, 
  (account)
  (owner)
  (active)
  (active_overrides)
  (memo_key)
  (json_metadata)
  (traits)
  (extensions)
)
