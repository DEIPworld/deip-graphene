#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/account_balance_object.hpp>

namespace deip {
namespace chain {

class dbs_account_balance : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_account_balance() = delete;

protected:
    explicit dbs_account_balance(database &db);

public:
    using account_balance_refs_type = std::vector<std::reference_wrapper<const account_balance_object>>;

    const account_balance_object& create(const account_name_type& owner,
                                         const protocol::asset_symbol_type& symbol,
                                         const share_type& amount);

    const account_balance_object& get(const account_balance_id_type& id) const;

    bool exists_by_owner_and_asset(const account_name_type& owner,
                                      const protocol::asset_symbol_type& symbol) const;

    const account_balance_refs_type get_by_owner(const account_name_type& owner) const;

    void check_existence(const account_balance_id_type &id) const;

    void check_existence_by_owner_and_asset(const account_name_type& owner,
                                            const protocol::asset_symbol_type& symbol) const;

    void remove(const account_balance_object& asset_backed_token);

    const account_balance_object& get_by_owner_and_asset(const account_name_type& owner,
                                                         const protocol::asset_symbol_type& symbol) const;

    const account_balance_object& get_by_owner_and_asset(const account_name_type& owner,
                                                         const string& symbol) const;

    void adjust_balance(const account_name_type& account_name, const asset& delta);

};

} // namespace chain
} // namespace deip