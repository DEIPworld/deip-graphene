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

    share_type plan_certs = 0;
    share_type plan_sharings = 0;
    share_type plan_contracts = 0;

    uint16_t period;
    fc::time_point_sec billing_date;

    void validate() const
    {
        FC_ASSERT(plan_certs >= 0, "Certs amount cant be < 0.");
        FC_ASSERT(plan_sharings >= 0, "Sharings amount cant be < 0.");
        FC_ASSERT(plan_contracts >= 0, "Contracts amount cant be < 0.");
    }
};

struct additional_subscription_limits_data_type : base_subscription_data_type
{
    share_type additional_certs = 0;
    share_type additional_sharings = 0;
    share_type additional_contracts = 0;

    void validate() const
    {
        FC_ASSERT(additional_certs >= 0, "Additional certs amount cant be < 0.");
        FC_ASSERT(additional_sharings >= 0, "Additional sharings amount cant be < 0.");
        FC_ASSERT(additional_contracts >= 0, "Additional contracts amount cant be < 0.");
    }
};

};
}

FC_REFLECT(deip::chain::subscription_data_type, (external_plan_id)(plan_certs)(plan_sharings)(plan_contracts)(period)(billing_date))

FC_REFLECT(deip::chain::additional_subscription_limits_data_type, (additional_certs)(additional_sharings)(additional_contracts))