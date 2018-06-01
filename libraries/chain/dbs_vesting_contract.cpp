#include <deip/chain/dbs_vesting_contract.hpp>
#include <deip/chain/database.hpp>

namespace deip {
namespace chain {

dbs_vesting_contract::dbs_vesting_contract(database &db)
    : _base_type(db)
{
}

const vesting_contract_object& dbs_vesting_contract::create(const account_name_type &sender,
                                                            const account_name_type &receiver,
                                                            const asset& balance,
                                                            const uint32_t withdrawal_periods,
                                                            const uint32_t contract_duration)
{
    const vesting_contract_object& new_vesting_contract = db_impl().create<vesting_contract_object>([&](vesting_contract_object& vesting_contract) {
        vesting_contract.sender = sender;
        vesting_contract.receiver = receiver;
        vesting_contract.balance = balance;
        vesting_contract.withdrawn = 0;
        vesting_contract.withdrawal_periods = withdrawal_periods;
        vesting_contract.start_date = db_impl().head_block_time();
        vesting_contract.expiration_date = db_impl().head_block_time() + contract_duration;
        vesting_contract.contract_duration = fc::time_point_sec(contract_duration);
    });

    return new_vesting_contract;
}

const vesting_contract_object& dbs_vesting_contract::get(const vesting_contract_id_type& id)
{
    try {
        return db_impl().get<vesting_contract_object, by_id>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const vesting_contract_object& dbs_vesting_contract::get_by_sender_and_reviever(const account_name_type& sender,
                                                                                const account_name_type& receiver)
{
    try {
        return db_impl().get<vesting_contract_object, by_sender_and_receiver>(boost::make_tuple(sender, receiver));
    }
    FC_CAPTURE_AND_RETHROW((sender)(receiver))
}

dbs_vesting_contract::vesting_contract_refs_type dbs_vesting_contract::get_by_receiver(const account_name_type& receiver)
{
    vesting_contract_refs_type ret;

    auto it_pair = db_impl().get_index<vesting_contract_index>().indicies().get<by_receiver>().equal_range(receiver);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_vesting_contract::withdraw(const vesting_contract_id_type& id,
                                                              const asset& to_withdraw)
{
    auto& vesting_contract = db_impl().get<vesting_contract_object, by_id>(id);

    FC_ASSERT(to_withdraw.amount <= vesting_contract.balance.amount, "You cant withdraw more than contract balance");

    if (to_withdraw.amount == vesting_contract.balance.amount)
        db_impl().remove(vesting_contract);
    else
        db_impl().modify(vesting_contract, [&](vesting_contract_object& v) { v.withdrawn += to_withdraw.amount;
                                                                             v.balance -= to_withdraw; });
}

void dbs_vesting_contract::check_vesting_contract_existence_by_sender_and_receiver(const account_name_type& sender,
                                                                                   const account_name_type& receiver)
{
    const auto& idx = db_impl().get_index<vesting_contract_index>().indices().get<by_sender_and_receiver>();

    FC_ASSERT(idx.find(boost::make_tuple(sender, receiver)) != idx.cend(), "Vesting contract with \"${1}\" sender and \"${2}\" receiver doenst exists.",
              ("1", sender)("2", receiver));
}

} //namespace chain
} //namespace deip
