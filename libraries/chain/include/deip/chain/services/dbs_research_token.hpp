#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/research_token_object.hpp>
#include <deip/chain/schema/research_object.hpp>

namespace deip {
    namespace chain {

        class dbs_research_token : public dbs_base {
            friend class dbservice_dbs_factory;

            dbs_research_token() = delete;

        protected:
            explicit dbs_research_token(database &db);

        public:

            using research_token_refs_type = std::vector<std::reference_wrapper<const research_token_object>>;
            using research_token_optional_ref_type = fc::optional<std::reference_wrapper<const research_token_object>>;

            const research_token_object& create_research_token(const account_name_type& owner,
                                                               const research_object& research,
                                                               const share_type& amount,
                                                               const bool& is_compensation);

            /* Get research token by id
             * @returns research token object by its id
            */
            const research_token_object& get(const research_token_id_type &id) const;

            const research_token_optional_ref_type get_research_token_if_exists(const research_token_id_type& id) const;

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

            const research_token_optional_ref_type get_research_token_by_owner_and_research_if_exists(const account_name_type &owner,
                                                                                                      const research_id_type &research_id) const;

            void check_existence_by_owner_and_research( const account_name_type& owner,
                                                        const research_id_type& research_id);

            const bool exists_by_owner_and_research(const account_name_type& owner,
                                                    const research_id_type& research_id) const;
        };
    } // namespace chain
} // namespace deip