#include <deip/chain/services/dbs_award.hpp>
#include <deip/chain/database/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_award::dbs_award(database &db)
        : _base_type(db)
{
}

const award_object& dbs_award::create_award(const external_id_type& funding_opportunity_number,
                                            const external_id_type& award_number,
                                            const account_name_type& awardee,
                                            const asset& amount,
                                            const account_id_type& university_id,
                                            const external_id_type& university_external_id,
                                            const percent& university_overhead,
                                            const account_name_type& creator,
                                            const award_status& status)
{
    const award_object& award = db_impl().create<award_object>([&](award_object& award) {
        award.funding_opportunity_number = funding_opportunity_number;
        award.award_number = award_number;
        award.awardee = awardee;
        award.amount = amount;
        award.university_id = university_id;
        award.university_external_id = university_external_id;
        award.university_overhead = university_overhead;
        award.creator = creator;
        award.status = static_cast<uint16_t>(status);
    });

    return award;
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

const award_object& dbs_award::get_award(const external_id_type& award_number) const
{
    const auto& idx = db_impl()
      .get_index<award_index>()
      .indicies()
      .get<by_award_number>();

    auto itr = idx.find(award_number);
    FC_ASSERT(itr != idx.end(), "Award NUM:${1} does not exist", ("1", award_number));
    return *itr;
}

const dbs_award::award_optional_ref_type dbs_award::get_award_if_exists(const award_id_type& id) const
{
    award_optional_ref_type result;

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

const dbs_award::award_optional_ref_type dbs_award::get_award_if_exists(const external_id_type& award_number) const
{
    award_optional_ref_type result;

    const auto& idx = db_impl()
      .get_index<award_index>()
      .indicies()
      .get<by_award_number>();

    auto itr = idx.find(award_number);
    if (itr != idx.end())
    {
        result = *itr;
    }
    return result;
}

dbs_award::award_refs_type dbs_award::get_awards_by_funding_opportunity(const external_id_type& funding_opportunity_number) const
{
    award_refs_type ret;

    const auto& idx = db_impl()
      .get_index<award_index>()
      .indicies()
      .get<by_funding_opportunity_number>();

    auto it_pair = idx.equal_range(funding_opportunity_number);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


const award_recipient_object& dbs_award::create_award_recipient(
  const external_id_type& award_number,
  const external_id_type& subaward_number,
  const external_id_type& funding_opportunity_number,
  const account_name_type& awardee,
  const account_name_type& source,
  const asset& total_amount,
  const research_id_type& research_id,
  const external_id_type& research_external_id,
  const award_recipient_status& status)
{
    const award_recipient_object& award_recipient = 
      db_impl().create<award_recipient_object>([&](award_recipient_object& ar_o) {
        ar_o.award_number = award_number;
        ar_o.subaward_number = subaward_number;
        ar_o.funding_opportunity_number = funding_opportunity_number;
        ar_o.awardee = awardee;
        ar_o.source = source;
        ar_o.total_amount = total_amount;
        ar_o.total_expenses = asset(0, total_amount.symbol);
        ar_o.research_id = research_id;
        ar_o.research_external_id = research_external_id;
        ar_o.status = static_cast<uint16_t>(status);
    });

    return award_recipient;
}

const bool dbs_award::award_exists(const award_id_type& award_id) const
{
    const auto& idx = db_impl()
      .get_index<award_index>()
      .indices()
      .get<by_id>();

    auto itr = idx.find(award_id);
    return itr != idx.end();
}

const bool dbs_award::award_exists(const external_id_type& award_number) const
{
    const auto& idx = db_impl()
      .get_index<award_index>()
      .indices()
      .get<by_award_number>();

    auto itr = idx.find(award_number);
    return itr != idx.end();
}

const award_object& dbs_award::update_award_status(
  const award_object& award,
  const award_status& status)
{
    db_impl().modify(award, [&](award_object& a_o) {
        a_o.status = static_cast<uint16_t>(status);
    });
    return award;
}

const award_recipient_object& dbs_award::get_award_recipient(const award_recipient_id_type& id) const
{
    const auto& idx = db_impl()
      .get_index<award_recipient_index>()
      .indicies()
      .get<by_id>();
      
    auto itr = idx.find(id);
    FC_ASSERT(itr != idx.end(), "Award recipient id:${1} does not exists", ("1", id));
    return *itr;
}

const dbs_award::award_recipient_optional_ref_type dbs_award::get_award_recipient_if_exists(const award_recipient_id_type& id) const
{
    award_recipient_optional_ref_type result;

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

dbs_award::award_recipient_refs_type dbs_award::get_award_recipients_by_funding_opportunity(const external_id_type& funding_opportunity_number) const
{
    award_recipient_refs_type ret;

    const auto& idx = db_impl()
      .get_index<award_recipient_index>()
      .indicies()
      .get<by_funding_opportunity_number>();

    auto it_pair = idx.equal_range(funding_opportunity_number);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_award::award_recipient_refs_type dbs_award::get_award_recipients_by_award(const external_id_type& award_number) const
{
    award_recipient_refs_type ret;

    const auto& idx = db_impl()
      .get_index<award_recipient_index>()
      .indicies()
      .get<by_award_number>();

    auto it_pair = idx.equal_range(award_number);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_award::award_recipient_refs_type dbs_award::get_award_recipients_by_account(const account_name_type& awardee) const
{
    award_recipient_refs_type ret;

    const auto& idx = db_impl()
      .get_index<award_recipient_index>()
      .indicies()
      .get<by_awardee>();

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


const bool dbs_award::award_recipient_exists(const award_recipient_id_type& award_recipient_id) const
{
    const auto& idx = db_impl()
      .get_index<award_recipient_index>()
      .indices()
      .get<by_id>();

    auto itr = idx.find(award_recipient_id);
    return itr != idx.end();
}

const award_recipient_object& dbs_award::adjust_expenses(
  const award_recipient_id_type& award_recipient_id, const asset& delta)
{
    const auto& award_recipient = get_award_recipient(award_recipient_id);
    db_impl().modify(award_recipient, [&](award_recipient_object& arr_o) { arr_o.total_expenses += delta; });

    return award_recipient;
}

const bool dbs_award::award_recipient_exists(
  const external_id_type& award_number,
  const external_id_type& subaward_number) const
{
    const auto& idx = db_impl()
      .get_index<award_recipient_index>()
      .indices()
      .get<by_award_and_subaward_number>();

    auto itr = idx.find(std::make_tuple(award_number, subaward_number));
    return itr != idx.end();
}

const award_recipient_object& dbs_award::update_award_recipient_status(
  const award_recipient_object& award_recipient, 
  const award_recipient_status& status)
{
    db_impl().modify(award_recipient, [&](award_recipient_object& a_o) { 
        a_o.status = static_cast<uint16_t>(status);
    });
    return award_recipient;
}

const award_recipient_object& dbs_award::get_award_recipient(const external_id_type& award_number, const external_id_type& subaward_number) const
{
    const auto& idx = db_impl()
      .get_index<award_recipient_index>()
      .indicies()
      .get<by_award_and_subaward_number>();

    auto itr = idx.find(std::make_tuple(award_number, subaward_number));
    FC_ASSERT(itr != idx.end(), "Subward ${1}:${2} does not exists", ("1", award_number)("2", subaward_number));
    return *itr;
}

const award_withdrawal_request_object& dbs_award::create_award_withdrawal_request(
  const external_id_type& payment_number,
  const external_id_type& award_number,
  const external_id_type& subaward_number,
  const account_name_type& requester,
  const asset& amount,
  const std::string& description,
  const fc::time_point_sec& time,
  const std::string& attachment)
{
    const auto& withdrawal = db_impl().create<award_withdrawal_request_object>([&](award_withdrawal_request_object& awr_o) {
        awr_o.payment_number = payment_number;
        awr_o.award_number = award_number;
        awr_o.subaward_number = subaward_number;
        awr_o.requester = requester;
        awr_o.amount = amount;
        awr_o.time = time;
        fc::from_string(awr_o.description, description);
        fc::from_string(awr_o.attachment, attachment);
    });

    return withdrawal;
}

const dbs_award::award_withdrawal_request_ref_optional_type dbs_award::get_award_withdrawal_request_if_exists(
  const external_id_type& award_number,
  const external_id_type& payment_number) const
{
    award_withdrawal_request_ref_optional_type result;

    const auto& idx = db_impl()
      .get_index<award_withdrawal_request_index>()
      .indices()
      .get<by_award_and_payment_number>();

    auto itr = idx.find(std::make_tuple(award_number, payment_number));
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const bool dbs_award::award_withdrawal_request_exists(
  const external_id_type& award_number,
  const external_id_type& payment_number) const
{
    const auto& idx = db_impl()
      .get_index<award_withdrawal_request_index>()
      .indices()
      .get<by_award_and_payment_number>();

    auto itr = idx.find(std::make_tuple(award_number, payment_number));
    return itr != idx.end();
}

const award_withdrawal_request_object& dbs_award::get_award_withdrawal_request(
  const external_id_type& award_number,
  const external_id_type& payment_number) const
{
    const auto& idx = db_impl()
      .get_index<award_withdrawal_request_index>()
      .indices()
      .get<by_award_and_payment_number>();

    auto itr = idx.find(std::make_tuple(award_number, payment_number));
    FC_ASSERT(itr != idx.end(), "Award withdrawal ${1}:${2} does not exist", ("1", award_number)("2", payment_number));
    return *itr;
}

const award_withdrawal_request_object& dbs_award::update_award_withdrawal_request(
  const award_withdrawal_request_object& award_withdrawal_request,
  const award_withdrawal_request_status& status)
{
    db_impl().modify(award_withdrawal_request, [&](award_withdrawal_request_object& awr_o) { 
        awr_o.status = static_cast<uint16_t>(status); 
    });
    return award_withdrawal_request;
}


dbs_award::award_withdrawal_request_refs_type dbs_award::get_award_withdrawal_requests_by_award_and_status(
  const external_id_type& award_number,
  const award_withdrawal_request_status& status) const
{
    award_withdrawal_request_refs_type ret;

    const auto& idx = db_impl()
      .get_index<award_withdrawal_request_index>()
      .indicies()
      .get<by_award_number_and_status>();

    auto it_pair = idx.equal_range(std::make_tuple(award_number, static_cast<uint16_t>(status)));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_award::award_withdrawal_request_refs_type dbs_award::get_award_withdrawal_requests_by_award(const external_id_type& award_number) const
{
    award_withdrawal_request_refs_type ret;

    const auto& idx = db_impl()
      .get_index<award_withdrawal_request_index>()
      .indicies()
      .get<by_award_number>();

    auto it_pair = idx.equal_range(award_number);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_award::award_withdrawal_request_refs_type dbs_award::get_award_withdrawal_requests_by_award_and_subaward(
  const external_id_type& award_number,
  const external_id_type& subaward_number) const
{
    award_withdrawal_request_refs_type ret;

    const auto& idx = db_impl()
      .get_index<award_withdrawal_request_index>()
      .indicies()
      .get<by_award_and_subaward_number>();

    auto it_pair = idx.equal_range(std::make_tuple(award_number, subaward_number));
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
