#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/contract_agreement_object.hpp>

namespace deip {
namespace chain {

class dbs_contract_agreement : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_contract_agreement() = delete;

protected:
    explicit dbs_contract_agreement(database& db);

public:
    using refs_type = std::vector<std::reference_wrapper<const contract_agreement_object>>;
    using optional_ref_type = fc::optional<std::reference_wrapper<const contract_agreement_object>>;

    const contract_agreement_object& create(const external_id_type& external_id,
                                                   const account_name_type& creator,
                                                   const flat_set<account_name_type>& parties,
                                                   const std::string& hash,
                                                   const fc::time_point_sec& start_time,
                                                   const fc::optional<fc::time_point_sec>& end_time);

    const bool exists(const external_id_type& external_id) const;
    const optional_ref_type get_if_exists(const external_id_type& id) const;

    const contract_agreement_object& accept_by(const contract_agreement_object& contract,
                                               const account_name_type& party);
};

} // namespace chain
} // namespace deip
