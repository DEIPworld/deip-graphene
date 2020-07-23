#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void validate_assessment_model(const assessment_models& assessment_model)
{
    bool is_validated = false;

    const std::set<int>& assessment_models =
    {
        assessment_models::tag<binary_scoring_assessment_model_type>::value,
        assessment_models::tag<multicriteria_scoring_assessment_model_type>::value
    };

    FC_ASSERT(std::find(assessment_models.begin(), assessment_models.end(), assessment_model.which()) != assessment_models.end(), "Provided assessment model is not defined.");

    if (assessment_model.which() == assessment_models::tag<binary_scoring_assessment_model_type>::value)
    {
        is_validated = true;
    }

    else if (assessment_model.which() == assessment_models::tag<multicriteria_scoring_assessment_model_type>::value)
    {
        const auto model = assessment_model.get<multicriteria_scoring_assessment_model_type>();
        FC_ASSERT(model.scores.size() >= DEIP_MIN_NUMBER_OF_REVIEW_CRITERIAS, "Must be at least ${num} review criterias", ("num", DEIP_MIN_NUMBER_OF_REVIEW_CRITERIAS));

        FC_ASSERT(std::find_if(model.scores.begin(), model.scores.end(),
                [&](const std::pair<uint16_t, uint16_t>& score) {
                    const assessment_criteria criteria = static_cast<assessment_criteria>(score.first);
                    return criteria <assessment_criteria::FIRST || criteria > assessment_criteria::LAST; }) == model.scores.end(),
                        "Invalid criteria.");
        FC_ASSERT(std::find_if(model.scores.begin(), model.scores.end(),
                [&](const std::pair<uint16_t, uint16_t>& score) {return score.second < DEIP_MIN_REVIEW_CRITERIA_SCORE || score.second > DEIP_MAX_REVIEW_CRITERIA_SCORE; }) == model.scores.end(),
                        "Invalid score.");

        is_validated = true;
    }

    FC_ASSERT(is_validated, "Assessment model is not valid.");
}

void create_review_operation::validate() const
{
    validate_account_name(author);
    validate_160_bits_hexadecimal_string(external_id);
    validate_160_bits_hexadecimal_string(research_content_external_id);
    FC_ASSERT(!content.empty(), "Content cannot be empty");
    FC_ASSERT(weight.amount > 0 && weight.amount <= DEIP_100_PERCENT, "Weight should be in 1% to 100% range");
    FC_ASSERT(disciplines.size() != 0, "Disciplines list must include entries");

    validate_assessment_model(assessment_model);

    for (const auto& id : disciplines)
    {
        validate_160_bits_hexadecimal_string(id);
    }
}

} /* deip::protocol */
} /* protocol */

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::assessment_models)