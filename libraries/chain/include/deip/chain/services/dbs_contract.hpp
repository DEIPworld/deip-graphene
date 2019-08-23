#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/contract_object.hpp>

namespace deip {
namespace chain {

class dbs_contract : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_contract() = delete;

protected:
    explicit dbs_contract(database &db);

public:
    using contracts_refs_type = std::vector<std::reference_wrapper<const contract_object>>;

    const contract_object& create(const account_name_type& creator,
                                  const account_name_type& receiver,
                                  const public_key_type& creator_key,
                                  const std::string& contract_hash,
                                  const fc::time_point_sec& created_at,
                                  const fc::time_point_sec& start_date,
                                  const fc::time_point_sec& end_date);

    const contract_object& get(const contract_id_type& id) const;

    void check_contract_existence(const contract_id_type& id) const;

    contracts_refs_type get_by_creator(const account_name_type& creator);

    contracts_refs_type get_by_receiver(const account_name_type& receiver);

    void sign_by_receiver(const contract_object& contract,
                          const public_key_type& receiver_key);

    void set_new_contract_status(const contract_object& contract,
                                 const contract_status& status);

};
} // namespace chain
} // namespace deip