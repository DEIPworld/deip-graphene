#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/security_token_object.hpp>
#include <deip/chain/schema/research_object.hpp>

namespace deip {
namespace chain {

    class dbs_security_token : public dbs_base
    {
        friend class dbservice_dbs_factory;

        dbs_security_token() = delete;

    protected:
        explicit dbs_security_token(database& db);

    public:
        using security_token_refs_type = std::vector<std::reference_wrapper<const security_token_object>>;
        using security_token_optional_ref_type = fc::optional<std::reference_wrapper<const security_token_object>>;

        const security_token_object& create_security_token(const research_object& research,
                                                           const external_id_type& security_token_external_id,
                                                           const uint32_t& amount);

        void transfer_security_token(const account_name_type& from,
                                     const account_name_type& to,
                                     const external_id_type& security_token_external_id,
                                     const uint32_t& amount);

        const security_token_object& get_security_token(const security_token_id_type& id) const;

        const security_token_refs_type get_security_tokens(const external_id_type& security_token_external_id) const;

        const security_token_refs_type get_security_tokens_by_owner(const account_name_type& owner) const;

        const security_token_refs_type get_security_tokens_by_research(const external_id_type& research_external_id) const;

        const security_token_object& get_security_token_by_owner(const account_name_type& owner, const external_id_type& security_token_external_id) const;

        const security_token_optional_ref_type get_security_token_by_owner_if_exists(const account_name_type& owner, const external_id_type& security_token_external_id) const;

        const bool security_token_exists_by_owner(const account_name_type& owner, const external_id_type& security_token_external_id) const;
    };
} // namespace chain
} // namespace deip