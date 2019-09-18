#include <deip/chain/services/dbs_subscription.hpp>
#include <deip/chain/database/database.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace deip {
namespace chain {

dbs_subscription::dbs_subscription(database& db)
    : _base_type(db)
{
}

const subscription_object& dbs_subscription::create(const std::string& json_data, const research_group_id_type& research_group_id)
{
    subscription_data_type data = get_data(json_data);

    const subscription_object& subscription = db_impl().create<subscription_object>([&](subscription_object& s_o) {
        s_o.research_group_id = research_group_id;

        s_o.external_plan_id = data.external_plan_id;
        s_o.remained_certs = data.plan_certs;
        s_o.remained_sharings = data.plan_sharings;
        s_o.remained_contracts = data.plan_contracts;

        s_o.plan_certs = data.plan_certs;
        s_o.plan_sharings = data.plan_sharings;
        s_o.plan_contracts = data.plan_contracts;

        s_o.period = static_cast<billing_period>(data.period);
        s_o.billing_date = data.billing_date;
        s_o.first_billing_date = data.billing_date;

    });

    return subscription;
}

const subscription_object& dbs_subscription::get(const subscription_id_type& id) const
{
    try {
        return db_impl().get<subscription_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

void dbs_subscription::set_new_billing_date(const subscription_object& subscription)
{
    using namespace boost::gregorian;
    using namespace boost::posix_time;

    std::string iso = subscription.first_billing_date.to_iso_string();
    ptime t = from_time_t(subscription.first_billing_date.sec_since_epoch());

    db_impl().modify(subscription, [&](subscription_object &s_o) {
        s_o.month_subscriptions_count++;
        t += months(s_o.month_subscriptions_count);
        s_o.billing_date = fc::time_point_sec(to_time_t(t));
        s_o.remained_certs = s_o.plan_certs;
        s_o.remained_sharings = s_o.plan_sharings;
        s_o.remained_contracts = s_o.plan_contracts;
    });
}

} // namespace chain
} // namespace deip

