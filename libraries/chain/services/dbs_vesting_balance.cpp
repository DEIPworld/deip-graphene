#include <deip/chain/services/dbs_vesting_balance.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

dbs_vesting_balance::dbs_vesting_balance(database &db)
    : _base_type(db)
{
}

const vesting_balance_object & dbs_vesting_balance::create(const account_name_type &owner, const asset &balance,
                                                             const uint32_t &vesting_duration_seconds,
                                                             const uint32_t& period_duration_seconds,
                                                             const uint32_t &vesting_cliff_seconds)
{
    return db_impl().create<vesting_balance_object>([&](vesting_balance_object& vesting_balance) {
        vesting_balance.owner = owner;
        vesting_balance.balance = balance;
        vesting_balance.start_timestamp = db_impl().head_block_time();
        vesting_balance.vesting_duration_seconds = vesting_duration_seconds;
        vesting_balance.period_duration_seconds = period_duration_seconds;
        vesting_balance.vesting_cliff_seconds = vesting_cliff_seconds;
    });
}

const vesting_balance_object& dbs_vesting_balance::get(const vesting_balance_id_type& id) const
{
    try {
        return db_impl().get<vesting_balance_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const dbs_vesting_balance::vesting_balance_optional_ref_type
dbs_vesting_balance::get_vesting_balance_if_exists(const vesting_balance_id_type& id) const
{
    vesting_balance_optional_ref_type result;
    const auto& idx = db_impl()
            .get_index<vesting_balance_index>()
            .indicies()
            .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

dbs_vesting_balance::vesting_balance_refs_type dbs_vesting_balance::get_vesting_balance_by_owner(const account_name_type &owner) const
{
    vesting_balance_refs_type ret;

    auto it_pair = db_impl().get_index<vesting_balance_index>().indicies().get<by_owner>().equal_range(owner);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_vesting_balance::withdraw(const vesting_balance_id_type &id, const asset &amount)
{
    auto& vesting = db_impl().get<vesting_balance_object, by_id>(id);
    FC_ASSERT(vesting.balance >= amount);
    db_impl().modify(vesting, [&](vesting_balance_object& v) {
        v.balance -= amount;
        v.withdrawn += amount;
    });
}

void dbs_vesting_balance::check_existence(const vesting_balance_id_type& id)
{
    const auto& idx = db_impl().get_index<vesting_balance_index>().indices().get<by_id>();

    FC_ASSERT(idx.find(id) != idx.cend(), "Vesting contract \"${1}\" doesn't exist.",
              ("1", id));
}

} //namespace chain
} //namespace deip
