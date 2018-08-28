#pragma once

#include "dbs_base_impl.hpp"
#include <vector>
#include <set>
#include <functional>

#include "../schema/grant_objects.hpp"

namespace deip {
namespace chain {

/** DB service for operations with grant_object
 *  --------------------------------------------
 */
class dbs_grant : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_grant() = delete;

protected:
    explicit dbs_grant(database& db);

public:
    using grant_refs_type = std::vector<std::reference_wrapper<const grant_object>>;

    /** Lists all grant owners.
     *
     *  @warning limit must be less or equal than DEIP_LIMIT_GRANTS_LIST_SIZE.
     *
     */
    std::set<string> lookup_grant_owners(const string& lower_bound_owner_name, uint32_t limit) const;

    /** Lists all grants.
     *
     * @returns a list of grant objects
     */
    grant_refs_type get_grants() const;

    /** Lists all grants registered for owner.
     *
     * @param owner the name of the owner
     * @returns a list of grant objects
     */
    grant_refs_type get_grants(const account_name_type& owner) const;

    /** Get grant by id
     */
    const grant_object& get_grant(grant_id_type id) const;

    /** Create grant.
     *  The owner has abilities for all operations (for update, close and schedule operations).
     *
     * @param owner the name of the owner
     * @param balance the total balance (use DEIP_SYMBOL)
     * @param start_block the block number when start allocate grant
     * @param end_block the block number when finish allocate grant
     * @param target_discipline the grant target discipline to allocate funds
     * @param target_research the grant target research tos allocate funds 
     * @returns a grant object
     */
    const grant_object& create_grant(const account_object& owner,
                                     const asset& balance,
                                     const uint32_t& start_block,
                                     const uint32_t& end_block,
                                     const discipline_id_type& target_discipline,
                                     const bool is_extendable,
                                     const std::string& content_hash);


    /** Distribute asset from grant.
     *  This operation takes into account the deadline and last block number
     *
     * @param grant the grant that is distributed
     */
    asset allocate_funds(const grant_object& grant);

    void clear_expired_grants();

    bool is_expired(const grant_object& grant);

private:
    uint64_t _get_grants_count(const account_name_type& owner) const;
};
} // namespace chain
} // namespace deip
