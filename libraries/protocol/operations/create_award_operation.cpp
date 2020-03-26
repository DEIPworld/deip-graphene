#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void inline validate_awardees(const flat_set<awardee_type>& awardees, const asset& award)
{
    FC_ASSERT(awardees.size() > 0, "Awardees list cannot be empty.");

    std::map<account_name_type, share_type> incoming_values;
    std::map<account_name_type, share_type> outcoming_values;

    bool is_main_awardee_set = false;

    for (auto& awardee : awardees)
    {
        validate_account_name(awardee.awardee);
        FC_ASSERT(awardee.research_id >= 0, "Research id cant be less than a 0");
        FC_ASSERT(awardee.university_id >= 0, "University id cant be less than a 0");
        FC_ASSERT(awardee.university_overhead >= 0 && awardee.university_overhead <= DEIP_1_PERCENT * 50, "University overhead must be in 0% to 50% range");
        FC_ASSERT(awardee.award.symbol == award.symbol, "Asset symbols don't match.");
        if (is_main_awardee_set)
            FC_ASSERT(awardee.source.valid(), "Must be only 1 main awardee.");

        share_type university_fee = (awardee.award.amount * awardee.university_overhead) / DEIP_100_PERCENT;
        share_type award_with_fee = awardee.award.amount + university_fee;

        incoming_values[awardee.awardee] += award_with_fee;

        if (awardee.source.valid()) {
            outcoming_values[*awardee.source] += award_with_fee;
        } else {
            is_main_awardee_set = true;
        }
    }

    for (const std::pair<account_name_type, share_type>& outcoming : outcoming_values)
    {
        FC_ASSERT(incoming_values.find(outcoming.first) != incoming_values.end(), "Wrong list structure(trying to give away money without receiving)");
    }

    for (const std::pair<account_name_type, share_type>& incoming : incoming_values)
    {
        auto it = outcoming_values.find(incoming.first);
        if (it != outcoming_values.end())
        {
            FC_ASSERT(it->second <= incoming_values[it->first], "Wrong list structure(trying to give away money more than received)");
        }
    }
}

void create_award_operation::validate() const
{
    validate_account_name(creator);
    FC_ASSERT(funding_opportunity_id >= 0, "Funding opportunity id cant be less than a 0");
    FC_ASSERT(award.amount > 0, "Award amount must be > 0.");
    validate_awardees(awardees, award);
}


} /* deip::protocol */
} /* protocol */