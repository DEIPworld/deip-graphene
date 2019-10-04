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
    optional<std::string> external_id;
    optional<std::string> external_plan_id;

    optional<share_type> file_certificate_quota;
    optional<share_type> nda_contract_quota ;
    optional<share_type> nda_protected_file_quota;

    optional<share_type> current_file_certificate_quota_units;
    optional<share_type> current_nda_contract_quota_units ;
    optional<share_type> current_nda_protected_file_quota_units;

    optional<share_type> extra_file_certificate_quota_units;
    optional<share_type> extra_nda_contract_quota_units;
    optional<share_type> extra_nda_protected_file_quota_units;

    optional<uint16_t> period;
    optional<fc::time_point_sec> billing_date;

    void validate() const
    {

    }
};

struct additional_subscription_limits_data_type : base_subscription_data_type
{
    share_type extra_file_certificate_quota_units = 0;
    share_type extra_nda_contract_quota_units = 0;
    share_type extra_nda_protected_file_quota_units = 0;

    void validate() const
    {
//        FC_ASSERT(extra_file_certificate_quota_units >= 0, "File certificate quota extra units must be specified");
//        FC_ASSERT(extra_nda_contract_quota_units >= 0, "NDA contract quota extra units must be specified");
//        FC_ASSERT(extra_nda_protected_file_quota_units >= 0, "NDA protected file quota extra units must be specified");
    }
};

};
}

FC_REFLECT(deip::chain::subscription_data_type, (external_id)(external_plan_id)(file_certificate_quota)(nda_contract_quota)(nda_protected_file_quota)
                                                (current_file_certificate_quota_units)(current_nda_contract_quota_units)(current_nda_protected_file_quota_units)
                                                (extra_file_certificate_quota_units)(extra_nda_contract_quota_units)(extra_nda_protected_file_quota_units)
                                                (period)(billing_date))

FC_REFLECT(deip::chain::additional_subscription_limits_data_type, (extra_file_certificate_quota_units)(extra_nda_contract_quota_units)(extra_nda_protected_file_quota_units))