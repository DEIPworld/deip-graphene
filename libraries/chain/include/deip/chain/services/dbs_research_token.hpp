#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/research_token_object.hpp>

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
            const research_token_object& get(const research_token_id_type &id) const;

            /* Get research tokens by account name
            * @returns a list of all research token objects for specific account
            */
            research_token_refs_type get_by_owner(const account_name_type &owner) const;

            /* Get research tokens by research id
             * @returns a list of all research token objects for specific discipline
            */
            research_token_refs_type get_by_research(const research_id_type &research_id) const;

            /* Get research tokens by research_id and account_name
               @returns a list of all research token objects for */

            const research_token_object& get_by_owner_and_research(const account_name_type &owner,
                                                                                         const research_id_type &research_id) const;

            void check_existence_by_owner_and_research(const account_name_type& owner,
                                                                                const research_id_type& research_id);

            bool exists_by_owner_and_research(const account_name_type& owner,
                                                                          const research_id_type& research_id);
        };
    } // namespace chain
} // namespace deip
