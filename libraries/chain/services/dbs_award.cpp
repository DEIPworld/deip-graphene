#include <deip/chain/services/dbs_award.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_award::dbs_award(database &db)
        : _base_type(db)
{
}

const award_object& dbs_award::create_award(const funding_opportunity_id_type& funding_opportunity_id,
                                            const account_name_type &creator,
                                            const asset &amount)
{
    auto& new_award = db_impl().create<award_object>([&](award_object& award) {
        award.funding_opportunity_id = funding_opportunity_id;
        award.creator = creator;
        award.amount = amount;
    });

    return new_award;
}

const award_object& dbs_award::get_award(const award_id_type& id) const
{
    const auto& idx = db_impl()
      .get_index<award_index>()
      .indicies()
      .get<by_id>();
      
    auto itr = idx.find(id);
    FC_ASSERT(itr != idx.end(), "Award id:${1} does not exist", ("1", id));
    return *itr;
}

const dbs_award::award_optional_type dbs_award::get_award_if_exists(const award_id_type& id) const
{
    award_optional_type result;

    const auto& idx = db_impl()
      .get_index<award_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

dbs_award::award_refs_type dbs_award::get_awards_by_funding_opportunity(const funding_opportunity_id_type& funding_opportunity_id) const
{
    award_refs_type ret;

    const auto& idx = db_impl()
      .get_index<award_index>()
      .indicies()
      .get<by_funding_opportunity>();

    auto it_pair = idx.equal_range(funding_opportunity_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const award_recipient_object& dbs_award::create_award_recipient(const award_id_type& award_id,
                                                                const funding_opportunity_id_type& funding_opportunity_id,
                                                                const research_id_type& research_id,
                                                                const research_group_id_type& research_group_id,
                                                                const account_name_type& awardee,
                                                                const asset& total_amount,
                                                                const research_group_id_type& university_id,
                                                                const share_type& university_overhead)
{
    auto& award_recipient = db_impl().create<award_recipient_object>([&](award_recipient_object& ar_o) {
        ar_o.award_id = award_id;
        ar_o.funding_opportunity_id = funding_opportunity_id;
        ar_o.research_id = research_id;
        ar_o.research_group_id = research_group_id;
        ar_o.awardee = awardee;
        ar_o.total_amount = total_amount;
        ar_o.total_expenses = asset(0, total_amount.symbol);
        ar_o.university_id = university_id;
        ar_o.university_overhead = university_overhead;
    });

    return award_recipient;
}

const award_recipient_object& dbs_award::get_award_recipient(const award_recipient_id_type& id)
{
    const auto& idx = db_impl()
      .get_index<award_recipient_index>()
      .indicies()
      .get<by_id>();
      
    auto itr = idx.find(id);
    FC_ASSERT(itr != idx.end(), "Award recipient id:${1} does not exists", ("1", id));
    return *itr;
}

const dbs_award::award_recipient_optional_type dbs_award::get_award_recipient_if_exists(const award_recipient_id_type& id)
{
    award_recipient_optional_type result;

    const auto& idx = db_impl()
      .get_index<award_recipient_index>()
      .indicies()
      .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

dbs_award::award_recipient_refs_type dbs_award::get_award_recipients_by_award(const award_id_type& award_id)
{
    award_recipient_refs_type ret;

    const auto& idx = db_impl()
      .get_index<award_recipient_index>()
      .indicies()
      .get<by_award>();

    auto it_pair = idx.equal_range(award_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_award::award_recipient_refs_type dbs_award::get_award_recipients_by_funding_opportunity(const funding_opportunity_id_type& funding_opportunity_id)
{
    award_recipient_refs_type ret;

    const auto& idx = db_impl()
      .get_index<award_recipient_index>()
      .indicies()
      .get<by_funding_opportunity>();

    auto it_pair = idx.equal_range(funding_opportunity_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_award::award_recipient_refs_type dbs_award::get_award_recipients_by_account(const account_name_type& awardee)
{
    award_recipient_refs_type ret;

    const auto& idx = db_impl().get_index<award_recipient_index>().indicies().get<by_awardee>();

    auto it_pair = idx.equal_range(awardee);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

} //namespace chain
} //
