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
    auto& _asset_obj = db_impl().get<asset_object, by_symbol>(symbol);
    auto& account_balance = db_impl().create<account_balance_object>([&](account_balance_object& account_balance) {
        account_balance.owner = owner;
        account_balance.asset_id = _asset_obj.id;
        account_balance.symbol = symbol;
        account_balance.amount = amount;
    });

    return account_balance;
}

const account_balance_object& dbs_account_balance::get(const account_balance_id_type& id) const
{
    return db_impl().get<account_balance_object>(id);
}

bool dbs_account_balance::exists_by_owner_and_asset(const account_name_type& owner,
                                                       const protocol::asset_symbol_type& symbol) const
{
    const auto& idx = db_impl().get_index<account_balance_index>().indices().get<by_owner_and_asset>();

    if (idx.find(boost::make_tuple(owner, symbol)) != idx.cend())
        return true;
    else
        return false;
}

dbs_account_balance::account_balance_refs_type dbs_account_balance::get_by_owner(const account_name_type& owner)
{
    account_balance_refs_type ret;

    auto it_pair = db_impl().get_index<account_balance_index>().indicies().get<by_owner>().equal_range(owner);
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
    const auto& idx = db_impl().get_index<account_balance_index>().indices().get<by_owner_and_asset>();
    FC_ASSERT(idx.find(std::make_tuple(owner, symbol)) != idx.cend(), "Account balance \"${1}\" with \"${2}\" asset does not exist.", ("1", owner)("2", symbol));
}

void dbs_account_balance::remove(const account_balance_object& account_balance)
{
    db_impl().remove(account_balance);
}

const account_balance_object& dbs_account_balance::get_by_owner_and_asset(const account_name_type& owner,
                                                                          const protocol::asset_symbol_type& symbol) const
{
    return db_impl().get<account_balance_object, by_owner_and_asset>(boost::make_tuple(owner, symbol));
}

void dbs_account_balance::adjust_balance(const account_name_type& account_name, const asset& delta)
{
    if(!exists_by_owner_and_asset(account_name, delta.symbol) && delta.amount > 0)
        create(account_name, delta.symbol, 0);

    auto& balance_object = get_by_owner_and_asset(account_name, delta.symbol);

#ifdef IS_TEST_NET
    if (delta.amount < 0 && account_name != DEIP_REGISTRAR_ACCOUNT_NAME)
        FC_ASSERT(balance_object.amount >= abs(delta.amount.value), "Account doesn't have enough funds.");
#else
    if (delta.amount < 0)
        FC_ASSERT(balance_object.amount >= abs(delta.amount.value), "Account doesn't have enough funds.");
#endif

    db_impl().modify(balance_object, [&](account_balance_object& ab_o) { ab_o.amount += delta.amount; });

}

} //namespace chain
} //namespace deip