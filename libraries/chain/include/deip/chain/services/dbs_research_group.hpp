#pragma once

#include "dbs_base_impl.hpp"

#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/deip_object_types.hpp>
#include <deip/chain/schema/research_group_object.hpp>

#include <fc/shared_string.hpp>
#include <fc/fixed_string.hpp>

namespace deip {
namespace chain {

class dbs_research_group : public dbs_base
{

  friend class dbservice_dbs_factory;
  dbs_research_group() = delete;

  protected:
    explicit dbs_research_group(database& db);

  public:
    using research_group_refs_type = std::vector<std::reference_wrapper<const research_group_object>>;
    using research_group_token_refs_type = std::vector<std::reference_wrapper<const research_group_token_object>>;
    using research_group_optional_ref_type = fc::optional<std::reference_wrapper<const research_group_object>>;
    using research_group_token_optional_ref_type = fc::optional<std::reference_wrapper<const research_group_token_object>>;

    const research_group_object& get_research_group(const research_group_id_type& id) const;

    const research_group_optional_ref_type get_research_group_if_exists(const research_group_id_type& id) const;

    const research_group_object& get_research_group_by_account(const account_name_type& account) const;

    const research_group_optional_ref_type get_research_group_by_account_if_exists(const account_name_type& account) const;

    /* [DEPRECATED] */ const research_group_object& get_research_group_by_permlink(const string& permlink) const;

    /* [DEPRECATED] */ const research_group_optional_ref_type get_research_group_by_permlink_if_exists(const string& permlink) const;

    const research_group_object& create_personal_research_group(const account_name_type& account);

    const research_group_object& create_research_group(const account_name_type& account,
                                                       const account_name_type& creator,
                                                       const string& description);

    const research_group_object& update_research_group(const research_group_object& research_group,
                                                       const string& description);

    const bool research_group_exists(const research_group_id_type& research_group_id) const;

    const bool research_group_exists(const string& permlink) const;

    const bool research_group_exists(const account_name_type& account) const;

    const research_group_token_object& get_research_group_token_by_id(const research_group_token_id_type& id) const;

    research_group_token_refs_type get_research_group_tokens_by_member(const account_name_type& member) const;

    research_group_token_refs_type get_research_group_tokens(const research_group_id_type& research_group_id) const;

    const research_group_token_object& get_research_group_token_by_member(const account_name_type& member,
                                                                          const research_group_id_type& internal_id) const;

    const research_group_token_optional_ref_type get_research_group_token_by_member_if_exists(const account_name_type& member,
                                                                                              const research_group_id_type& internal_id) const;
    
    const bool is_research_group_member(const account_name_type& member, const research_group_id_type& research_group_id) const;

    const research_group_token_object& add_member_to_research_group(const account_name_type& account,
                                                                    const research_group_id_type& research_group_id,
                                                                    const share_type& share,
                                                                    const account_name_type& inviter);

    research_group_token_refs_type remove_member_from_research_group(const account_name_type& account,
                                                                     const research_group_id_type& research_group_id);

    research_group_token_refs_type rebalance_research_group_tokens(const research_group_id_type& research_group_id,
                                                                   const std::map<account_name_type, share_type> shares);

    const std::set<account_name_type> get_research_group_members(const research_group_id_type& id) const;

    const research_group_refs_type lookup_research_groups(const research_group_id_type& lower_bound,
                                                          uint32_t limit) const;
};

} // namespace chain
} // namespace deip
