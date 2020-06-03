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
        assessment_models::tag<binary_scoring_assessment_model_v1_0_0_type>::value,
        assessment_models::tag<multicriteria_scoring_assessment_model_v1_0_0_type>::value
    };

    FC_ASSERT(std::find(assessment_models.begin(), assessment_models.end(), assessment_model.which()) != assessment_models.end(), "Provided assessment model is not defined.");

    if (assessment_model.which() == assessment_models::tag<binary_scoring_assessment_model_v1_0_0_type>::value)
    {
        is_validated = true;
    }
    else if (assessment_model.which() == assessment_models::tag<multicriteria_scoring_assessment_model_v1_0_0_type>::value)
    {
        const auto model = assessment_model.get<multicriteria_scoring_assessment_model_v1_0_0_type>();
        FC_ASSERT(model.scores.size() >= DEIP_MIN_NUMBER_OF_REVIEW_CRITERIAS, "Must be at least ${num} review criterias", ("num", DEIP_MIN_NUMBER_OF_REVIEW_CRITERIAS));

        FC_ASSERT(std::find_if(model.scores.begin(), model.scores.end(),
                [&](const std::pair<uint16_t, uint16_t>& score) {
                    const assessment_criteria criteria = static_cast<assessment_criteria>(score.first);
                    return criteria <assessment_criteria::FIRST || criteria > assessment_criteria::LAST; }) == model.scores.end(),
                        "Invalid criteria.");
        FC_ASSERT(std::find_if(model.scores.begin(), model.scores.end(),
                [&](const std::pair<uint16_t, uint16_t>& score) {return score.second < DEIP_MIN_REVIEW_MARK || score.second > DEIP_MAX_REVIEW_MARK; }) == model.scores.end(),
                        "Invalid score.");

        is_validated = true;
    }

    FC_ASSERT(is_validated, "Assessment model is not valid.");
}

void create_review_operation::validate() const
{
    validate_account_name(author);
    FC_ASSERT(research_content_id >= 0, "Id cant be less than a 0");
    FC_ASSERT(!content.empty(), "Content cannot be empty");
    FC_ASSERT(weight > 0 && weight <= DEIP_100_PERCENT, "Weight should be in 1% to 100% range");
    validate_assessment_model(assessment_model);
}

} /* deip::protocol */
} /* protocol */

namespace fc {

  std::string assessment_model_name_from_type(const std::string& type_name)
  {
    auto start = type_name.find_last_of(':') + 1;
    auto end = type_name.find_last_of('_');
    auto result = type_name.substr(start, end - start);
    return result;
  }

}

DEFINE_ASSESSMENT_MODELS_TYPE(deip::protocol::assessment_models)