#include <deip/chain/dbs_vesting_contract.hpp>
#include <deip/chain/database.hpp>

namespace deip {
namespace chain {

dbs_vesting_contract::dbs_vesting_contract(database &db)
    : _base_type(db)
{
}

const vesting_contract_object& dbs_vesting_contract::create(const account_name_type& sender,
                                                            const account_name_type& reciever,
                                                            const share_type amount,
                                                            const share_type contract_parts,
                                                            const uint32_t contract_duration)
{
    const vesting_contract_object& new_vesting_contract = db_impl().create<vesting_contract_object>([&](vesting_contract_object& vesting_contract) {
        vesting_contract.sender = sender;
        vesting_contract.reciever = reciever;
        vesting_contract.amount = amount;
        vesting_contract.withdrawn = 0;
        vesting_contract.contract_parts = contract_parts;
        vesting_contract.start_date = db_impl().head_block_time();
        vesting_contract.expiration_date = db_impl().head_block_time() + DAYS_TO_SECONDS(contract_duration);
    });

    return new_vesting_contract;
}

const vesting_contract_object& dbs_vesting_contract::get(const vesting_contract_id_type& id)
{
    return db_impl().get<vesting_contract_object, by_id>(id);
}

const vesting_contract_object& dbs_vesting_contract::get_by_sender_and_reviever(const account_name_type& sender,
                                                                                const account_name_type& reciever)
{
    return db_impl().get<vesting_contract_object, by_sender_and_reciever>(boost::make_tuple(sender, reciever));
}

dbs_vesting_contract::vesting_contract_refs_type dbs_vesting_contract::get_by_reciever(const account_name_type& reciever)
{
    vesting_contract_refs_type ret;

    auto it_pair = db_impl().get_index<vesting_contract_index>().indicies().get<by_reciever>().equal_range(reciever);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const vesting_contract_object& dbs_vesting_contract::increase_withdrawn_amount(const vesting_contract_object& vesting_contract,
                                                                               const share_type withdraw)
{
    FC_ASSERT(vesting_contract.withdrawn + withdraw <= vesting_contract.amount, "You cant withdraw more than contract amount");
    if (vesting_contract.withdrawn + withdraw == vesting_contract.amount)
        db_impl().remove(vesting_contract);

    db_impl().modify(vesting_contract, [&](vesting_contract_object& v) { v.withdrawn += withdraw; });
}

void dbs_vesting_contract::check_vesting_contract_existence_by_sender_and_reciever(const account_name_type& sender,
                                                                                   const account_name_type& reciever)
{
    const auto& idx = db_impl().get_index<vesting_contract_index>().indices().get<by_sender_and_reciever>();

    FC_ASSERT(idx.find(boost::make_tuple(sender, reciever)) != idx.cend(), "Vesting contract with \"${1}\" sender and \"${2}\" reciever doenst exists.",
              ("1", sender)("2", reciever));
}

} //namespace chain
} //namespace deip
