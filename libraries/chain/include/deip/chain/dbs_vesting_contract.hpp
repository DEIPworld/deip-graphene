#pragma once

#include <deip/chain/dbs_base_impl.hpp>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/vesting_contract_object.hpp>


namespace deip {
namespace chain {


class dbs_vesting_contract : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_vesting_contract() = delete;
protected:
    explicit dbs_vesting_contract(database& db);

public:

    using vesting_contract_refs_type = std::vector<std::reference_wrapper<const vesting_contract_object>>;

    const vesting_contract_object& create(const account_name_type &sender,
                                          const account_name_type &receiver,
                                          const asset& balance,
                                          const uint32_t withdrawal_periods,
                                          const uint32_t contract_duration);

    const vesting_contract_object& get(const vesting_contract_id_type& id);

    const vesting_contract_object& get_by_sender_and_reviever(const account_name_type& sender,
                                                              const account_name_type& receiver);

    vesting_contract_refs_type get_by_receiver(const account_name_type& receiver);

    const vesting_contract_object& withdraw(const vesting_contract_id_type& id,
                                            const asset& to_withdraw);

    void check_vesting_contract_existence_by_sender_and_receiver(const account_name_type& sender,
                                                                 const account_name_type& receiver);
};

} // namespace chain
} // namespace deip
