#pragma once

#include "deip_object_types.hpp"
#include <fc/utf8.hpp>

using namespace deip::protocol;

namespace deip {
namespace chain {

struct base_subscription_data_type
{
    virtual void validate() const = 0;
};

struct subscription_data_type : base_subscription_data_type
{
    uint16_t external_plan_id;

    share_type file_certificate_quota = 0;
    share_type nda_contract_quota = 0;
    share_type nda_protected_file_quota = 0;

    uint16_t period;
    fc::time_point_sec billing_date;

    void validate() const
    {
        FC_ASSERT(file_certificate_quota >= 0, "File certificate quota must be specified");
        FC_ASSERT(nda_contract_quota >= 0, "NDA contract quota must be specified");
        FC_ASSERT(nda_protected_file_quota >= 0, "NDA protected file quota must be specified");
    }
};

struct additional_subscription_limits_data_type : base_subscription_data_type
{
    share_type extra_file_certificate_quota_units = 0;
    share_type extra_nda_contract_quota_units = 0;
    share_type extra_nda_protected_file_quota_units = 0;

    void validate() const
    {
        FC_ASSERT(extra_file_certificate_quota_units >= 0, "File certificate quota extra units must be specified");
        FC_ASSERT(extra_nda_contract_quota_units >= 0, "NDA contract quota extra units must be specified");
        FC_ASSERT(extra_nda_protected_file_quota_units >= 0, "NDA protected file quota extra units must be specified");
    }
};

};
}

FC_REFLECT(deip::chain::subscription_data_type, (external_plan_id)(file_certificate_quota)(nda_contract_quota)(nda_protected_file_quota)(period)(billing_date))

FC_REFLECT(deip::chain::additional_subscription_limits_data_type, (extra_file_certificate_quota_units)(extra_nda_contract_quota_units)(extra_nda_protected_file_quota_units))