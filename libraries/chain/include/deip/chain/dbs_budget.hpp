#pragma once

#include <deip/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <deip/chain/budget_objects.hpp>

namespace deip {
namespace chain {

/** DB service for operations with budget_object
 *  --------------------------------------------
 */
class dbs_budget : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_budget() = delete;

protected:
    explicit dbs_budget(database& db);

public:
    using budget_refs_type = std::vector<std::reference_wrapper<const budget_object>>;

    /** Lists all budget owners.
     *
     *  @warning limit must be less or equal than DEIP_LIMIT_BUDGETS_LIST_SIZE.
     *
     */
    std::set<string> lookup_budget_owners(const string& lower_bound_owner_name, uint32_t limit) const;

    /** Lists all budgets.
     *
     * @returns a list of budget objects
     */
    budget_refs_type get_budgets() const;

    /** Lists all budgets registered for owner.
     *
     * @param owner the name of the owner
     * @returns a list of budget objects
     */
    budget_refs_type get_budgets(const account_name_type& owner) const;

    /** Get budget by id
     */
    const budget_object& get_budget(budget_id_type id) const;

    /** Create budget.
     *  The owner has abilities for all operations (for update, close and schedule operations).
     *
     * @param owner the name of the owner
     * @param balance the total balance (use DEIP_SYMBOL)
     * @param start_block the block number when start allocate grant
     * @param end_block the block number when finish allocate grant
     * @param target_discipline the budget target discipline to allocate funds
     * @param target_research the budget target research tos allocate funds 
     * @returns a budget object
     */
    const budget_object& create_grant(const account_object& owner,
                                       const asset& balance,
                                       const uint32_t& start_block, 
                                       const uint32_t& end_block,
                                       const discipline_id_type& target_discipline);


    /** Distribute asset from budget.
     *  This operation takes into account the deadline and last block number
     *
     * @param budget the budget that is distributed
     */
    asset allocate_funds(const budget_object& budget);

private:
    uint64_t _get_budgets_count(const account_name_type& owner) const;
};
} // namespace chain
} // namespace deip
