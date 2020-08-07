#include <deip/protocol/deip_operations.hpp>
#include <locale>
#include <fc/log/logger.hpp>
#include <fc/io/json.hpp>
#include <fc/io/sstream.hpp>
#include <functional>
#include <numeric>
#include "boost/date_time/posix_time/posix_time.hpp"

namespace deip {
namespace protocol {

using boost::posix_time::ptime;
using boost::posix_time::time_period;
using boost::posix_time::from_iso_string;

struct phase_period_validator
{

    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
    }

    void operator()(const apply_phase_type& apply_phase)
    {
        set_phase_period(apply_phase_period, apply_phase.start_time, apply_phase.end_time);
    }

    void operator()(const await_review_phase_type& await_review_phase)
    {
        set_phase_period(await_review_phase_period, await_review_phase.start_time, await_review_phase.end_time);
    }

    void operator()(const review_phase_type& review_phase)
    {
        set_phase_period(review_phase_period, review_phase.start_time, review_phase.end_time);
    }

    void operator()(const decision_phase_type& decision_phase)
    {
        set_phase_period(decision_phase_period, decision_phase.start_time, decision_phase.end_time);
    }

    void set_phase_period(fc::optional<time_period>& period, time_point_sec start_point, time_point_sec end_point)
    {
        const string& start_timestamp = start_point.to_non_delimited_iso_string();
        const string& end_timestamp = end_point.to_non_delimited_iso_string();

        ptime start_time(from_iso_string(start_timestamp));
        ptime end_time(from_iso_string(end_timestamp));

        period = time_period(start_time, end_time);
    }

    void validate() const
    {
        if (apply_phase_period.valid())
        {
            const time_period& apply_period = *apply_phase_period;

            if (await_review_phase_period.valid())
            {
                const time_period& await_review_period = *await_review_phase_period;
                FC_ASSERT(
                  apply_period.is_adjacent(await_review_period) ||
                  apply_period.begin() <= await_review_period.begin(), 
                  "'Apply phase' (${1}) can be prior to 'Awaiting review phase' (${2}) or include its start point",
                  ("1", to_simple_string(apply_period))("2", to_simple_string(await_review_period))
                );
            }

            if (review_phase_period.valid())
            {
                const time_period& review_period = *review_phase_period;
                FC_ASSERT(
                  apply_period.is_adjacent(review_period) || 
                  apply_period.begin() <= review_period.begin(),
                  "'Apply phase' (${1}) can be prior to 'Review phase' (${2}) or include its start point",
                  ("1", to_simple_string(apply_period))("2", to_simple_string(review_period))
                );
            }

            if (decision_phase_period.valid())
            {
                const time_period& decision_period = *decision_phase_period;
                FC_ASSERT(
                  apply_period.is_adjacent(decision_period) || 
                  apply_period.begin() <= decision_period.begin(),
                  "'Apply phase' (${1}) can be prior to 'Decision phase' (${2}) or include its start point",
                  ("1", to_simple_string(apply_period))("2", to_simple_string(decision_period))
                );
            }
        }
    }

private:
    fc::optional<time_period> apply_phase_period;
    fc::optional<time_period> await_review_phase_period;
    fc::optional<time_period> review_phase_period;
    fc::optional<time_period> decision_phase_period;
};


struct stage_validator
{
    typedef void result_type;
    template <typename T> void operator()(const T& v) const
    {
    }
    
    void operator()(const assessment_stage& stage)
    {
        _stage = &stage;

        FC_ASSERT(_stage->phases.size() != 0, "Assessment stage should contain at least 1 phase");

        phase_period_validator period_validator;
        for (const assessment_stage_phase& phase : _stage->phases)
        {
            phase.visit(period_validator);
            phase.visit(*this);
        }

        period_validator.validate();
    }

    void operator()(const apply_phase_type& apply_phase) const
    {
        // fc::variants args = fc::json::variants_from_string(fn_args, fc::json::parse_type::strict_parser);

        // const auto& phase_itr = std::find_if(_stage->phases.begin(), _stage->phases.end(), [&](const assessment_stage_phase& p) {
        //     const auto phase = p.get<stage_phase>();
        //     return phase.start_time < apply_phase.start_time;
        // });
        

        // FC_ASSERT(phase_itr == _stage->phases.end(), "Apply phase should be the earliest one");
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

private:
    const assessment_stage* _stage;
    
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
