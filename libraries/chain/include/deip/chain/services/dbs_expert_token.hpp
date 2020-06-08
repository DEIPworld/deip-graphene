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
    using expert_token_optional_ref_type = fc::optional<std::reference_wrapper<const expert_token_object>>;

    const expert_token_object& create_expert_token(const account_name_type& account, 
                                                   const discipline_id_type& discipline_id,
                                                   const share_type& delta, 
                                                   const bool& create_parent);

    /* Get expert token by id
     * @returns expert token object by its id
    */
    const expert_token_object& get_expert_token(const expert_token_id_type& id) const;

    const expert_token_optional_ref_type get_expert_token_if_exists(const expert_token_id_type& id) const;

    /* Get expert token by account name & discipline
    * @returns expert token in specified discipline for account
    */
    const expert_token_object& get_expert_token_by_account_and_discipline(const account_name_type& account,
                                                                           const discipline_id_type& discipline_id) const;

    const expert_token_optional_ref_type get_expert_token_by_account_and_discipline_if_exists(const account_name_type& account,
                                                                                              const discipline_id_type& discipline_id) const;

    /* Get expert tokens by account name
    * @returns a list of all expert token objects for specific account
    */
    expert_token_refs_type get_expert_tokens_by_account_name(const account_name_type& account_name) const;

    /* Get expert tokens by discipline id
     * @returns a list of all expert token objects for specific discipline
    */
    const expert_token_refs_type get_expert_tokens_by_discipline(const discipline_id_type& discipline_id) const;

    const expert_token_refs_type get_expert_tokens_by_discipline(const external_id_type& discipline_external_id) const;

    const bool expert_token_exists_by_account_and_discipline(const account_name_type& account, const discipline_id_type& discipline_id) const;

    const std::tuple<share_type, share_type> adjust_expert_token( const account_name_type& account,
                                                                  const discipline_id_type& discipline_id,
                                                                  const share_type& amount);

};

} // namespace chain
} // namespace deip
