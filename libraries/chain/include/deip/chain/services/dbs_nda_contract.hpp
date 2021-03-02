#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/nda_contract_object.hpp>

namespace deip {
namespace chain {

class dbs_nda_contract : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_nda_contract() = delete;

protected:
    explicit dbs_nda_contract(database& db);

public:
    using nda_contract_refs_type = std::vector<std::reference_wrapper<const nda_contract_object>>;
    using nda_contract_optional_ref_type = fc::optional<std::reference_wrapper<const nda_contract_object>>;

    const nda_contract_object& create_research_nda(const external_id_type& external_id,
                                                   const account_name_type& creator,
                                                   const std::set<account_name_type>& parties,
                                                   const std::string& description,
                                                   const external_id_type& research_external_id,
                                                   const fc::time_point_sec& start_time,
                                                   const fc::time_point_sec& end_time);

    const bool research_nda_exists(const external_id_type& external_id) const;

    const nda_contract_optional_ref_type get_research_nda_if_exists(const external_id_type& external_id) const;

    const nda_contract_object& get_research_nda(const external_id_type& external_id) const;

    const nda_contract_refs_type get_research_nda_by_hash(const fc::string& hash) const;

    const nda_contract_refs_type get_research_nda_by_creator(const account_name_type& creator) const;

    const nda_contract_refs_type get_research_nda_by_research(const external_id_type& research_external_id) const;

    void process_nda_contracts();
};
} // namespace chain
} // namespace deip