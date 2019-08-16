#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_contract.hpp>

namespace deip {
namespace chain {

dbs_contract::dbs_contract(database &db)
    : _base_type(db)
{
}

const contract_object& dbs_contract::create(const account_name_type& creator,
                                            const account_name_type& receiver,
                                            const public_key_type& creator_key,
                                            const std::string& contract_hash,
                                            const std::string& receiver_email_hash,
                                            const fc::time_point_sec& created_at,
                                            const fc::time_point_sec& start_date,
                                            const fc::time_point_sec& end_date)
{
    auto& contract = db_impl().create<contract_object>([&](contract_object& c_o) {
        c_o.creator = creator;
        c_o.receiver = receiver;
        c_o.creator_key = creator_key;
        fc::from_string(c_o.contract_hash, contract_hash);
        fc::from_string(c_o.receiver_email_hash, receiver_email_hash);
        c_o.created_at = created_at;
        c_o.start_date = start_date;
        c_o.end_date = end_date;
    });

    return contract;
}

const contract_object& dbs_contract::get(const contract_id_type& id) const
{
    try {
        return db_impl().get<contract_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

void dbs_contract::check_contract_existence(const contract_id_type& id) const
{
    const auto& contract = db_impl().find<contract_object, by_id>(id);
    FC_ASSERT(contract != nullptr, "Contract with id \"${1}\" must exist.", ("1", id));
}

dbs_contract::contracts_refs_type dbs_contract::get_by_creator(const account_name_type& creator)
{
    contracts_refs_type ret;

    auto it_pair = db_impl().get_index<contract_index>().indicies().get<by_creator>().equal_range(creator);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_contract::contracts_refs_type dbs_contract::get_by_receiver(const account_name_type& receiver)
{
    contracts_refs_type ret;

    auto it_pair = db_impl().get_index<contract_index>().indicies().get<by_receiver>().equal_range(receiver);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_contract::sign_by_receiver(const contract_object& contract,
                                    const std::string& receiver_email_hash,
                                    const public_key_type& receiver_key)
{
    db_impl().modify(contract, [&](contract_object& c_o)
    {
        c_o.receiver_key = receiver_key;
        c_o.status = contract_status::contract_signed;
    });
}

void dbs_contract::set_new_contract_status(const contract_object& contract,
                                           const contract_status& status)
{
    db_impl().modify(contract, [&](contract_object& c_o) { c_o.status = status; });
}

} //namespace chain
} //namespace deip