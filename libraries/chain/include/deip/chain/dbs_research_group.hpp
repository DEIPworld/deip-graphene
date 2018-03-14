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

class proposal_object;

class dbs_research_group : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_research_group() = delete;
protected:
    explicit dbs_research_group(database& db);

public:

    using research_group_token_refs_type = std::vector<std::reference_wrapper<const research_group_token_object>>;

    /** Create research_group object.
    *
    * @returns research group object
    */
    const research_group_object& create_research_group(const string& permlink, const string& description,
                                                       const share_type quorum_percent,
                                                       const share_type tokens_amount);

    /** Get research group by id
     */
    const research_group_object& get_research_group(const research_group_id_type& id) const;


    const research_group_object& get_research_group_by_permlink(const fc::string& permlink) const;

    void change_quorum(const uint32_t quorum_percent, const research_group_id_type& research_group_id);

    void check_research_group_existence(const research_group_id_type& research_group_id) const;

    void check_research_group_existence_by_permlink(const string& permlink) const;

    const research_group_token_object& get_research_group_token_by_id(const research_group_token_id_type& id) const;

    research_group_token_refs_type get_research_group_tokens_by_account_name(const account_name_type &account_name) const;

    research_group_token_refs_type get_research_group_tokens_by_research_group_id(const research_group_id_type& research_group_id) const;

    const research_group_token_object& get_research_group_token_by_account_and_research_group_id(const account_name_type& account,
                                                                const research_group_id_type& research_group_id) const;

    const research_group_token_object& create_research_group_token(const research_group_id_type& research_group_id,
                                                                   const share_type amount,
                                                                   const account_name_type& owner);

    void remove_token(const account_name_type& account, const research_group_id_type& research_group_id);

    void check_research_group_token_existence(const account_name_type& account,
                                        const research_group_id_type& research_group_id) const;

    const research_group_object&  increase_research_group_total_tokens_amount(const research_group_id_type& research_group_id, const share_type delta);

    const research_group_object&  decrease_research_group_total_tokens_amount(const research_group_id_type& research_group_id, const share_type delta);

    const research_group_object& increase_research_group_funds(const research_group_id_type& research_group_id, const share_type deips);

    const research_group_object& decrease_research_group_funds(const research_group_id_type& research_group_id, const share_type deips);

    const research_group_token_object& increase_research_group_token_amount(const research_group_id_type& research_group_id,
                                                                            const account_name_type& account_name,
                                                                            const share_type amount);

};

} // namespace chain
} // namespace deip
