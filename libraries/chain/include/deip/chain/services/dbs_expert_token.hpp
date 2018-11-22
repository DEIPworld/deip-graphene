#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/expert_token_object.hpp>
#include <deip/chain/schema/account_object.hpp>

namespace deip {
    namespace chain {

///** DB service for operations with expert_token_object
// *  --------------------------------------------
// */
class dbs_expert_token : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_expert_token() = delete;

protected:
    explicit dbs_expert_token(database &db);

public:

    using expert_token_refs_type = std::vector<std::reference_wrapper<const expert_token_object>>;

    const expert_token_object& create(const account_name_type& account, const discipline_id_type& discipline_id, const share_type& amount);

    const expert_token_object& create_with_parent_discipline(const account_name_type& account, const discipline_id_type& discipline_id, const share_type& amount);

    /* Get expert token by id
     * @returns expert token object by its id
    */
    const expert_token_object& get_expert_token(const expert_token_id_type& id) const;

    /* Get expert token by account name & discipline
    * @returns expert token in specified discipline for account
    */
    const expert_token_object& get_expert_token_by_account_and_discipline(const account_name_type& account,
                                                                           const discipline_id_type& discipline_id) const;

    /* Get expert tokens by account name
    * @returns a list of all expert token objects for specific account
    */
    expert_token_refs_type get_expert_tokens_by_account_name(const account_name_type& account_name) const;

    /* Get expert tokens by discipline id
     * @returns a list of all expert token objects for specific discipline
    */
    expert_token_refs_type get_expert_tokens_by_discipline_id(const discipline_id_type& discipline_id) const;

    void check_expert_token_existence_by_account_and_discipline(const account_name_type& account,
                                                                const discipline_id_type& discipline_id);

    bool expert_token_exists_by_account_and_discipline(const account_name_type& account,
                                                       const discipline_id_type& discipline_id);

    void increase_expertise_tokens(const account_object &account,
                                   const discipline_id_type &discipline_id,
                                   const share_type &amount);

    void update_expertise_proxy(const expert_token_object& expert_token, const optional<expert_token_object>& proxy_expert_token);

    void adjust_proxied_expertise(const expert_token_object& expert_token,
                                  const std::array<share_type,DEIP_MAX_PROXY_RECURSION_DEPTH + 1>& delta,
                                  int depth = 0);

    void adjust_proxied_expertise(const expert_token_object& expert_token,
                                  share_type delta,
                                  int depth = 0);
};

} // namespace chain
} // namespace deip
