#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/account_balance_object.hpp>

namespace deip {
namespace chain {

using protocol::asset_symbol_type;

class dbs_account_balance : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_account_balance() = delete;

protected:
    explicit dbs_account_balance(database &db);

public:
    using account_balance_refs_type = std::vector<std::reference_wrapper<const account_balance_object>>;
    using account_balance_optional_ref_type = fc::optional<std::reference_wrapper<const account_balance_object>>;

    const account_balance_object& create_account_balance(const account_name_type& owner,
                                                         const protocol::asset_symbol_type& symbol,
                                                         const share_type& amount);

    const bool account_balance_exists_by_owner_and_asset(const account_name_type& owner, const asset_symbol_type& symbol) const;

    const bool account_balance_exists_by_owner_and_asset(const account_name_type& owner, const string& symbol) const;

    const account_balance_refs_type get_account_balances_by_owner(const account_name_type& owner) const;

    const account_balance_object& get_account_balance_by_owner_and_asset(const account_name_type& owner, const asset_symbol_type& symbol);

    const account_balance_optional_ref_type get_account_balance_by_owner_and_asset_if_exists(const account_name_type& owner,
                                                                                             const asset_symbol_type& symbol) const;
                                                                                             
    const account_balance_optional_ref_type get_account_balance_by_owner_and_asset_if_exists(const account_name_type& owner,
                                                                                             const string& str_symbol) const;

    const account_balance_refs_type get_accounts_balances_by_symbol(const asset_symbol_type& symbol) const;

    const account_balance_refs_type get_accounts_balances_by_symbol(const string& str_symbol) const;

    const account_balance_object& adjust_account_balance(const account_name_type& owner, const asset& delta);

    const account_balance_object& freeze_account_balance(const account_name_type& account, const asset& amount);

    const account_balance_object& unfreeze_account_balance(const account_name_type& account, const asset& amount);

    void remove_account_balance(const account_balance_object& balance);
};

} // namespace chain
} // namespace deip