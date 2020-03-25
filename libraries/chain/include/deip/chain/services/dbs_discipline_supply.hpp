#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/schema/discipline_supply_object.hpp>

namespace deip {
namespace chain {

/** DB service for operations with discipline_supply_object
 *  --------------------------------------------
 */
class dbs_discipline_supply : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_discipline_supply() = delete;

protected:
    explicit dbs_discipline_supply(database& db);

public:
    using discipline_supply_refs_type = std::vector<std::reference_wrapper<const discipline_supply_object>>;

    /** Lists all discipline supply owners.
     *
     *  @warning limit must be less or equal than DEIP_LIMIT_DISCIPLINE_SUPPLIES_LIST_SIZE.
     *
     */
    std::set<string> lookup_discipline_supply_owners(const string &lower_bound_owner_name, uint32_t limit) const;

    /** Lists all discipline_supplies.
     *
     * @returns a list of discipline supply objects
     */
    discipline_supply_refs_type get_discipline_supplies() const;

    /** Lists all discipline supplies registered for owner.
     *
     * @param owner the name of the owner
     * @returns a list of grant objects
     */
    discipline_supply_refs_type get_discipline_supplies_by_owner(const account_name_type &owner) const;

    /** Get discipline supply by id
     */
    const discipline_supply_object& get_discipline_supply(discipline_supply_id_type id) const;

    /** Create discipline supply.
     *  The owner has abilities for all operations (for update, close and schedule operations).
     *
     * @param owner the name of the owner
     * @param balance the total balance (use DEIP_SYMBOL)
     * @param start_block the block number when start allocate discipline supply
     * @param end_block the block number when finish allocate discipline supply
     * @param target_discipline the discipline supply target discipline to allocate funds
     * @param target_research the discipline supply target research tos allocate funds
     * @returns a discipline supply object
     */
    const discipline_supply_object& create_discipline_supply(const account_object &owner,
                                                             const asset &balance,
                                                             const uint32_t &start_block,
                                                             const uint32_t &end_block,
                                                             const discipline_id_type &target_discipline,
                                                             const bool is_extendable,
                                                             const std::string &content_hash);


    /** Distribute asset from discipline supply.
     *  This operation takes into account the deadline and last block number
     *
     * @param discipline supply the discipline supply that is distributed
     */
    asset allocate_funds(const discipline_supply_object& discipline_supply);

    void clear_expired_discipline_supplies();

    bool is_expired(const discipline_supply_object& discipline_supply);

    share_type supply_researches_in_discipline(const discipline_id_type &discipline_id, const share_type &amount);

    void process_discipline_supplies();

private:
    uint64_t _get_discipline_supplies_count(const account_name_type &owner) const;
};
} // namespace chain
} // namespace deip
