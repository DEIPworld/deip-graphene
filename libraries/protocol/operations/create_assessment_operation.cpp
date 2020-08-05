#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <fc/io/json.hpp>
#include <fc/io/sstream.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

struct stage_validator
{
    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
    }

    void operator()(const assessment_stage& stage) const
    {
        for (const assessment_stage_phase& phase : stage.phases)
        {
            phase.visit(*this);
        }
    }

    void operator()(const apply_phase_type& apply_phase) const
    {
        // fc::variants args = fc::json::variants_from_string(fn_args, fc::json::parse_type::strict_parser);
    }

    void operator()(const await_review_phase_type& await_review_phase) const
    {

    }

    void operator()(const review_phase_type& review_phase) const
    {

    }

    void operator()(const decision_phase_type& decision_phase) const
    {

    }
};


void create_assessment_operation::validate() const
{
    FC_ASSERT(stages.size() != 0, "Assessment should have at least 1 stage");
    stage_validator s_validator;

    for (const auto& stage : stages)
    {
        s_validator(stage);
    }
}

} // namespace protocol
} // namespace deip

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::apply_phase_option)
DEFINE_STATIC_VARIANT_TYPE(deip::protocol::await_review_phase_option)
DEFINE_STATIC_VARIANT_TYPE(deip::protocol::review_phase_option)
DEFINE_STATIC_VARIANT_TYPE(deip::protocol::decision_phase_option)

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::assessment_stage_phase)
