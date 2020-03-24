#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_grant.hpp>
#include <deip/chain/services/dbs_grant_application.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/services/dbs_research_group.hpp>

#include <deip/chain/util/reward.hpp>

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

void dbs_grant::distribute_grant(const grant_object& grant)
{
    dbs_grant_application& grant_application_service = db_impl().obtain_service<dbs_grant_application>();
    dbs_research_discipline_relation& rdr_service = db_impl().obtain_service<dbs_research_discipline_relation>();
    dbs_research_group& research_group_service = db_impl().obtain_service<dbs_research_group>();

    asset used_grant = asset(0, grant.amount.symbol);

    auto applications = grant_application_service.get_grant_applications_by_grant(grant.id);
    std::multimap<share_type, research_id_type, std::greater<share_type>> researches_eci;

    for (auto& application_ref : applications) {
        const auto &application = application_ref.get();

        auto parent_discipline_itr = std::min_element(grant.target_disciplines.begin(), grant.target_disciplines.end());
        if (parent_discipline_itr != grant.target_disciplines.end())
        {
            const auto& rd_relation = rdr_service.get_research_discipline_relation_by_research_and_discipline(application.research_id, *parent_discipline_itr);
            researches_eci.insert(std::make_pair(rd_relation.research_eci, rd_relation.research_id));
        }
    }

    std::vector<std::pair<share_type, research_id_type>> max_number_of_research_to_grant(grant.max_number_of_research_to_grant);
    std::copy(researches_eci.begin(), researches_eci.end(), max_number_of_research_to_grant.begin());

    share_type total_eci = 0;
    for (auto& research_eci : max_number_of_research_to_grant)
        total_eci += research_eci.first;

    for (auto& research_eci : max_number_of_research_to_grant)
    {
        asset research_reward = util::calculate_share(grant.amount, research_eci.first, total_eci);
        auto& research = db_impl().get<research_object>(research_eci.second);
        research_group_service.increase_research_group_balance(research.research_group_id, research_reward);
        used_grant += research_reward;
    }

    if (used_grant > grant.amount)
        wlog("Attempt to distribute amount that is greater than grant amount: ${used_grant} > ${grant}, grant: ${g_id}", ("used_grant", used_grant)("grant", grant.amount)("g_id", grant.id._id));

    //FC_ASSERT(used_grant <= grant.amount, "Attempt to distribute amount that is greater than grant amount");

    db_impl().modify(grant, [&](grant_object& g) {
        g.amount -= used_grant;
    });

    remove_grant_with_announced_application_window(grant);
}

void dbs_grant::process_grants()
{
    dbs_research& research_service = db_impl().obtain_service<dbs_research>();
    dbs_grant_application& grant_application_service = db_impl().obtain_service<dbs_grant_application>();

    const auto& grants_idx = db_impl().get_index<grant_index>().indices().get<by_end_date>();
    auto itr = grants_idx.begin();
    auto _head_block_time = db_impl().head_block_time();

    while (itr != grants_idx.end() && itr->end_date <= _head_block_time)
    {
        auto current_grant = itr++;
        auto& grant = *current_grant;
        auto applications = grant_application_service.get_grant_applications_by_grant(grant.id);
        if (applications.size() < grant.min_number_of_applications) {
            remove_grant_with_announced_application_window(grant);
            continue;
        }

        std::vector<grant_application_id_type> applications_to_delete;
        for (auto& application_ref : applications) {
            const auto& application = application_ref.get();
            const auto& research = research_service.get_research(application.research_id);
            if (research.number_of_positive_reviews < grant.min_number_of_positive_reviews)
                applications_to_delete.push_back(application.id);
        }

        for (auto& application_id : applications_to_delete)
            grant_application_service.delete_grant_appication_by_id(application_id);

        if (grant_application_service.get_grant_applications_by_grant(grant.id).size() == 0)
        {
            remove_grant_with_announced_application_window(grant);
            continue;
        }

        distribute_grant(grant);
    }
}

} //namespace chain
} //namespace deip