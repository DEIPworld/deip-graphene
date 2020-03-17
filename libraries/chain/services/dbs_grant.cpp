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

const grant_object& dbs_grant::create_grant_with_announced_application_window(
    const account_name_type& grantor,
    const asset& amount,
    const std::set<discipline_id_type>& target_disciplines,
    const research_group_id_type& review_committee_id,
    const uint16_t& min_number_of_positive_reviews,
    const uint16_t& min_number_of_applications,
    const uint16_t& max_number_of_research_to_grant,
    const fc::time_point_sec start_date,
    const fc::time_point_sec end_date)
{
    auto& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    account_balance_service.adjust_balance(grantor, -amount);

    const auto now = db_impl().head_block_time();
    const grant_object& grant = db_impl().create<grant_object>([&](grant_object& grant) {
        grant.grantor = grantor;
        grant.amount = amount;
        grant.target_disciplines.insert(target_disciplines.begin(), target_disciplines.end());
        grant.review_committee_id = review_committee_id;
        grant.min_number_of_positive_reviews = min_number_of_positive_reviews;
        grant.min_number_of_applications = min_number_of_applications;
        grant.max_number_of_research_to_grant = max_number_of_research_to_grant;
        grant.start_date = start_date;
        grant.end_date = end_date;
        grant.created_at = now;
    });

    return grant;
}

const grant_object& dbs_grant::get_grant_with_announced_application_window(const grant_id_type& id) const
{
    const auto& idx = db_impl()
      .get_index<grant_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    FC_ASSERT(itr != idx.end(), "Grant with id ${1} does not exist", ("1", id));
    return *itr;
}

const fc::optional<std::reference_wrapper<const grant_object>> dbs_grant::get_grant_with_announced_application_window_if_exists(const grant_id_type& id) const
{
    fc::optional<std::reference_wrapper<const grant_object>> result;

    const auto& idx = db_impl()
      .get_index<grant_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}


const bool dbs_grant::grant_with_announced_application_window_exists(const grant_id_type& id) const
{
    const auto& idx = db_impl()
      .get_index<grant_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    return itr != idx.end(); 
}


dbs_grant::grant_refs_type dbs_grant::get_grants_with_announced_application_window_by_grantor(const account_name_type& grantor)
{
    grant_refs_type ret;

    const auto& idx = db_impl()
      .get_index<grant_index>()
      .indicies()
      .get<by_owner>();

    auto it_pair = idx.equal_range(grantor);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


void dbs_grant::remove_grant_with_announced_application_window(const grant_object& grant)
{
    auto& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    account_balance_service.adjust_balance(grant.grantor, grant.amount);

    const auto& applications_idx = db_impl()
      .get_index<grant_application_index>()
      .indicies()
      .get<by_grant_id>();
      
    auto it_pair = applications_idx.equal_range(grant.id);
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