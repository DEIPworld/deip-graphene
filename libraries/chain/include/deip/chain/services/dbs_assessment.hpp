#pragma once

#include "dbs_base_impl.hpp"
#include <deip/chain/schema/assessment_object.hpp>
#include <deip/chain/schema/assessment_stage_object.hpp>
#include <deip/chain/schema/assessment_stage_phase_object.hpp>

namespace deip {
namespace chain {

class dbs_assessment : public dbs_base
{
    friend class dbservice_dbs_factory;

    dbs_assessment() = delete;

protected:
    explicit dbs_assessment(database& db);

public:
    using assessment_optional_ref_type = fc::optional<std::reference_wrapper<const assessment_object>>;
    using assessment_stage_optional_ref_type = fc::optional<std::reference_wrapper<const assessment_stage_object>>;
    using assessment_stage_phase_optional_ref_type = fc::optional<std::reference_wrapper<const assessment_stage_phase_object>>;

    using assessment_ref_type = std::vector<std::reference_wrapper<const assessment_object>>;
    using assessment_stage_ref_type = std::vector<std::reference_wrapper<const assessment_stage_object>>;
    using assessment_stage_phase_ref_type = std::vector<std::reference_wrapper<const assessment_stage_phase_object>>;

    const assessment_object& create_assessment(const external_id_type& external_id, const account_name_type& creator);
};

} // namespace chain
} // namespace deip