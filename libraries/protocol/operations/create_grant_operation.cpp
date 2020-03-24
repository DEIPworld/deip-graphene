#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {


void inline validate_grant_contract_permitted_details(
  const uint16_t& kind, 
  const std::vector<grant_contract_details>& details)
{
    bool is_validated = false;

    // TODO: change op.details type to set
    std::set<grant_contract_details> distinct;
    distinct.insert(details.begin(), details.end());
    FC_ASSERT(distinct.size() == details.size(), "Duplicated research group details specified");

    const grant_contract_type type = static_cast<grant_contract_type>(kind);
    FC_ASSERT(type != grant_contract_type::unknown, "Grant contract type is required");
    FC_ASSERT(type >= grant_contract_type::FIRST && type <= grant_contract_type::LAST, 
      "Provided enum value is outside of the range: val = ${enum_val}, first = ${first}, last = ${last}",
      ("enum_val", type)("first", grant_contract_type::FIRST)("last", grant_contract_type::LAST));

    if (type == grant_contract_type::announced_application_window)
    {
        const std::set<int>& permitted_details = 
        {
            grant_contract_details::tag<announced_application_window_contract_v1_0_0_type>::value
        };

        const bool is_permitted = std::count_if(details.begin(), details.end(),
          [&](const grant_contract_details& detail) {
              return permitted_details.count(detail.which()) != 0;
          }) == details.size();

        FC_ASSERT(is_permitted, 
          "Provided grant contract details are not permitted for ${type} type",
          ("type", type));

        is_validated = true;
    }

    else if (type == grant_contract_type::funding_opportunity_announcement)
    {
        const std::set<int>& permitted_details = 
        {
            grant_contract_details::tag<funding_opportunity_announcement_contract_v1_0_0_type>::value
        };

        const bool is_permitted = std::count_if(details.begin(), details.end(),
          [&](const grant_contract_details& detail) {
              return permitted_details.count(detail.which()) != 0;
          }) == details.size();

        FC_ASSERT(is_permitted, 
          "Provided grant contract details are not permitted for ${type} type",
          ("type", type));

        is_validated = true;
    }

    else if (type == grant_contract_type::discipline_supply_announcement)
    {
        const std::set<int>& permitted_details =
        {
            grant_contract_details::tag<discipline_supply_announcement_contract_v1_0_0_type>::value
        };

        const bool is_permitted = std::count_if(details.begin(), details.end(),
          [&](const grant_contract_details& detail) {
              return permitted_details.count(detail.which()) != 0;
          }) == details.size();

        FC_ASSERT(is_permitted,
          "Provided grant contract details are not permitted for ${type} type",
          ("type", type));

        is_validated = true;
    }

    FC_ASSERT(is_validated, "Grant contract details are not validated");
}


void inline validate_grant_contract(const grant_contract_details grant_contract, const asset& amount, const account_name_type& grantor, const std::set<int64_t>& target_disciplines)
{  
  bool is_validated = false;

  if (grant_contract.which() == grant_contract_details::tag<announced_application_window_contract_v1_0_0_type>::value) 
  {
      const auto announced_application_window_contract = grant_contract.get<announced_application_window_contract_v1_0_0_type>();
      FC_ASSERT(announced_application_window_contract.min_number_of_positive_reviews >= 0, "Min number of positive reviews must be equal or greater than 0");
      FC_ASSERT(announced_application_window_contract.min_number_of_applications > 0, "Min number of grant applications must be greater than 0");
      FC_ASSERT(announced_application_window_contract.min_number_of_applications >= announced_application_window_contract.max_number_of_research_to_grant, "Min number of grant applications must be equal or greater than max number of research");
      FC_ASSERT(announced_application_window_contract.max_number_of_research_to_grant > 0, "Max number of research must be greater than 0");
      FC_ASSERT(announced_application_window_contract.end_date > announced_application_window_contract.start_date, "Grant applications apply period is not valid");

      is_validated = true;
  }

  else if (grant_contract.which() == grant_contract_details::tag<funding_opportunity_announcement_contract_v1_0_0_type>::value) 
  {
    const auto funding_opportunity_announcement_contract = grant_contract.get<funding_opportunity_announcement_contract_v1_0_0_type>();
    
    FC_ASSERT(funding_opportunity_announcement_contract.funding_opportunity_number.size() > 0, "Funding opportunity number is not specified");
    FC_ASSERT(fc::is_utf8(funding_opportunity_announcement_contract.funding_opportunity_number), "Funding opportunity number is not valid UTF-8 string");

    for (auto& pair : funding_opportunity_announcement_contract.additional_info)
    {
        FC_ASSERT(fc::is_utf8(pair.first), "Info key ${key} is not valid UTF-8 string", ("key", pair.first));
        FC_ASSERT(fc::is_utf8(pair.second), "Info value ${val} is not valid UTF-8 string", ("val", pair.second));
    }

    FC_ASSERT(funding_opportunity_announcement_contract.officers.count(grantor) != 0, 
      "Funding opportunity officers list should include the grantor ${1}", ("1", grantor));

    FC_ASSERT(funding_opportunity_announcement_contract.award_ceiling <= amount, "Award ceiling amount must be less than total amount");
    FC_ASSERT(funding_opportunity_announcement_contract.award_floor <= funding_opportunity_announcement_contract.award_ceiling, "Award floor must be less than total amount");
    FC_ASSERT(funding_opportunity_announcement_contract.expected_number_of_awards > 0, "Expected number of awards must be specified");
    FC_ASSERT(funding_opportunity_announcement_contract.close_date > funding_opportunity_announcement_contract.open_date, "Close date must be greater than open date");

    is_validated = true;
  }

  else if (grant_contract.which() == grant_contract_details::tag<discipline_supply_announcement_contract_v1_0_0_type>::value)
  {
    const auto discipline_supply_announcement_contract = grant_contract.get<discipline_supply_announcement_contract_v1_0_0_type>();

    FC_ASSERT(target_disciplines.size() == 1, "Must be only 1 target discipline in discipline supply.");
    FC_ASSERT(discipline_supply_announcement_contract.end_time > discipline_supply_announcement_contract.start_time, "Invalid discipline supply duration.");
    FC_ASSERT(discipline_supply_announcement_contract.content_hash.size() > 0, "Content hash must be specified");
    FC_ASSERT(fc::is_utf8(discipline_supply_announcement_contract.content_hash), "Content hash is not valid UTF8 string");

    for (auto& pair : discipline_supply_announcement_contract.additional_info)
    {
        FC_ASSERT(fc::is_utf8(pair.first), "Info key ${key} is not valid UTF-8 string", ("key", pair.first));
        FC_ASSERT(fc::is_utf8(pair.second), "Info value ${val} is not valid UTF-8 string", ("val", pair.second));
    }

    is_validated = true;
  }

  FC_ASSERT(is_validated, "Grant contract details are not validated");
}

fc::optional<grant_contract_details> create_grant_operation::get_grant_contract() const
{
    fc::optional<grant_contract_details> grant_contract;
    auto itr = std::find_if(details.begin(), details.end(),
      [&](const grant_contract_details& detail) {
        return 
            detail.which() == grant_contract_details::tag<announced_application_window_contract_v1_0_0_type>::value || 
            detail.which() == grant_contract_details::tag<funding_opportunity_announcement_contract_v1_0_0_type>::value ||
            detail.which() == grant_contract_details::tag<discipline_supply_announcement_contract_v1_0_0_type>::value;
    });

    if (itr != details.end())
    {
        grant_contract = *itr;
    }

    return grant_contract;
}


void create_grant_operation::validate() const
{
    validate_account_name(grantor);
    FC_ASSERT(amount > asset(0, amount.symbol), "Amount is required");
    FC_ASSERT(target_disciplines.size() != 0 && target_disciplines.count(0) == 0, "Disciplines list is required and should not contain 0");
    FC_ASSERT(details.size() != 0, "Grant contract details are required");
    validate_grant_contract_permitted_details(type, details);

    const auto grant_contract_opt = get_grant_contract();
    FC_ASSERT(grant_contract_opt.valid(), "Grant contract details are not required");
    validate_grant_contract(*grant_contract_opt, amount, grantor, target_disciplines);
}


} /* deip::protocol */
} /* protocol */

namespace fc {

  std::string grant_contract_detail_name_from_type(const std::string& type_name)
  {
    auto start = type_name.find_last_of(':') + 1;
    auto end = type_name.find_last_of('_');
    auto result = type_name.substr(start, end - start);
    return result;
  }

}


DEFINE_GRANT_CONTRACT_DETAILS_TYPE(deip::protocol::grant_contract_details)