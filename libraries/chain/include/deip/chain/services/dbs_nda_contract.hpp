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
    using contracts_refs_type = std::vector<std::reference_wrapper<const nda_contract_object>>;

    const nda_contract_object& create(const account_name_type& party_a,
                                  const research_group_id_type& party_a_research_group_id,
                                  const account_name_type& signee,
                                  const research_group_id_type& signee_research_group_id,
                                  const std::string& title,
                                  const std::string& contract_hash,
                                  const fc::time_point_sec& created_at,
                                  const fc::time_point_sec& start_date,
                                  const fc::time_point_sec& end_date);

    const nda_contract_object& get(const nda_contract_id_type& id) const;

    void check_contract_existence(const nda_contract_id_type& id) const;

    contracts_refs_type get_by_hash(const fc::string& hash);

    contracts_refs_type get_by_creator(const account_name_type& party_a);

    contracts_refs_type get_by_signee(const account_name_type &signee);

    contracts_refs_type get_by_creator_research_group(const research_group_id_type& research_group_id);

    contracts_refs_type get_by_signee_research_group(const research_group_id_type& research_group_id);

    contracts_refs_type get_by_creator_research_group_and_contract_hash(const research_group_id_type& research_group_id, const fc::string& hash);

    contracts_refs_type get_by_signee_research_group_and_contract_hash(const research_group_id_type& research_group_id, const fc::string& hash);

    contracts_refs_type get_by_creator_research_group_and_signee_research_group(const research_group_id_type& party_a_research_group_id, const research_group_id_type& signee_research_group_id);

    contracts_refs_type get_by_creator_research_group_and_signee_research_group_and_contract_hash(
        const research_group_id_type& party_a_research_group_id,
        const research_group_id_type& signee_research_group_id,
        const fc::string& hash);

    const nda_contract_object& sign(const nda_contract_object& contract, const account_name_type& contract_signer, const fc::string& sig);

    void set_new_contract_status(const nda_contract_object& contract, const nda_contract_status& status);
};
} // namespace chain
} // namespace deip