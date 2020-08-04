#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_award.hpp>
#include <deip/chain/services/dbs_funding_opportunity.hpp>
#include <deip/chain/services/dbs_grant_application.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/database/database.hpp>

#include <deip/chain/util/reward.hpp>

namespace deip {
namespace chain {

dbs_funding_opportunity::dbs_funding_opportunity(database &db)
    : _base_type(db)
{
}

const funding_opportunity_object& dbs_funding_opportunity::create_grant_with_officer_evaluation_distribution(const research_group_id_type& organization_id,
                                                                                                             const research_group_id_type& review_committee_id,
                                                                                                             const research_group_id_type& treasury_id,
                                                                                                             const account_name_type& grantor,
                                                                                                             const external_id_type& funding_opportunity_number,
                                                                                                             const flat_map<string, string>& additional_info,
                                                                                                             const std::set<discipline_id_type>& target_disciplines,
                                                                                                             const asset& amount,
                                                                                                             const asset& award_ceiling,
                                                                                                             const asset& award_floor,
                                                                                                             const uint16_t& expected_number_of_awards,
                                                                                                             const std::set<account_name_type>& officers,
                                                                                                             const fc::time_point_sec& open_date,
                                                                                                             const fc::time_point_sec& close_date)
{
    dbs_account_balance& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    account_balance_service.adjust_balance(grantor, -amount);

    const auto now = db_impl().head_block_time();
    const funding_opportunity_object& foa = db_impl().create<funding_opportunity_object>([&](funding_opportunity_object& funding_opportunity) {
        funding_opportunity.organization_id = organization_id;
        funding_opportunity.review_committee_id = review_committee_id;
        funding_opportunity.treasury_id = treasury_id;
        funding_opportunity.grantor = grantor;
        funding_opportunity.funding_opportunity_number = funding_opportunity_number;
        
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
        funding_opportunity.current_supply = amount; 
        funding_opportunity.expected_number_of_awards = expected_number_of_awards;
        funding_opportunity.officers.insert(officers.begin(), officers.end());
        funding_opportunity.open_date = open_date;
        funding_opportunity.close_date = close_date;
        funding_opportunity.posted_date = now;
        funding_opportunity.distribution_type = static_cast<uint16_t>(funding_opportunity_distribution_type::officer_evaluation);
    });

    return foa;
}

const funding_opportunity_object& dbs_funding_opportunity::create_grant_with_eci_evaluation_distribution(const account_name_type& grantor,
                                                                                                         const asset& amount,
                                                                                                         const std::set<discipline_id_type>& target_disciplines,
                                                                                                         const external_id_type& funding_opportunity_number,
                                                                                                         const flat_map<string, string>& additional_info,
                                                                                                         const research_group_id_type& review_committee_id,
                                                                                                         const uint16_t& min_number_of_positive_reviews,
                                                                                                         const uint16_t& min_number_of_applications,
                                                                                                         const uint16_t& max_number_of_research_to_grant,
                                                                                                         const fc::time_point_sec& open_date,
                                                                                                         const fc::time_point_sec& close_date)
{
    auto& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    account_balance_service.adjust_balance(grantor, -amount);

    const auto now = db_impl().head_block_time();
    const funding_opportunity_object& funding_opportunity = db_impl().create<funding_opportunity_object>([&](funding_opportunity_object& funding_opportunity) {
        funding_opportunity.grantor = grantor;
        funding_opportunity.amount = amount;
        funding_opportunity.current_supply = amount;
        funding_opportunity.target_disciplines.insert(target_disciplines.begin(), target_disciplines.end());
        funding_opportunity.funding_opportunity_number = funding_opportunity_number;

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

        funding_opportunity.review_committee_id = review_committee_id;
        funding_opportunity.min_number_of_positive_reviews = min_number_of_positive_reviews;
        funding_opportunity.min_number_of_applications = min_number_of_applications;
        funding_opportunity.max_number_of_research_to_grant = max_number_of_research_to_grant;
        funding_opportunity.open_date = open_date;
        funding_opportunity.close_date = close_date;
        funding_opportunity.posted_date = now;
        funding_opportunity.distribution_type = static_cast<uint16_t>(funding_opportunity_distribution_type::eci_evaluation);
    });

    return funding_opportunity;
}

const funding_opportunity_object& dbs_funding_opportunity::get_funding_opportunity(const funding_opportunity_id_type& id) const
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


const funding_opportunity_object& dbs_funding_opportunity::get_funding_opportunity(const external_id_type& funding_opportunity_number) const
{
    const auto& idx = db_impl()
      .get_index<funding_opportunity_index>()
      .indicies()
      .get<by_funding_opportunity_number>();

    auto itr = idx.find(funding_opportunity_number);
    FC_ASSERT(itr != idx.end(), "FO with ${1} does not exist", ("1", funding_opportunity_number));
    return *itr;
}


const fc::optional<std::reference_wrapper<const funding_opportunity_object>>
dbs_funding_opportunity::get_funding_opportunity_announcement_if_exists(const external_id_type& opportunity_number) const
{
    fc::optional<std::reference_wrapper<const funding_opportunity_object>> result;
    const auto& idx = db_impl()
      .get_index<funding_opportunity_index>()
      .indicies()
      .get<by_funding_opportunity_number>();

    auto itr = idx.find(opportunity_number);

    if (itr != idx.end()) 
    {
        result = *itr;
    }

    return result;
}

const bool dbs_funding_opportunity::funding_opportunity_exists(const funding_opportunity_id_type& id) const
{
    const auto& idx = db_impl()
      .get_index<funding_opportunity_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    return itr != idx.end();
}

const bool dbs_funding_opportunity::funding_opportunity_exists(const external_id_type& number) const
{
    const auto& idx = db_impl()
      .get_index<funding_opportunity_index>()
      .indicies()
      .get<by_funding_opportunity_number>();

    auto itr = idx.find(number);
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

void dbs_funding_opportunity::remove_funding_opportunity(const funding_opportunity_object& funding_opportunity)
{
    dbs_account_balance& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    account_balance_service.adjust_balance(funding_opportunity.grantor, funding_opportunity.amount);

    db_impl().remove(funding_opportunity);
}

void dbs_funding_opportunity::adjust_funding_opportunity_supply(const funding_opportunity_id_type& funding_opportunity_id, const asset& delta)
{
    const auto& foa = get_funding_opportunity(funding_opportunity_id);
    
    FC_ASSERT(foa.current_supply.amount + delta.amount > 0, 
      "Funding opportunity supply can not be negative. Current:${1} , Delta:{2}",
      ("1", foa.current_supply.amount)("2", delta.amount));
      
    db_impl().modify(foa, [&](funding_opportunity_object& ab_o) { 
        ab_o.current_supply += delta; 
    });
}

void dbs_funding_opportunity::distribute_funding_opportunity(const funding_opportunity_object &funding_opportunity)
{
    dbs_award& award_service = db_impl().obtain_service<dbs_award>();
    dbs_grant_application& grant_application_service = db_impl().obtain_service<dbs_grant_application>();
    dbs_research_discipline_relation& rdr_service = db_impl().obtain_service<dbs_research_discipline_relation>();
    dbs_research_group& research_group_service = db_impl().obtain_service<dbs_research_group>();
    dbs_account_balance& account_balance_service = db_impl().obtain_service<dbs_account_balance>();

    auto& dgpo = db_impl().get_dynamic_global_properties();

    if(funding_opportunity.distribution_type == static_cast<uint16_t>(funding_opportunity_distribution_type::eci_evaluation))
    {
        asset used_funding_opportunity = asset(0, funding_opportunity.amount.symbol);

        auto applications = grant_application_service.get_grant_applications_by_funding_opportunity_number(funding_opportunity.funding_opportunity_number);
        std::multimap<share_type, research_id_type, std::greater<share_type>> researches_eci;

        for (auto& application_ref : applications) {
            const auto &application = application_ref.get();

            auto parent_discipline_itr = std::min_element(funding_opportunity.target_disciplines.begin(), funding_opportunity.target_disciplines.end());
            if (parent_discipline_itr != funding_opportunity.target_disciplines.end())
            {
                const auto& rd_relation = rdr_service.get_research_discipline_relation_by_research_and_discipline(application.research_id, *parent_discipline_itr);
                researches_eci.insert(std::make_pair(rd_relation.research_eci, rd_relation.research_id));
            }
        }

        std::vector<std::pair<share_type, research_id_type>> max_number_of_research_to_grant(funding_opportunity.max_number_of_research_to_grant);
        std::copy(researches_eci.begin(), researches_eci.end(), max_number_of_research_to_grant.begin());

        share_type total_eci = 0;
        for (auto& research_eci : max_number_of_research_to_grant)
            total_eci += research_eci.first;

        award_id_type max_award_id;
        award_recipient_id_type max_award_recipient_id;
        research_group_id_type research_group_with_max_award_id;
        share_type max_eci = 0;

        for (auto& research_eci : max_number_of_research_to_grant)
        {
            asset research_reward = util::calculate_share(funding_opportunity.amount, research_eci.first, total_eci);
            auto& research = db_impl().get<research_object>(research_eci.second);
            auto& research_group = research_group_service.get_research_group(research.research_group_id);

            string award_number(funding_opportunity.funding_opportunity_number);
            award_number.append(research.external_id);
            award_number.append(fc::to_string(dgpo.head_block_number));

            auto& award = award_service.create_award(funding_opportunity.funding_opportunity_number,
                                                     external_id_type((string)fc::ripemd160::hash(award_number)),
                                                     research_group.account,
                                                     research_reward,
                                                     research.research_group_id,
                                                     percent(0),
                                                     account_name_type(),
                                                     award_status::approved);

            string subaward_number(funding_opportunity.funding_opportunity_number);
            subaward_number.append(research.external_id);
            subaward_number.append(research_group.account);
            subaward_number.append(fc::to_string(dgpo.head_block_number));

            auto& award_recipient = award_service.create_award_recipient(external_id_type((string)fc::ripemd160::hash(award_number)),
                                                                         external_id_type((string)fc::ripemd160::hash(subaward_number)),
                                                                         funding_opportunity.funding_opportunity_number,
                                                                         research_group.account,
                                                                         account_name_type(),
                                                                         research_reward,
                                                                         research.id._id,
                                                                         award_recipient_status::confirmed);

            if (research_eci.first > max_eci) {
                max_award_id = award.id;
                max_award_recipient_id = award_recipient.id;
                research_group_with_max_award_id = research.research_group_id;
                max_eci = research_eci.first;
            }

            account_balance_service.adjust_balance(research_group.account, research_reward);
            award_service.adjust_expenses(award_recipient.id._id, research_reward);

            used_funding_opportunity += research_reward;
        }

        if (used_funding_opportunity > funding_opportunity.current_supply)
            wlog("Attempt to distribute amount that is greater than funding_opportunity current supply: ${1} > ${2}, funding_opportunity: ${3}",
                ("1", used_funding_opportunity)("2", funding_opportunity.current_supply)("3", funding_opportunity.funding_opportunity_number));

        if (used_funding_opportunity < funding_opportunity.current_supply)
        {
            asset remainder = funding_opportunity.current_supply - used_funding_opportunity;
            const auto& award = award_service.get_award(max_award_id);
            const auto& award_recipient = award_service.get_award_recipient(max_award_recipient_id);

            const auto& research_group = research_group_service.get_research_group(research_group_with_max_award_id);
            account_balance_service.adjust_balance(research_group.account, remainder);

            db_impl().modify(award, [&](award_object& a_o) {
                a_o.amount += remainder;
            });

            db_impl().modify(award_recipient, [&](award_recipient_object& ar_o) {
                ar_o.total_amount += remainder;
                ar_o.total_expenses += remainder;
            });
        }

        adjust_funding_opportunity_supply(funding_opportunity.id, -used_funding_opportunity);
    }
}

void dbs_funding_opportunity::process_funding_opportunities()
{
    dbs_research& research_service = db_impl().obtain_service<dbs_research>();
    dbs_grant_application& grant_application_service = db_impl().obtain_service<dbs_grant_application>();

    const auto& fo_with_eci_evaluation_idx =
            db_impl().get_index<funding_opportunity_index>()
                    .indices()
                    .get<by_distribution_type_and_close_date>()
                    .equal_range(static_cast<uint16_t>(funding_opportunity_distribution_type::eci_evaluation));

    auto it = fo_with_eci_evaluation_idx.first;
    const auto it_end = fo_with_eci_evaluation_idx.second;

    auto _head_block_time = db_impl().head_block_time();

    while (it != it_end && it->close_date <= _head_block_time)
    {
        auto current_fo_with_eci_evaluation = it++;
        auto& fo_with_eci_evaluation = *current_fo_with_eci_evaluation;
        auto applications = grant_application_service.get_grant_applications_by_funding_opportunity_number(fo_with_eci_evaluation.funding_opportunity_number);
        if (applications.size() < fo_with_eci_evaluation.min_number_of_applications) {
            remove_funding_opportunity(fo_with_eci_evaluation);
            continue;
        }

        std::vector<grant_application_id_type> applications_to_delete;
        for (auto& application_ref : applications) {
            const auto& application = application_ref.get();
            const auto& research = research_service.get_research(application.research_id);
            if (research.number_of_positive_reviews < fo_with_eci_evaluation.min_number_of_positive_reviews)
                applications_to_delete.push_back(application.id);
        }

        for (auto& application_id : applications_to_delete)
            grant_application_service.delete_grant_appication_by_id(application_id);

        if (grant_application_service.get_grant_applications_by_funding_opportunity_number(fo_with_eci_evaluation.funding_opportunity_number).size() == 0)
        {
            remove_funding_opportunity(fo_with_eci_evaluation);
            continue;
        }

        distribute_funding_opportunity(fo_with_eci_evaluation);
    }
}


} //namespace chain
} //namespace deip