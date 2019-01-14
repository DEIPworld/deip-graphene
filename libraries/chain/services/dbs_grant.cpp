#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_grant.hpp>
#include <deip/chain/services/dbs_grant_application.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

dbs_grant::dbs_grant(database &db)
    : _base_type(db)
{
}

const grant_object& dbs_grant::create(const discipline_id_type& target_discipline,
                                      const asset& amount,
                                      const int16_t& min_number_of_positive_reviews,
                                      const int16_t& min_number_of_applications,
                                      const int16_t& max_number_of_researches_to_grant,
                                      fc::time_point_sec start_time,
                                      fc::time_point_sec end_time,
                                      const account_name_type& owner,
                                      const set<account_name_type>& officers)
{
    auto now = db_impl().head_block_time();

    FC_ASSERT(start_time >= now, "Start time must be greater then now");

    auto& grant = db_impl().create<grant_object>([&](grant_object& grant) {
        grant.target_discipline = target_discipline;
        grant.amount = amount;
        grant.min_number_of_positive_reviews = min_number_of_positive_reviews;
        grant.min_number_of_applications = min_number_of_applications;
        grant.max_number_of_researches_to_grant = max_number_of_researches_to_grant;
        grant.start_time = start_time;
        grant.end_time = end_time;
        grant.created_at = now;
        grant.owner = owner;
        grant.officers.insert(officers.begin(), officers.end());
    });

    return grant;
}

const grant_object& dbs_grant::get(const grant_id_type& id) const
{
    try {
        return db_impl().get<grant_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

void dbs_grant::check_grant_existence(const grant_id_type& id) const
{
    const auto& grant = db_impl().find<grant_object, by_id>(id);
    FC_ASSERT(grant != nullptr, "Grant with id \"${1}\" must exist.", ("1", id));
}

dbs_grant::grant_refs_type dbs_grant::get_by_target_discipline(const discipline_id_type& discipline_id)
{
    grant_refs_type ret;

    auto it_pair = db_impl().get_index<grant_index>().indicies().get<by_target_discipline>().equal_range(discipline_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_grant::grant_refs_type dbs_grant::get_by_owner(const account_name_type& owner)
{
    grant_refs_type ret;

    auto it_pair = db_impl().get_index<grant_index>().indicies().get<by_owner>().equal_range(owner);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

std::set<string> dbs_grant::lookup_grant_owners(const string &lower_bound_owner_name,
                                                                        uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_LIMIT_DISCIPLINE_SUPPLIES_LIST_SIZE,
              "limit must be less or equal than ${1}, actual limit value == ${2}",
              ("1", DEIP_LIMIT_DISCIPLINE_SUPPLIES_LIST_SIZE)("2", limit));
    const auto& grants_by_owner_name = db_impl().get_index<grant_index>().indices().get<by_owner>();
    set<string> result;

    for (auto itr = grants_by_owner_name.lower_bound(lower_bound_owner_name);
         limit-- && itr != grants_by_owner_name.end(); ++itr)
    {
        result.insert(itr->owner);
    }

    return result;
}

void dbs_grant::delete_grant(const grant_object& grant)
{
    auto& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    account_balance_service.adjust_balance(grant.owner, grant.amount);

    auto it_pair = db_impl().get_index<grant_application_index>().indicies().get<by_grant_id>().equal_range(grant.id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        auto current = it++;
        db_impl().remove(*current);
    }

    db_impl().remove(grant);
}

} //namespace chain
} //namespace deip