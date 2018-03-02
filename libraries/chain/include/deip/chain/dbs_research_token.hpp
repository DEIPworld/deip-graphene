#pragma once

#include <deip/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/research_token_object.hpp>

namespace deip {
    namespace chain {

        class dbs_research_token : public dbs_base {
            friend class dbservice_dbs_factory;

            dbs_research_token() = delete;

        protected:
            explicit dbs_research_token(database &db);

        public:

            using research_token_refs_type = std::vector<std::reference_wrapper<const research_token_object>>;

            const research_token_object& create_research_token(const account_name_type& owner,
                                                               const deip::chain::share_type amount,
                                                               const research_id_type& research_id);

            void increase_research_token_amount(const research_token_object& research_token, const share_type delta);

            void decrease_research_token_amount(const research_token_object& research_token, const share_type delta);

            /* Get research token by id
             * @returns research token object by its id
            */
            const research_token_object& get_research_token(const research_token_id_type &id) const;

            /* Get research tokens by account name
            * @returns a list of all research token objects for specific account
            */
            research_token_refs_type get_research_tokens_by_account_name(const account_name_type &account_name) const;

            /* Get research tokens by research id
             * @returns a list of all research token objects for specific discipline
            */
            research_token_refs_type get_research_tokens_by_research_id(const research_id_type &research_id) const;

            /* Get research tokens by research_id and account_name
               @returns a list of all research token objects for */

            const research_token_object& get_research_token_by_account_name_and_research_id(const account_name_type &account_name,
                                                                                         const research_id_type &research_id) const;

            bool check_research_token_existence_by_account_name_and_research_id(const account_name_type& account_name,
                                                                              const research_id_type& research_id) const;
        };
    } // namespace chain
} // namespace deip
