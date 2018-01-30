#include <deip/chain/dbs_budget.hpp>
#include <deip/chain/database.hpp>
#include <deip/chain/dbs_account.hpp>
#include <deip/chain/account_object.hpp>

#include <deip/chain/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_budget::dbs_budget(database& db)
    : _base_type(db)
{
}

dbs_budget::budget_refs_type dbs_budget::get_budgets() const
{
    budget_refs_type ret;

    auto idx = db_impl().get_index<budget_index>().indicies();
    auto it = idx.cbegin();
    const auto it_end = idx.cend();
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

std::set<string> dbs_budget::lookup_budget_owners(const string& lower_bound_owner_name, uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_LIMIT_BUDGETS_LIST_SIZE,
              "limit must be less or equal than ${1}, actual limit value == ${2}",
              ("1", DEIP_LIMIT_BUDGETS_LIST_SIZE)("2", limit));
    const auto& budgets_by_owner_name = db_impl().get_index<budget_index>().indices().get<by_owner_name>();
    set<string> result;

    for (auto itr = budgets_by_owner_name.lower_bound(lower_bound_owner_name);
         limit-- && itr != budgets_by_owner_name.end(); ++itr)
    {
        result.insert(itr->owner);
    }

    return result;
}

dbs_budget::budget_refs_type dbs_budget::get_budgets(const account_name_type& owner) const
{
    budget_refs_type ret;

    auto it_pair = db_impl().get_index<budget_index>().indicies().get<by_owner_name>().equal_range(owner);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const budget_object& dbs_budget::get_budget(budget_id_type id) const
{
    return db_impl().get<budget_object>(id);
}

const budget_object& dbs_budget::create_grant(const account_object& owner,
                                       const asset& balance,
                                       const uint32_t& start_block, 
                                       const uint32_t& end_block,
                                       const discipline_id_type& target_discipline)
{
    // clang-format off
    FC_ASSERT(balance.symbol == DEIP_SYMBOL, "invalid asset type (symbol)");
    FC_ASSERT(balance.amount > 0, "invalid balance");
    FC_ASSERT(owner.balance >= balance, "insufficient funds");
    FC_ASSERT(_get_budgets_count(owner.name) < DEIP_LIMIT_BUDGETS_PER_OWNER,
              "can't create more then ${1} budgets per owner", ("1", DEIP_LIMIT_BUDGETS_PER_OWNER));
    // clang-format on

    const dynamic_global_property_object& props = db_impl().get_dynamic_global_properties();
    auto head_block_num = props.head_block_number;
    uint32_t start = start_block;
    if (start < head_block_num){
        start = head_block_num;
    }
    
    FC_ASSERT(start < end_block, "grant start block should be before end block");
    //Withdraw fund from account
    dbs_account& account_service = db().obtain_service<dbs_account>();
    account_service.decrease_balance(owner, balance);
    
    share_type per_block(balance.amount);
    per_block /= (end_block - start);

    FC_ASSERT(per_block >= DEIP_MIN_GRANT_PER_BLOCK,
            "We can't proceed grant that spend less than ${1} per block", ("1", DEIP_LIMIT_BUDGETS_PER_OWNER));

    const budget_object& new_budget = db_impl().create<budget_object>([&](budget_object& budget) {
        budget.owner = owner.name;
        budget.target_discipline = target_discipline;
        budget.created = props.time;
        budget.start_block = start;
        budget.end_block = end_block;
        budget.balance = balance;
        budget.per_block = per_block;
    });
    return new_budget;
}

asset dbs_budget::allocate_funds(const budget_object& budget)
{
    auto amount = asset(std::max(budget.per_block, budget.balance.amount), DEIP_SYMBOL);
    db_impl().modify(budget, [&](budget_object& b) {
        b.balance -= amount;
    });
    if (budget.balance.amount == 0){
        db_impl().remove(budget);
    }
    return amount;
}

uint64_t dbs_budget::_get_budgets_count(const account_name_type& owner) const
{
    return db_impl().get_index<budget_index>().indicies().get<by_owner_name>().count(owner);
}

} // namespace chain
} // namespace deip
