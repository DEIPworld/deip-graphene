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
                                                            const share_type amount,
                                                            const uint32_t withdrawal_periods,
                                                            const uint32_t contract_duration)
{
    const vesting_contract_object& new_vesting_contract = db_impl().create<vesting_contract_object>([&](vesting_contract_object& vesting_contract) {
        vesting_contract.sender = sender;
        vesting_contract.receiver = receiver;
        vesting_contract.amount = amount;
        vesting_contract.withdrawn = 0;
        vesting_contract.withdrawal_period = withdrawal_periods;
        vesting_contract.start_date = db_impl().head_block_time();
        vesting_contract.expiration_date = db_impl().head_block_time() + contract_duration;
        vesting_contract.contract_duration = fc::time_point_sec(contract_duration);
    });

    return new_vesting_contract;
}

const vesting_contract_object& dbs_vesting_contract::get(const vesting_contract_id_type& id)
{
    return db_impl().get<vesting_contract_object, by_id>(id);
}

const vesting_contract_object& dbs_vesting_contract::get_by_sender_and_reviever(const account_name_type& sender,
                                                                                const account_name_type& receiver)
{
    return db_impl().get<vesting_contract_object, by_sender_and_receiver>(boost::make_tuple(sender, receiver));
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

const vesting_contract_object& dbs_vesting_contract::withdraw(const vesting_contract_object &vesting_contract,
                                                              const share_type to_withdraw)
{
    FC_ASSERT(vesting_contract.withdrawn + to_withdraw <= vesting_contract.amount, "You cant withdraw more than contract amount");
    if (vesting_contract.withdrawn + to_withdraw == vesting_contract.amount) {
        db_impl().remove(vesting_contract);
    }

    db_impl().modify(vesting_contract, [&](vesting_contract_object& v) { v.withdrawn += to_withdraw;
                                                                         v.amount -= to_withdraw; });
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
