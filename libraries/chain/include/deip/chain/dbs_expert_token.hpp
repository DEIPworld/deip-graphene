#pragma once

#include <deip/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/expert_token_object.hpp>
#include <deip/chain/account_object.hpp>

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

    /* Get expert token by id
     * @returns expert token object by its id
    */
    const expert_token_object& get_expert_token(const expert_token_id_type& id) const;

    const expert_token_object& increase_common_tokens(const account_name_type &account, const share_type& amount);

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
    bool check_expert_token_existence_by_account_and_discipline_return(const account_name_type& account,
                                                                const discipline_id_type& discipline_id);
};
} // namespace chain
} // namespace deip
