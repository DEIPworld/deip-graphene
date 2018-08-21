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

    const vesting_contract_object& create(const account_name_type &creator, const account_name_type &owner, const asset &balance,
                                              const uint32_t &vesting_duration_seconds, const uint32_t& period_duration_seconds,
                                              const uint32_t &vesting_cliff_seconds);

    const vesting_contract_object& get(const vesting_contract_id_type& id);

    const vesting_contract_object& get_by_creator_and_owner(const account_name_type &creator,
                                                            const account_name_type &owner);

    vesting_contract_refs_type get_by_owner(const account_name_type &owner);

    void withdraw(const vesting_contract_id_type &id, const asset &amount);

    void check_existence_by_creator_and_owner(const account_name_type &creator,
                                              const account_name_type &owner);
};

} // namespace chain
} // namespace deip
