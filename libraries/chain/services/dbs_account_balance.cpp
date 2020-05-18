#include <deip/chain/schema/asset_object.hpp>

#include <deip/chain/services/dbs_account_balance.hpp>

#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

dbs_account_balance::dbs_account_balance(database &db)
    : _base_type(db)
{
}

const account_balance_object& dbs_account_balance::create(const account_name_type& owner,
                                                          const protocol::asset_symbol_type& symbol,
                                                          const share_type& amount)
{
    const auto& asset = db_impl().get<asset_object, by_symbol>(symbol);
    const auto& account_balance = db_impl().create<account_balance_object>([&](account_balance_object& account_balance) {
        account_balance.owner = owner;
        account_balance.asset_id = asset.id;
        account_balance.symbol = symbol;
        fc::from_string(account_balance.string_symbol, fc::to_string(asset.string_symbol));
        account_balance.amount = amount;
    });

    return account_balance;
}

const account_balance_object& dbs_account_balance::get(const account_balance_id_type& id) const
{
    return db_impl().get<account_balance_object>(id);
}

bool dbs_account_balance::exists_by_owner_and_asset(const account_name_type& owner, const protocol::asset_symbol_type& symbol) const
{
    const auto& idx = db_impl().get_index<account_balance_index>().indices().get<by_owner_and_asset_symbol>();
    return idx.find(boost::make_tuple(owner, symbol)) != idx.cend();
}

const dbs_account_balance::account_balance_refs_type dbs_account_balance::get_by_owner(const account_name_type& owner) const
{
    account_balance_refs_type ret;

    auto it_pair = db_impl()
      .get_index<account_balance_index>()
      .indicies()
      .get<by_owner>()
      .equal_range(owner);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_account_balance::check_existence(const account_balance_id_type &id) const
{
    const auto& idx = db_impl().get_index<account_balance_index>().indices().get<by_id>();
    FC_ASSERT(idx.find(id) != idx.cend(), "Account balance \"${1}\" does not exist.", ("1", id));
}

void dbs_account_balance::check_existence_by_owner_and_asset(const account_name_type& owner,
                                                             const protocol::asset_symbol_type& symbol) const
{
    const auto& idx = db_impl().get_index<account_balance_index>().indices().get<by_owner_and_asset_symbol>();
    FC_ASSERT(idx.find(std::make_tuple(owner, symbol)) != idx.cend(), "Account balance \"${1}\" with \"${2}\" asset does not exist.", ("1", owner)("2", symbol));
}

void dbs_account_balance::remove(const account_balance_object& account_balance)
{
    db_impl().remove(account_balance);
}

const account_balance_object& dbs_account_balance::get_by_owner_and_asset(const account_name_type& owner,
                                                                          const protocol::asset_symbol_type& symbol) const
{
    return db_impl().get<account_balance_object, by_owner_and_asset_symbol>(boost::make_tuple(owner, symbol));
}

const account_balance_object& dbs_account_balance::get_by_owner_and_asset(const account_name_type& owner,
                                                                          const string& symbol) const
{
    return db_impl().get<account_balance_object, by_owner_and_asset_string_symbol>(boost::make_tuple(owner, symbol));
}


void dbs_account_balance::adjust_balance(const account_name_type& account_name, const asset& delta)
{
    if (!exists_by_owner_and_asset(account_name, delta.symbol))
    {
        create(account_name, delta.symbol, 0);
    }

    const auto& balance = get_by_owner_and_asset(account_name, delta.symbol);

    if (delta.amount < 0)
    {
        FC_ASSERT(balance.amount >= abs(delta.amount.value),
          "Account ${1} does not have enough funds to transfer ${2}",
          ("1", account_name)("2", delta));
    }

    db_impl().modify(balance, [&](account_balance_object& ab_o) { ab_o.amount += delta.amount; });
}

} //namespace chain
} //namespace deip