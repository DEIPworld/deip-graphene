#pragma once

#include "dbs_base_impl.hpp"

#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/deip_object_types.hpp>
#include <deip/chain/schema/research_group_invite_object.hpp>

#include <fc/shared_string.hpp>
#include <fc/fixed_string.hpp>

namespace deip {
namespace chain {

class proposal_object;

class dbs_research_group_invite : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_research_group_invite() = delete;
protected:
    explicit dbs_research_group_invite(database& db);

public:

    using research_group_invite_refs_type = std::vector<std::reference_wrapper<const research_group_invite_object>>;
    using research_group_invite_optional_ref_type = fc::optional<std::reference_wrapper<const research_group_invite_object>>;

    const research_group_invite_object& create(const account_name_type& account_name,
                                               const research_group_id_type& research_group_id,
                                               const share_type& research_group_token_amount,
                                               const std::string& cover_letter,
                                               const account_name_type& token_source,
                                               const bool& is_head);

    const research_group_invite_object& get_research_group_invite(const research_group_invite_id_type& research_group_invite_id) const;

    const research_group_invite_optional_ref_type get_research_group_invite_if_exists(const research_group_invite_id_type& research_group_invite_id) const;

    const research_group_invite_object& get_research_group_invite_by_account_and_research_group(const account_name_type& account_name, const research_group_id_type& research_group_id) const;

    const research_group_invite_optional_ref_type
    get_research_group_invite_by_account_and_research_group_if_exists(const account_name_type& account_name, const research_group_id_type& research_group_id) const;

    const bool research_group_invite_exists(const research_group_invite_id_type& research_group_invite_id) const;

    research_group_invite_refs_type get_research_group_invites_by_account_name(const account_name_type& account_name) const;

    research_group_invite_refs_type get_research_group_invites_by_research_group_id(const research_group_id_type& research_group_id) const;

    void clear_expired_invites();

    bool is_expired(const research_group_invite_object& invite);

    void remove(const research_group_invite_object& invite);
};

} // namespace chain
} // namespace deip
