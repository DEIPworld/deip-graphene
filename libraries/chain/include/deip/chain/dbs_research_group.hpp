#pragma once

#include <deip/chain/dbs_base_impl.hpp>

#include <vector>
#include <set>
#include <functional>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/research_group_object.hpp>

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

    using research_group_token_refs_type = std::vector<std::reference_wrapper<const research_group_token_object>>;
    /** Get research group by id
     */
    const research_group_object& get_research_group(const research_group_id_type& id) const;


    const research_group_object& get_research_group_by_permlink(const fc::string& permlink) const;
    /** Create research_group object.
     *
     * @returns research group object
     */
    const research_group_object& create_research_group(const std::string& name,
                                                       const string& permlink,
                                                       const string& description,
                                                       const share_type& quorum_percent,
                                                       const std::map<uint16_t, share_type>& proposal_quorums,
                                                       const bool is_personal);

    void change_quorum(const uint32_t quorum_percent, const uint16_t proposal_type, const research_group_id_type& research_group_id);

    void check_research_group_existence(const research_group_id_type& research_group_id) const;

    const research_group_token_object& get_research_group_token_by_id(const research_group_token_id_type& id) const;

    research_group_token_refs_type get_tokens_by_account(const account_name_type &account_name) const;

    research_group_token_refs_type get_research_group_tokens(const research_group_id_type& research_group_id) const;

    const research_group_token_object& get_token_by_account_and_research_group(const account_name_type &account,
                                                                               const research_group_id_type &research_group_id) const;

    const research_group_token_object& create_research_group_token(const research_group_id_type& research_group_id,
                                                                   const share_type amount,
                                                                   const account_name_type& owner);

    void remove_token(const account_name_type& account, const research_group_id_type& research_group_id);

    void check_research_group_token_existence(const account_name_type& account,
                                        const research_group_id_type& research_group_id) const;

    const research_group_object& increase_balance(const research_group_id_type &research_group_id, const asset &deips);

    const research_group_object& decrease_balance(const research_group_id_type &research_group_id, const asset &deips);

    const share_type decrease_research_group_tokens_amount(const research_group_id_type& research_group_id,
                                                     const share_type delta);
    void increase_research_group_tokens_amount(const research_group_id_type& research_group_id,
                                                     const share_type delta);

    const research_group_token_object& set_new_research_group_token_amount(const research_group_id_type& research_group_id,
                                                                           const account_name_type& owner,
                                                                           const share_type new_amount);

    const flat_set<account_name_type> get_members(const research_group_id_type& id) const;

};

} // namespace chain
} // namespace deip
