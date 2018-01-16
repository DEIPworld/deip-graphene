#pragma once

#include <deip/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/expert_token_object.hpp>

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

    /* Get expert token by id
     * @returns expert token object by its id
    */
    const expert_token_object& get_expert_token(const expert_token_id_type id) const;

    /* Get expert tokens by account id
    * @returns a list of all expert token objects for specific account
    */
    expert_token_refs_type get_expert_tokens_by_account_id(const account_id_type account_id) const;

    /* Get expert tokens by discipline id 
     * @returns a list of all expert token objects for specific discipline
    */
    expert_token_refs_type get_expert_tokens_by_discipline_id(const discipline_id_type discipline_id) const;

};
} // namespace chain
} // namespace deip
