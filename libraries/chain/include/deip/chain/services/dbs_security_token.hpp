#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/security_token_object.hpp>
#include <deip/chain/schema/security_token_balance_object.hpp>
#include <deip/chain/schema/research_object.hpp>

namespace deip {
namespace chain {

    using protocol::security_token_amount_type;

    class dbs_security_token : public dbs_base
    {
        friend class dbservice_dbs_factory;

        dbs_security_token() = delete;

    protected:
        explicit dbs_security_token(database& db);

    public:
        using security_token_refs_type = std::vector<std::reference_wrapper<const security_token_object>>;
        using security_token_optional_ref_type = fc::optional<std::reference_wrapper<const security_token_object>>;
        using security_token_balance_refs_type = std::vector<std::reference_wrapper<const security_token_balance_object>>;
        using security_token_balance_optional_ref_type = fc::optional<std::reference_wrapper<const security_token_balance_object>>;

        const security_token_object& create_security_token(const research_object& research,
                                                           const external_id_type& external_id,
                                                           const uint32_t& total_amount);

        const security_token_balance_object& create_security_token_balance(const external_id_type& security_token_external_id,
                                                                          const external_id_type& research_external_id,
                                                                          const account_name_type& owner,
                                                                          const uint32_t& amount);

        void transfer_security_token(const account_name_type& from,
                                     const account_name_type& to,
                                     const external_id_type& security_token_external_id,
                                     const security_token_amount_type& amount);

        const security_token_balance_object& freeze_security_token(const account_name_type& account,
                                                                   const external_id_type& security_token_external_id,
                                                                   const security_token_amount_type& amount);

        const security_token_balance_object& unfreeze_security_token(const account_name_type& account,
                                                                     const external_id_type& security_token_external_id,
                                                                     const security_token_amount_type& amount);

        const security_token_object& get_security_token(const external_id_type& external_id) const;

        const security_token_optional_ref_type get_security_token_if_exists(const external_id_type& external_id) const;

        const security_token_refs_type get_security_tokens_by_research(const external_id_type& research_external_id) const;


        const security_token_balance_object& get_security_token_balance(const account_name_type& owner, const external_id_type& security_token_external_id) const;

        const security_token_balance_optional_ref_type get_security_token_balance_if_exists(const account_name_type& owner, const external_id_type& security_token_external_id) const;

        const security_token_balance_refs_type get_security_token_balances(const external_id_type& security_token_external_id) const;

        const security_token_balance_refs_type get_security_token_balances_by_owner(const account_name_type& owner) const;

        const security_token_balance_refs_type get_security_token_balances_by_research(const external_id_type& research_external_id) const;

        const security_token_balance_refs_type get_security_token_balances_by_owner_and_research(const account_name_type& owner, const external_id_type& research_external_id) const;

    };
} // namespace chain
} // namespace deip