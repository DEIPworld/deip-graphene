#pragma once

#include "deip_object_types.hpp"
#include <fc/utf8.hpp>

using namespace deip::protocol;

namespace deip {
namespace chain {

struct subscription_data_type
{
    uint16_t external_plan_id;

    share_type plan_certs = 0;
    share_type plan_sharings = 0;
    share_type plan_contracts = 0;

    uint16_t period;
    fc::time_point_sec billing_date;
};

struct additional_subscription_limits_data_type
{
    share_type additional_certs = 0;
    share_type additional_sharings = 0;
    share_type additional_contracts = 0;
};

};
}

FC_REFLECT(deip::chain::subscription_data_type, (external_plan_id)(plan_certs)(plan_sharings)(plan_contracts)(period)(billing_date))

FC_REFLECT(deip::chain::additional_subscription_limits_data_type, (additional_certs)(additional_sharings)(additional_contracts))