#pragma once

#include "dbs_base_impl.hpp"

#include <deip/chain/schema/deip_object_types.hpp>
#include <deip/chain/schema/vesting_balance_object.hpp>


namespace deip {
namespace chain {


class dbs_vesting_balance : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_vesting_balance() = delete;
protected:
    explicit dbs_vesting_balance(database& db);

public:

    using vesting_balance_refs_type = std::vector<std::reference_wrapper<const vesting_balance_object>>;

    const vesting_balance_object& create(const account_name_type &owner, const asset &balance,
                                              const uint32_t &vesting_duration_seconds, const uint32_t& period_duration_seconds,
                                              const uint32_t &vesting_cliff_seconds);

    const vesting_balance_object& get(const vesting_balance_id_type& id);

    vesting_balance_refs_type get_by_owner(const account_name_type &owner);

    void withdraw(const vesting_balance_id_type &id, const asset &amount);

    void check_existence(const vesting_balance_id_type& id);
};

} // namespace chain
} // namespace deip
