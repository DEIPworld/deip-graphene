#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void inline validate_distribution_model(const grant_distribution_models distribution_model,
                                        const asset& amount,
                                        const account_name_type& grantor,
                                        const flat_set<external_id_type>& target_disciplines)
{  
  bool is_validated = false;

  if (distribution_model.which() == grant_distribution_models::tag<announced_application_window_contract_type>::value) 
  {
      const auto contract = distribution_model.get<announced_application_window_contract_type>();

      validate_160_bits_hexadecimal_string(contract.review_committee_id);

      FC_ASSERT(contract.min_number_of_positive_reviews >= 0, "Min number of positive reviews must be equal or greater than 0");
      FC_ASSERT(contract.min_number_of_applications > 0, "Min number of grant applications must be greater than 0");
      FC_ASSERT(contract.min_number_of_applications >= contract.max_number_of_research_to_grant, "Min number of grant applications must be equal or greater than max number of research");
      FC_ASSERT(contract.max_number_of_research_to_grant > 0, "Max number of research must be greater than 0");
      FC_ASSERT(contract.open_date > contract.close_date, "Grant applications apply period is not valid");

      is_validated = true;
  }

  else if (distribution_model.which() == grant_distribution_models::tag<funding_opportunity_announcement_contract_type>::value) 
  {
      const auto contract = distribution_model.get<funding_opportunity_announcement_contract_type>();

      validate_160_bits_hexadecimal_string(contract.organization_id);
      validate_160_bits_hexadecimal_string(contract.review_committee_id);
      validate_160_bits_hexadecimal_string(contract.treasury_id);

      for (auto& pair : contract.additional_info)
      {
          FC_ASSERT(fc::is_utf8(pair.first), "Info key ${1} is not valid UTF-8 string", ("1", pair.first));
          FC_ASSERT(fc::is_utf8(pair.second), "Info value ${1} is not valid UTF-8 string", ("1", pair.second));
      }

      FC_ASSERT(contract.officers.count(grantor) != 0, "Funding opportunity officers list should include the grantor ${1}", ("1", grantor));
      FC_ASSERT(contract.award_ceiling <= amount, "Award ceiling amount must be less than total amount");
      FC_ASSERT(contract.award_floor <= contract.award_ceiling, "Award floor must be less than total amount");
      FC_ASSERT(contract.expected_number_of_awards > 0, "Expected number of awards must be specified");
      FC_ASSERT(contract.close_date > contract.open_date, "Close date must be greater than open date");

      is_validated = true;
  }

  else if (distribution_model.which() == grant_distribution_models::tag<discipline_supply_announcement_contract_type>::value)
  {
    const auto contract = distribution_model.get<discipline_supply_announcement_contract_type>();

    FC_ASSERT(target_disciplines.size() == 1, "Must be only 1 target discipline in discipline supply.");
    FC_ASSERT(contract.end_time > contract.start_time, "Invalid discipline supply duration.");
    FC_ASSERT(contract.content_hash.size() > 0, "Content hash must be specified");
    FC_ASSERT(fc::is_utf8(contract.content_hash), "Content hash is not valid UTF8 string");

    for (auto& pair : contract.additional_info)
    {
        FC_ASSERT(fc::is_utf8(pair.first), "Info key ${key} is not valid UTF-8 string", ("key", pair.first));
        FC_ASSERT(fc::is_utf8(pair.second), "Info value ${val} is not valid UTF-8 string", ("val", pair.second));
    }

    is_validated = true;
  }

  FC_ASSERT(is_validated, "Grant contract details are not validated");
}


void create_grant_operation::validate() const
{
    validate_account_name(grantor);
    FC_ASSERT(amount > asset(0, amount.symbol), "Amount is required");
    FC_ASSERT(target_disciplines.size() != 0, "Disciplines list is required");

    validate_distribution_model(distribution_model, amount, grantor, target_disciplines);
}


} /* deip::protocol */
} /* protocol */

namespace fc {

  std::string grant_distribution_model_name_from_type(const std::string& type_name)
  {
    auto start = type_name.find_last_of(':') + 1;
    auto end = type_name.find_last_of('_');
    auto result = type_name.substr(start, end - start);
    return result;
  }

}


DEFINE_GRANT_DISTRIBUTION_MODELS_TYPE(deip::protocol::grant_distribution_models)