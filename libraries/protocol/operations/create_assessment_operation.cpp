#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <functional>
#include <numeric>

namespace deip {
namespace protocol {

void create_assessment_operation::validate() const
{
  
}

} /* deip::protocol */
} /* protocol */

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::apply_phase_option)
DEFINE_STATIC_VARIANT_TYPE(deip::protocol::await_review_phase_option)
DEFINE_STATIC_VARIANT_TYPE(deip::protocol::review_phase_option)
DEFINE_STATIC_VARIANT_TYPE(deip::protocol::decision_phase_option)

DEFINE_STATIC_VARIANT_TYPE(deip::protocol::assessment_stage_phase)
