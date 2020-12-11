#include <deip/chain/schema/asset_object.hpp>

#include <deip/chain/services/dbs_account_balance.hpp>

#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

using protocol::asset_symbol_type;
using deip::protocol::asset;

dbs_account_balance::dbs_account_balance(database& db)
    : _base_type(db)
{
}

const account_balance_object& dbs_account_balance::create_account_balance(const account_name_type& owner,
                                                                          const asset_symbol_type& symbol,
                                                                          const share_type& amount)
{
    const auto& asset = db_impl().get<asset_object, by_symbol>(symbol);
    const auto& account_balance = db_impl().create<account_balance_object>([&](account_balance_object& account_balance) {
        account_balance.owner = owner;
        account_balance.asset_id = asset.id;
        account_balance.symbol = symbol;
        fc::from_string(account_balance.string_symbol, fc::to_string(asset.string_symbol));
        account_balance.amount = amount;

        if (static_cast<asset_type>(asset.type) == asset_type::research_security_token)
        {
            account_balance.tokenized_research = *asset.tokenized_research;
        }
    });

    return account_balance;
}


const dbs_account_balance::account_balance_refs_type dbs_account_balance::get_account_balances_by_owner(const account_name_type& owner) const
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

const dbs_account_balance::account_balance_refs_type dbs_account_balance::get_accounts_balances_by_symbol(const asset_symbol_type& symbol) const
{
    account_balance_refs_type ret;

    auto it_pair = db_impl()
      .get_index<account_balance_index>()
      .indicies()
      .get<by_symbol>()
      .equal_range(symbol);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


const dbs_account_balance::account_balance_refs_type dbs_account_balance::get_accounts_balances_by_symbol(const string& str_symbol) const
{
    account_balance_refs_type ret;

    auto it_pair = db_impl()
      .get_index<account_balance_index>()
      .indicies()
      .get<by_string_symbol>()
      .equal_range(str_symbol, fc::strcmp_less());

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


const bool dbs_account_balance::account_balance_exists_by_owner_and_asset(const account_name_type& owner,
                                                                          const asset_symbol_type& symbol) const
{
    const auto& idx = db_impl().get_index<account_balance_index>().indices().get<by_owner_and_asset_symbol>();
    return idx.find(std::make_tuple(owner, symbol)) != idx.cend();
}

const bool dbs_account_balance::account_balance_exists_by_owner_and_asset(const account_name_type& owner,
                                                                          const string& symbol) const
{
    const auto& idx = db_impl().get_index<account_balance_index>().indices().get<by_owner_and_asset_string_symbol>();
    return idx.find(std::make_tuple(owner, symbol)) != idx.cend();
}

void dbs_account_balance::remove_account_balance(const account_balance_object& account_balance)
{
    db_impl().remove(account_balance);
}

const account_balance_object& dbs_account_balance::get_account_balance_by_owner_and_asset(const account_name_type& owner, const asset_symbol_type& symbol) const
{
    return db_impl().get<account_balance_object, by_owner_and_asset_symbol>(boost::make_tuple(owner, symbol));
}

const dbs_account_balance::account_balance_optional_ref_type dbs_account_balance::get_account_balance_by_owner_and_asset_if_exists(const account_name_type& owner, const string& symbol) const
{
    account_balance_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<account_balance_index>()
      .indicies()
      .get<by_owner_and_asset_string_symbol>();

    auto itr = idx.find(boost::make_tuple(owner, symbol));
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const dbs_account_balance::account_balance_optional_ref_type dbs_account_balance::get_account_balance_by_owner_and_asset_if_exists(const account_name_type& owner, const asset_symbol_type& symbol) const
{
    account_balance_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<account_balance_index>()
      .indicies()
      .get<by_owner_and_asset_symbol>();

    auto itr = idx.find(boost::make_tuple(owner, symbol));
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const account_balance_object& dbs_account_balance::adjust_account_balance(const account_name_type& owner, const asset& delta)
{
    if (!account_balance_exists_by_owner_and_asset(owner, delta.symbol))
    {
        create_account_balance(owner, delta.symbol, 0);
    }

    const auto& balance = get_account_balance_by_owner_and_asset(owner, delta.symbol);

    if (delta.amount < 0)
    {
        FC_ASSERT(balance.amount >= abs(delta.amount.value),
          "Account ${1} does not have enough funds to transfer ${2}",
          ("1", owner)("2", delta));
    }

    db_impl().modify(balance, [&](account_balance_object& ab_o) { ab_o.amount += delta.amount; });

    return balance;
}

const account_balance_object& dbs_account_balance::freeze_account_balance(const account_name_type& account,
                                                                          const asset& amount)
{
    const account_balance_object& balance = get_account_balance_by_owner_and_asset(account, amount.symbol);

    FC_ASSERT(balance.amount >= amount.amount, "Security token balance (${1}) for account ${2} is not enough to freeze ${3}",
        ("1", balance.to_asset())("2", account)("3", amount));

    db_impl().modify(balance, [&](account_balance_object& stb_o) {
        stb_o.amount -= amount.amount;
        stb_o.frozen_amount += amount.amount;
    });

    return balance;
}

const account_balance_object& dbs_account_balance::unfreeze_account_balance(const account_name_type& account,
                                                                            const asset& amount)
{
    const account_balance_object& balance = get_account_balance_by_owner_and_asset(account, amount.symbol);

    FC_ASSERT(balance.frozen_amount >= amount.amount, "Security token frozen balance (${1}) for account ${2} is not enough to unfreeze ${3}",
        ("1", asset(balance.frozen_amount, balance.symbol))("2", account)("3", amount));

    db_impl().modify(balance, [&](account_balance_object& stb_o) {
        stb_o.amount += amount.amount;
        stb_o.frozen_amount -= amount.amount;
    });

    return balance;
}

} //namespace chain
} //namespace deip