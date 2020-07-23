
#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/version.hpp>
#include <deip/protocol/asset.hpp>

namespace deip {
namespace protocol {

using deip::protocol::asset;
using deip::protocol::authority;

struct research_group_trait
{
    std::string name;
    std::string description;
    extensions_type extensions;
};

typedef fc::static_variant<
  research_group_trait
  > 
  account_trait;

struct create_account_operation : public entity_operation
{
    asset fee;
    account_name_type creator;
    account_name_type new_account_name;
    authority owner;
    authority active;
    flat_map<uint16_t, authority> active_overrides;
    public_key_type memo_key;
    optional<string> json_metadata;
    flat_set<account_trait> traits;
    extensions_type extensions;

    string entity_id() const { return "new_account_name"; }
    external_id_type get_entity_id() const { return new_account_name; }
    bool ignore_entity_id_validation() const { return is_user_account(); }

    void validate() const;
    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(creator);
    }

    bool is_research_group_account() const;
    bool is_user_account() const;
};



} // namespace protocol
} // namespace deip

FC_REFLECT(deip::protocol::create_account_operation,
  (fee)
  (creator)
  (new_account_name)
  (owner)
  (active)
  (active_overrides)
  (memo_key)
  (json_metadata)
  (traits)
  (extensions)
)

FC_REFLECT(deip::protocol::research_group_trait, (name)(description)(extensions))

DECLARE_STATIC_VARIANT_TYPE(deip::protocol::account_trait)
FC_REFLECT_TYPENAME(deip::protocol::account_trait)