#pragma once
#include <deip/protocol/base.hpp>

namespace deip {
namespace protocol {

struct authority_transfer_extension
{
    account_name_type account;
};

typedef fc::static_variant<
  authority_transfer_extension
  > 
  update_research_extension;

struct update_research_operation : public base_operation
{
    account_name_type account;
    external_id_type external_id;
    optional<string> description;
    optional<bool> is_private;
    optional<percent> review_share; // deprecated
    optional<percent> compensation_share; // deprecated
    optional<flat_set<account_name_type>> members; // deprecated

    flat_set<update_research_extension> update_extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(account);
    }
};


}
}

FC_REFLECT(deip::protocol::update_research_operation,
  (account)
  (external_id)
  (description)
  (is_private)
  (review_share) // deprecated
  (compensation_share) // deprecated
  (members) // deprecated
  (update_extensions)
)


FC_REFLECT(deip::protocol::authority_transfer_extension, 
  (account)
)

DECLARE_STATIC_VARIANT_TYPE(deip::protocol::update_research_extension)
FC_REFLECT_TYPENAME(deip::protocol::update_research_extension)