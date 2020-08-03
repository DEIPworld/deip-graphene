#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <fc/io/json.hpp>
#include <fc/io/sstream.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_assessment_operation::validate() const
{
    // fc::variants args = fc::json::variants_from_string(fn_args, fc::json::parse_type::strict_parser);
}

} // namespace protocol
} // namespace deip

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::apply_phase_option)
DEFINE_STATIC_VARIANT_TYPE(deip::protocol::await_review_phase_option)
DEFINE_STATIC_VARIANT_TYPE(deip::protocol::review_phase_option)
DEFINE_STATIC_VARIANT_TYPE(deip::protocol::decision_phase_option)

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::assessment_stage_phase)
