#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_funding_opportunity.hpp>
#include <deip/chain/database/database.hpp>

namespace deip {
namespace chain {

dbs_funding_opportunity::dbs_funding_opportunity(database &db)
    : _base_type(db)
{
}

const funding_opportunity_object& dbs_funding_opportunity::create_funding_opportunity_announcement(
  const research_group_id_type& organization_id,
  const research_group_id_type& review_committee_id,
  const account_name_type& grantor,
  const string& funding_opportunity_number,
  const flat_map<string, string>& additional_info,
  const std::set<discipline_id_type>& target_disciplines,
  const asset& amount,
  const asset& award_ceiling, 
  const asset& award_floor,
  const uint16_t& expected_number_of_awards,
  const fc::time_point_sec& open_date,
  const fc::time_point_sec& close_date)
{
    dbs_account_balance& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    account_balance_service.adjust_balance(grantor, -amount);

    const auto now = db_impl().head_block_time();
    const funding_opportunity_object& foa = db_impl().create<funding_opportunity_object>([&](funding_opportunity_object& funding_opportunity) {
        funding_opportunity.organization_id = organization_id;
        funding_opportunity.review_committee_id = review_committee_id;
        funding_opportunity.grantor = grantor;

        fc::from_string(funding_opportunity.funding_opportunity_number, funding_opportunity_number);
        
        for (auto& pair : additional_info)
        {
            string key = pair.first;
            int key_length = key.length();
            char key_array[key_length + 1];
            strcpy(key_array, key.c_str());

            string val = pair.second;
            int val_length = val.length();
            char val_array[val_length + 1];
            strcpy(val_array, val.c_str());

            funding_opportunity.additional_info.insert(std::pair<fc::shared_string, fc::shared_string>(
                fc::shared_string(key_array, basic_string_allocator(db_impl().get_segment_manager())),
                fc::shared_string(val_array, basic_string_allocator(db_impl().get_segment_manager())))
            );
        }

        funding_opportunity.target_disciplines.insert(target_disciplines.begin(), target_disciplines.end());
        funding_opportunity.amount = amount;
        funding_opportunity.award_ceiling = award_ceiling;
        funding_opportunity.award_floor = award_floor;
        funding_opportunity.expected_number_of_awards = expected_number_of_awards;
        funding_opportunity.open_date = open_date;
        funding_opportunity.close_date = close_date;
        funding_opportunity.posted_date = now;
    });

    return foa;
}

const funding_opportunity_object& dbs_funding_opportunity::get_funding_opportunity_announcement(const funding_opportunity_id_type& id) const
{
    const auto& idx = db_impl()
      .get_index<funding_opportunity_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    FC_ASSERT(itr != idx.end(), "FO with ${id} does not exist", ("id", id));
    return *itr;
}


const fc::optional<std::reference_wrapper<const funding_opportunity_object>> dbs_funding_opportunity::get_funding_opportunity_announcement_if_exists(const funding_opportunity_id_type& id) const
{
    fc::optional<std::reference_wrapper<const funding_opportunity_object>> result;
    const auto& idx = db_impl()
      .get_index<funding_opportunity_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end()) 
    {
        result = *itr;
    }

    return result;
}


const funding_opportunity_object& dbs_funding_opportunity::get_funding_opportunity_announcement_by_number(const string& opportunity_number) const
{
    const auto& idx = db_impl()
      .get_index<funding_opportunity_index>()
      .indicies()
      .get<by_opportunity_number>();

    auto itr = idx.find(opportunity_number, fc::strcmp_less());
    FC_ASSERT(itr != idx.end(), "FO with ${num} does not exist", ("num", opportunity_number));
    return *itr;
}


const fc::optional<std::reference_wrapper<const funding_opportunity_object>> dbs_funding_opportunity::get_funding_opportunity_announcement_by_number_if_exists(const string& opportunity_number) const
{
    fc::optional<std::reference_wrapper<const funding_opportunity_object>> result;
    const auto& idx = db_impl()
      .get_index<funding_opportunity_index>()
      .indicies()
      .get<by_opportunity_number>();

    auto itr = idx.find(opportunity_number, fc::strcmp_less());

    if (itr != idx.end()) 
    {
        result = *itr;
    }

    return result;
}

const bool dbs_funding_opportunity::funding_opportunity_announcement_exists(const funding_opportunity_id_type& id) const
{
    const auto& idx = db_impl()
      .get_index<funding_opportunity_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    return itr != idx.end();
}

dbs_funding_opportunity::funding_opportunity_refs_type dbs_funding_opportunity::get_funding_opportunity_announcements_by_grantor(const account_name_type& grantor) const
{
    funding_opportunity_refs_type ret;
    const auto& idx = db_impl()
      .get_index<funding_opportunity_index>()
      .indicies()
      .get<by_grantor>();

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

dbs_funding_opportunity::funding_opportunity_refs_type dbs_funding_opportunity::get_funding_opportunity_announcements_by_organization(const research_group_id_type& organization_id) const
{
    funding_opportunity_refs_type ret;
    const auto& idx = db_impl()
      .get_index<funding_opportunity_index>()
      .indicies()
      .get<by_organization>();

    auto it_pair = idx.equal_range(organization_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

// TODO: implement pagination
dbs_funding_opportunity::funding_opportunity_refs_type dbs_funding_opportunity::get_funding_opportunity_announcements_listing(const uint16_t& page, const uint16_t& limit) const
{
    funding_opportunity_refs_type ret;
    const auto& idx = db_impl()
      .get_index<funding_opportunity_index>()
      .indicies()
      .get<by_id>();

    auto it = idx.lower_bound(0);
    const auto it_end = idx.cend();

    while (it != it_end && ret.size() < limit)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_funding_opportunity::remove_funding_opportunity_announcement(const funding_opportunity_object& funding_opportunity)
{
    dbs_account_balance& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    account_balance_service.adjust_balance(funding_opportunity.grantor, funding_opportunity.amount);

    db_impl().remove(funding_opportunity);
}

} //namespace chain
} //namespace deip