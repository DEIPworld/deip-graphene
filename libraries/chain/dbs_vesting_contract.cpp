#include <deip/chain/dbs_vesting_contract.hpp>
#include <deip/chain/database.hpp>

namespace deip {
namespace chain {

dbs_vesting_contract::dbs_vesting_contract(database &db)
    : _base_type(db)
{
}

const vesting_contract_object & dbs_vesting_contract::create(const account_name_type &creator, const account_name_type &owner, const asset &balance,
                                                             const uint32_t &vesting_duration_seconds,
                                                             const uint32_t& period_duration_seconds,
                                                             const uint32_t &vesting_cliff_seconds)
{
    return db_impl().create<vesting_contract_object>([&](vesting_contract_object& vesting_contract) {
        vesting_contract.creator = creator;
        vesting_contract.owner = owner;
        vesting_contract.balance = balance;
        vesting_contract.start_timestamp = db_impl().head_block_time();
        vesting_contract.vesting_duration_seconds = vesting_duration_seconds;
        vesting_contract.period_duration_seconds = period_duration_seconds;
        vesting_contract.vesting_cliff_seconds = vesting_cliff_seconds;
    });
}

const vesting_contract_object& dbs_vesting_contract::get(const vesting_contract_id_type& id)
{
    try {
        return db_impl().get<vesting_contract_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const vesting_contract_object& dbs_vesting_contract::get_by_creator_and_owner(const account_name_type &creator,
                                                                              const account_name_type &owner)
{
    try {
        return db_impl().get<vesting_contract_object, by_creator_and_owner>(boost::make_tuple(creator, owner));
    }
    FC_CAPTURE_AND_RETHROW((creator)(owner))
}

dbs_vesting_contract::vesting_contract_refs_type dbs_vesting_contract::get_by_owner(const account_name_type &owner)
{
    vesting_contract_refs_type ret;

    auto it_pair = db_impl().get_index<vesting_contract_index>().indicies().get<by_owner>().equal_range(owner);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_vesting_contract::withdraw(const vesting_contract_id_type &id,
                                    const asset &amount)
{
    auto& vesting = db_impl().get<vesting_contract_object, by_id>(id);
    FC_ASSERT(vesting.balance >= amount);
    db_impl().modify(vesting, [&](vesting_contract_object& v) { v.balance -= amount; });
}

void dbs_vesting_contract::check_existence_by_creator_and_owner(const account_name_type &creator,
                                                                const account_name_type &owner)
{
    const auto& idx = db_impl().get_index<vesting_contract_index>().indices().get<by_creator_and_owner>();

    FC_ASSERT(idx.find(boost::make_tuple(creator, owner)) != idx.cend(), "Vesting contract with \"${1}\" creator and \"${2}\" owner doenst exists.",
              ("1", creator)("2", owner));
}

} //namespace chain
} //namespace deip
