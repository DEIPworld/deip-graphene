
#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/version.hpp>
#include <deip/protocol/asset.hpp>

namespace deip {
namespace protocol {

using deip::protocol::asset;
using deip::protocol::authority;

struct base_account_trait
{
    std::string _v;
    base_account_trait(string v = "1.0.0")
    {
        _v = v;
    }
};

struct research_group_v1_0_0_trait : base_account_trait
{
    research_group_v1_0_0_trait(string v = "1.0.0")
        : base_account_trait(v)
    {
    }

    std::string name;
    std::string description;
    std::map<uint16_t, authority> threshold_overrides;
};

typedef fc::static_variant<
  research_group_v1_0_0_trait
  >
  account_trait;


struct create_account_operation : public entity_operation
{
    asset fee;
    account_name_type creator;
    account_name_type new_account_name;
    authority owner;
    authority active;
    authority posting;
    public_key_type memo_key;
    optional<string> json_metadata;
    vector<account_trait> traits;
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

#define DECLARE_ACCOUNT_TRAIT_TYPE(AccountTraitType)                                                                   \
    namespace fc {                                                                                                     \
                                                                                                                       \
    void to_variant(const AccountTraitType&, fc::variant&);                                                            \
    void from_variant(const fc::variant&, AccountTraitType&);                                                          \
                                                                                                                       \
    } /* fc */

namespace fc {
using namespace deip::protocol;

std::string account_trait_name_from_type(const std::string& type_name);

struct from_account_trait_type
{
    variant& var;
    from_account_trait_type(variant& dv)
        : var(dv)
    {
    }

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
        auto name = account_trait_name_from_type(fc::get_typename<T>::name());
        var = variant(std::make_pair(name, v));
    }
};

struct get_account_trait_type
{
    string& name;
    get_account_trait_type(string& dv)
        : name(dv)
    {
    }

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
        name = account_trait_name_from_type(fc::get_typename<T>::name());
    }
};
} // namespace fc

#define DEFINE_ACCOUNT_TRAIT_TYPE(AccountTraitType)                                                                    \
    namespace fc {                                                                                                     \
                                                                                                                       \
    void to_variant(const AccountTraitType& var, fc::variant& vo)                                                      \
    {                                                                                                                  \
        var.visit(from_account_trait_type(vo));                                                                        \
    }                                                                                                                  \
                                                                                                                       \
    void from_variant(const fc::variant& var, AccountTraitType& vo)                                                    \
    {                                                                                                                  \
        static std::map<string, uint32_t> to_tag = []() {                                                              \
            std::map<string, uint32_t> name_map;                                                                       \
            for (int i = 0; i < AccountTraitType::count(); ++i)                                                        \
            {                                                                                                          \
                AccountTraitType tmp;                                                                                  \
                tmp.set_which(i);                                                                                      \
                string n;                                                                                              \
                tmp.visit(get_account_trait_type(n));                                                                  \
                name_map[n] = i;                                                                                       \
            }                                                                                                          \
            return name_map;                                                                                           \
        }();                                                                                                           \
                                                                                                                       \
        auto ar = var.get_array();                                                                                     \
        if (ar.size() < 2)                                                                                             \
            return;                                                                                                    \
        if (ar[0].is_uint64())                                                                                         \
            vo.set_which(ar[0].as_uint64());                                                                           \
        else                                                                                                           \
        {                                                                                                              \
            auto itr = to_tag.find(ar[0].as_string());                                                                 \
            FC_ASSERT(itr != to_tag.end(), "Invalid grant contract type: ${n}", ("n", ar[0]));                         \
            vo.set_which(to_tag[ar[0].as_string()]);                                                                   \
        }                                                                                                              \
        vo.visit(fc::to_static_variant(ar[1]));                                                                        \
    }                                                                                                                  \
    } /* fc */


FC_REFLECT(deip::protocol::create_account_operation,
  (fee)
  (creator)
  (new_account_name)
  (owner)
  (active)
  (posting)
  (memo_key)
  (json_metadata)
  (traits)
  (extensions)
)

FC_REFLECT(deip::protocol::base_account_trait, (_v))
FC_REFLECT_DERIVED(deip::protocol::research_group_v1_0_0_trait, (deip::protocol::base_account_trait),
  (name)
  (description)
  (threshold_overrides)
)

DECLARE_ACCOUNT_TRAIT_TYPE(deip::protocol::account_trait)
FC_REFLECT_TYPENAME(deip::protocol::account_trait)

