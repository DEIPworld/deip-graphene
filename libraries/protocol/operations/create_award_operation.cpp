#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void inline validate_awardees(const vector<subawardee_type>& subawardees, const account_name_type& awardee, const asset& award, const percent& university_overhead)
{
    const asset top_subawards_amount = std::accumulate(
      subawardees.begin(), subawardees.end(), asset(0, award.symbol),
      [&](asset acc, const subawardee_type& s) {
          return s.source == awardee ? acc + s.subaward : acc;
      });

    const asset university_fee = asset(((award.amount * university_overhead.amount) / DEIP_100_PERCENT), award.symbol);

    FC_ASSERT(award > (top_subawards_amount + university_fee), 
      "Total subawards amount with overhead fee should be less than total award amount");

    FC_ASSERT(!std::any_of(subawardees.begin(), subawardees.end(),
      [&](const subawardee_type& s) { return s.subawardee == awardee; }), 
      "Main awardee ${1} should not be specified in subawardees list", 
      ("1", awardee));

    if (subawardees.size() != 0)
    {
        FC_ASSERT(std::any_of(subawardees.begin(), subawardees.end(),
          [&](const subawardee_type& s) { return s.source == awardee; }), 
          "At least 1 subawardee should refer to the main awardee ${1}", 
          ("1", awardee));
    }

    for (auto& subaward : subawardees)
    {
        validate_account_name(subaward.subawardee);
        // validate_160_bits_hexadecimal_string(subaward.subaward_number);
        FC_ASSERT(subaward.subaward.amount > 0, "Award amount must be specified");
        FC_ASSERT(subaward.subaward.symbol == award.symbol, "Subaward asset does not match the award asset");

        FC_ASSERT(std::count_if(subawardees.begin(), subawardees.end(),
          [&](const subawardee_type& s) { return subaward.subawardee == s.subawardee; }) == 1, 
          "Duplicated subawardee ${1} found in the list", 
          ("1", subaward.subawardee));

        FC_ASSERT(std::none_of(subawardees.begin(), subawardees.end(),
          [&](const subawardee_type& s) { return subaward.source == s.subawardee && s.source == subaward.subawardee; }), 
          "Circular subaward reference detected for ${1}", 
          ("1", subaward.subawardee));

        if (subaward.source != awardee)
        {
            FC_ASSERT(std::any_of(subawardees.begin(), subawardees.end(),
              [&](const subawardee_type& s) { return subaward.source == s.subawardee; }), 
              "Subaward source ${1} for ${2} is not presented in the list", 
              ("1", subaward.source)("2", subaward.subawardee));

            const asset subsequent_subawards_amount = std::accumulate(
              subawardees.begin(), subawardees.end(), asset(0, award.symbol),
              [&](asset acc, const subawardee_type& s) { 
                return s.source == subaward.subawardee ? acc + s.subaward : acc;
              });

            FC_ASSERT(subaward.subaward > subsequent_subawards_amount,
              "Subaward ${1} amount ${2} is less than its subawards total amount ${3}",
              ("1", subaward.subawardee)("2", subaward.subaward)("3", subsequent_subawards_amount));
        }
    }
}

void create_award_operation::validate() const
{
    validate_account_name(creator);
    validate_account_name(awardee);
    // validate_160_bits_hexadecimal_string(funding_opportunity_number);
    // validate_160_bits_hexadecimal_string(award_number);
    FC_ASSERT(award.amount > 0, "Award amount must be > 0.");
    validate_160_bits_hexadecimal_string(research_external_id);
    validate_160_bits_hexadecimal_string(university_external_id);

    const auto& min_university_overhead_share = percent(0);
    const auto& max_university_overhead_share = percent(DEIP_1_PERCENT * 50);
    FC_ASSERT(university_overhead >= min_university_overhead_share && university_overhead <= max_university_overhead_share,
              "Percent for dropout compensation should be in ${1} to ${2} range. Provided value: ${3}.",
              ("1", min_university_overhead_share)("2", max_university_overhead_share)("3", university_overhead));

    validate_awardees(subawardees, awardee, award, university_overhead);
}


} /* deip::protocol */
} /* protocol */