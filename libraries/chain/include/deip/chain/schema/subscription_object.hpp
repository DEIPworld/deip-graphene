#pragma once

#include "deip_object_types.hpp"

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {

enum billing_period : uint16_t
{
    month = 1,
    year = 2
};

enum subscription_status : uint16_t
{
    subscription_active = 1,
    subscription_cancelled = 2,
    subscription_expired = 3
};

class subscription_object : public object<subscription_object_type, subscription_object>
{
    subscription_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    subscription_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    subscription_id_type id;
    research_group_id_type research_group_id;
    
    account_name_type owner;

    share_type remained_certs = 0;
    share_type remained_sharings = 0;
    share_type remained_contracts = 0;

    uint16_t external_plan_id;

    share_type plan_certs = 0;
    share_type plan_sharings = 0;
    share_type plan_contracts = 0;

    share_type additional_certs = 0;
    share_type additional_sharings = 0;
    share_type additional_contracts = 0;

    billing_period period;
    fc::time_point_sec billing_date;

    subscription_status status = subscription_status::subscription_active;

    fc::time_point_sec first_billing_date;
    uint16_t month_subscriptions_count = 0;
};

struct by_research_group;
struct by_owner;
struct by_research_group_and_owner;
struct by_billing_date;
struct by_status;

typedef multi_index_container<subscription_object,
                indexed_by<ordered_unique<tag<by_id>,
                           member<subscription_object,
                                   subscription_id_type,
                                  &subscription_object::id>>,
                    ordered_unique<tag<by_research_group>,
                            member<subscription_object,
                                    research_group_id_type,
                                   &subscription_object::research_group_id>>,
                    ordered_non_unique<tag<by_owner>,
                            member<subscription_object,
                                    account_name_type,
                                   &subscription_object::owner>>,
                    ordered_unique<tag<by_research_group_and_owner>,
                            composite_key<subscription_object,
                                 member<subscription_object,
                                         research_group_id_type,
                                        &subscription_object::research_group_id>,
                                 member<subscription_object,
                                         account_name_type,
                                        &subscription_object::owner>>>,               
                    ordered_non_unique<tag<by_billing_date>,
                            member<subscription_object,
                                    fc::time_point_sec,
                                   &subscription_object::billing_date>>,
                    ordered_non_unique<tag<by_status>,
                            member<subscription_object,
                                    subscription_status,
                                   &subscription_object::status>>>,
                allocator<subscription_object>>
        subscription_index;
    }
}

FC_REFLECT_ENUM(deip::chain::billing_period, (month)(year))

FC_REFLECT_ENUM(deip::chain::subscription_status, (subscription_active)(subscription_cancelled)(subscription_expired))

FC_REFLECT( deip::chain::subscription_object, (id)(research_group_id)(owner)(remained_certs)(remained_sharings)(remained_contracts)(external_plan_id)
                                              (plan_certs)(plan_sharings)(plan_contracts)(additional_certs)(additional_sharings)(additional_contracts)
                                              (period)(billing_date)(status)(first_billing_date)(month_subscriptions_count))

CHAINBASE_SET_INDEX_TYPE( deip::chain::subscription_object, deip::chain::subscription_index )