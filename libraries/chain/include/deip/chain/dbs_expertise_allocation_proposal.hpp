#pragma once

#include <deip/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/expertise_allocation_proposal_object.hpp>

namespace deip {
namespace chain {

class dbs_expertise_allocation_proposal : public dbs_base {
    friend class dbservice_dbs_factory;

    dbs_expertise_allocation_proposal() = delete;

protected:
    explicit dbs_expertise_allocation_proposal(database &db);

public:

    using expertise_allocation_proposal_refs_type = std::vector<std::reference_wrapper<const expertise_allocation_proposal_object>>;

    const expertise_allocation_proposal_object& create(const account_name_type& initiator,
                                                       const account_name_type& claimer,
                                                       const discipline_id_type& discipline_id);

    const expertise_allocation_proposal_object& get(const expertise_allocation_proposal_id_type& id) const;

    expertise_allocation_proposal_refs_type get_by_discipline_and_claimer(const discipline_id_type& discipline_id,
                                                                          const account_name_type& claimer) const;

    const expertise_allocation_proposal_object& get_by_discipline_initiator_and_claimer(const discipline_id_type& disicpline_id,
                                                                                        const account_name_type& initiator,
                                                                                        const account_name_type& claimer) const;

    expertise_allocation_proposal_refs_type get_by_discipline_id(const discipline_id_type& discipline_id) const;

    void check_existence_by_discipline_initiator_and_claimer(const discipline_id_type &discipline_id,
                                                             const account_name_type &initiator,
                                                             const account_name_type &claimer);

    bool is_exists_by_discipline_initiator_and_claimer(const discipline_id_type &discipline_id,
                                                       const account_name_type &initiator,
                                                       const account_name_type &claimer);

    void increase_total_voted_expertise(const expertise_allocation_proposal_object& expertise_allocation_proposal,
                                        const account_name_type &voter,
                                        const int16_t amount);

    void decrease_total_voted_expertise(const expertise_allocation_proposal_object& expertise_allocation_proposal,
                                        const account_name_type &voter,
                                        const int16_t amount);

};

} // namespace chain
} // namespace deip
