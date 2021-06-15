
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


struct authority_update_extension
{
    flat_map<account_name_type, weight_type>                 active_accounts_to_add;
    flat_set<account_name_type>                              active_accounts_to_remove;

    flat_map<account_name_type, weight_type>                 owner_accounts_to_add;
    flat_set<account_name_type>                              owner_accounts_to_remove;

    flat_map<public_key_type, weight_type>                   active_keys_to_add;
    flat_set<public_key_type>                                active_keys_to_remove;

    flat_map<public_key_type, weight_type>                   owner_keys_to_add;
    flat_set<public_key_type>                                owner_keys_to_remove;
};

typedef fc::static_variant<
  authority_update_extension
  > 
  update_account_extension;


struct update_account_operation : public base_operation
{
    account_name_type account;
    optional<authority> owner;
    optional<authority> active;
    optional<flat_map<uint16_t, optional<authority>>> active_overrides;
    optional<public_key_type> memo_key;
    optional<string> json_metadata;
    optional<flat_set<account_trait>> traits;

    flat_set<update_account_extension> update_extensions;

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
  (update_extensions)
)


FC_REFLECT(deip::protocol::authority_update_extension, 
  (active_accounts_to_add)
  (active_accounts_to_remove)
  (owner_accounts_to_add)
  (owner_accounts_to_remove)
  (active_keys_to_add)
  (active_keys_to_remove)
  (owner_keys_to_add)
  (owner_keys_to_remove)
)

DECLARE_STATIC_VARIANT_TYPE(deip::protocol::update_account_extension)
FC_REFLECT_TYPENAME(deip::protocol::update_account_extension)