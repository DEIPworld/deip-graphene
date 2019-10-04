#include <deip/chain/services/dbs_subscription.hpp>
#include <deip/chain/database/database.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace deip {
namespace chain {

dbs_subscription::dbs_subscription(database& db)
    : _base_type(db)
{
}

const subscription_object& dbs_subscription::create(const std::string& json_data, const research_group_id_type& research_group_id, const account_name_type& owner)
{
    const subscription_data_type data = get_data<subscription_data_type>(json_data);

    FC_ASSERT(data.file_certificate_quota.valid(), "File certificate quota field is not specified.");
    FC_ASSERT(data.nda_contract_quota.valid(), "Nda contract quota field is not specified.");
    FC_ASSERT(data.nda_protected_file_quota.valid(), "Nda protected file quota field is not specified.");
    FC_ASSERT(data.external_plan_id.valid(), "External plan field is not specified.");
    FC_ASSERT(data.external_id.valid(), "External id field is not specified.");
    FC_ASSERT(data.period.valid(), "Period field is not specified.");
    FC_ASSERT(data.billing_date.valid(), "Billing date field is not specified.");

    const subscription_object& subscription = db_impl().create<subscription_object>([&](subscription_object& s_o) {
        s_o.research_group_id = research_group_id;
        s_o.owner = owner;

        fc::from_string(s_o.external_id, *data.external_id);
        fc::from_string(s_o.external_plan_id, *data.external_plan_id);

        s_o.file_certificate_quota = *data.file_certificate_quota;
        s_o.nda_contract_quota = *data.nda_contract_quota;
        s_o.nda_protected_file_quota = *data.nda_protected_file_quota;

        s_o.current_file_certificate_quota_units = *data.file_certificate_quota;
        s_o.current_nda_contract_quota_units = *data.nda_contract_quota;
        s_o.current_nda_protected_file_quota_units = *data.nda_protected_file_quota;

        s_o.status = subscription_status::subscription_active;
        s_o.period = static_cast<billing_period>(*data.period);
        s_o.billing_date = *data.billing_date;
        s_o.first_billing_date = *data.billing_date;
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

const subscription_object& dbs_subscription::get_by_research_group(const research_group_id_type& research_group_id)
{
    try {
        return db_impl().get<subscription_object, by_research_group>(research_group_id);
    }
    FC_CAPTURE_AND_RETHROW((research_group_id))
}

dbs_subscription::subscription_refs_type
dbs_subscription::get_by_owner(const account_name_type& owner) const
{
    subscription_refs_type ret;

    auto it_pair = db_impl().get_index<subscription_index>().indicies().get<by_owner>().equal_range(owner);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_subscription::set_new_billing_date(const subscription_object& subscription)
{
    std::string iso = subscription.first_billing_date.to_iso_string();
    boost::posix_time::ptime t = boost::posix_time::from_time_t(subscription.first_billing_date.sec_since_epoch());

    db_impl().modify(subscription, [&](subscription_object &s_o) {
        s_o.month_subscriptions_count++;
        t += boost::gregorian::months(s_o.month_subscriptions_count);
        s_o.billing_date = fc::time_point_sec(to_time_t(t));
        s_o.current_file_certificate_quota_units = s_o.file_certificate_quota;
        s_o.current_nda_contract_quota_units = s_o.nda_contract_quota;
        s_o.current_nda_protected_file_quota_units = s_o.nda_protected_file_quota;
    });
}

void dbs_subscription::check_subscription_existence(const subscription_id_type& subscription_id) const
{
    const auto& idx = db_impl().get_index<subscription_index>().indices().get<by_id>();
    FC_ASSERT(idx.find(subscription_id) != idx.cend(), "Subscription \"${1}\" does not exist.", ("1", subscription_id));
}

void dbs_subscription::check_subscription_existence_by_research_group(const research_group_id_type& research_group_id) const
{
    const auto& idx = db_impl().get_index<subscription_index>().indices().get<by_research_group>();
    FC_ASSERT(idx.find(research_group_id) != idx.cend(), "Subscription with rg \"${1}\" does not exist.", ("1", research_group_id));
}

void dbs_subscription::check_subscription_existence_by_research_group_and_owner(const research_group_id_type& research_group_id, const account_name_type& owner) const
{
    const auto& idx = db_impl().get_index<subscription_index>().indices().get<by_research_group_and_owner>();

    FC_ASSERT(idx.find(boost::make_tuple(research_group_id, owner)) != idx.cend(), "\"${1}\" subscription does not exist in \"${2}\" group.",
              ("1", owner)("2", research_group_id));
}

void dbs_subscription::adjust_extra_quota_units(const subscription_object& subscription, const std::string& json_data)
{
    additional_subscription_limits_data_type data = get_data<additional_subscription_limits_data_type>(json_data);

    db_impl().modify(subscription, [&](subscription_object &s_o) {
        if (s_o.extra_file_certificate_quota_units + data.extra_file_certificate_quota_units > 0)
            s_o.extra_file_certificate_quota_units += data.extra_file_certificate_quota_units;
        else
            s_o.extra_file_certificate_quota_units = 0;

        if (s_o.extra_nda_contract_quota_units + data.extra_nda_contract_quota_units > 0)
            s_o.extra_nda_contract_quota_units += data.extra_nda_contract_quota_units;
        else
            s_o.extra_nda_contract_quota_units = 0;

        if (s_o.extra_nda_protected_file_quota_units + data.extra_nda_protected_file_quota_units > 0)
            s_o.extra_nda_protected_file_quota_units += data.extra_nda_protected_file_quota_units;
        else
            s_o.extra_nda_protected_file_quota_units = 0;
    });
}

void dbs_subscription::update(const subscription_object& subscription, const std::string& json_data)
{
    const subscription_data_type data = get_data<subscription_data_type>(json_data);
    db_impl().modify(subscription, [&](subscription_object& s_o) {
        
        if (data.external_id.valid())
            fc::from_string(s_o.external_id, *data.external_id);

        if (data.external_plan_id.valid())
            fc::from_string(s_o.external_plan_id, *data.external_plan_id);

        if (data.file_certificate_quota.valid())
            s_o.file_certificate_quota = *data.file_certificate_quota;

        if (data.nda_contract_quota.valid())
            s_o.nda_contract_quota = *data.nda_contract_quota;

        if (data.nda_protected_file_quota.valid())
            s_o.nda_protected_file_quota = *data.nda_protected_file_quota;

        if (data.current_file_certificate_quota_units.valid())
            s_o.current_file_certificate_quota_units = *data.current_file_certificate_quota_units;

        if (data.current_nda_contract_quota_units.valid())
            s_o.current_nda_contract_quota_units = *data.current_nda_contract_quota_units;

        if (data.current_nda_protected_file_quota_units.valid())
            s_o.current_nda_protected_file_quota_units = *data.current_nda_protected_file_quota_units;

        if (data.extra_file_certificate_quota_units.valid())
            s_o.extra_file_certificate_quota_units = *data.extra_file_certificate_quota_units;

        if (data.extra_nda_contract_quota_units.valid())
            s_o.extra_nda_contract_quota_units = *data.extra_nda_contract_quota_units;

        if (data.extra_nda_protected_file_quota_units.valid())
            s_o.extra_nda_protected_file_quota_units = *data.extra_nda_protected_file_quota_units;

        if (data.billing_date.valid()) {
            s_o.first_billing_date = *data.billing_date;
            s_o.billing_date = *data.billing_date;
        }

        if (data.period.valid())
            s_o.period = static_cast<billing_period>(*data.period);

    });
}

void dbs_subscription::consume_file_certificate_quota_unit(const research_group_id_type& research_group_id)
{
    check_subscription_existence_by_research_group(research_group_id);
    const subscription_object& subscription = get_by_research_group(research_group_id);

    if (subscription.current_file_certificate_quota_units > 0) 
    {
        db_impl().modify(subscription, [&](subscription_object& s_o){
            s_o.current_file_certificate_quota_units--;
        });
    }
    else
    {
        FC_ASSERT(subscription.extra_file_certificate_quota_units > 0, "You have no available certs.");
        db_impl().modify(subscription, [&](subscription_object& s_o){
            s_o.extra_file_certificate_quota_units--;
        });
    }
}

void dbs_subscription::consume_nda_contract_quota_unit(const research_group_id_type& research_group_id)
{
    check_subscription_existence_by_research_group(research_group_id);
    const subscription_object& subscription = get_by_research_group(research_group_id);

    if (subscription.current_nda_contract_quota_units > 0)
    {
        db_impl().modify(subscription, [&](subscription_object& s_o) { s_o.current_nda_contract_quota_units--; });
    }
    else
    {
        FC_ASSERT(subscription.extra_nda_contract_quota_units > 0, "You have no available contracts.");
        db_impl().modify(subscription, [&](subscription_object& s_o) { s_o.extra_nda_contract_quota_units--; });
    }
}

void dbs_subscription::consume_nda_protected_file_quota_unit(const research_group_id_type& research_group_id)
{
    check_subscription_existence_by_research_group(research_group_id);
    const subscription_object& subscription = get_by_research_group(research_group_id);

    if (subscription.current_nda_protected_file_quota_units > 0) 
    {
        db_impl().modify(subscription, [&](subscription_object& s_o){
            s_o.current_nda_protected_file_quota_units--;
        });
    }
    else
    {
        FC_ASSERT(subscription.extra_nda_protected_file_quota_units > 0, "You have no available sharings.");
        db_impl().modify(subscription, [&](subscription_object& s_o){
            s_o.extra_nda_protected_file_quota_units--;
        });
    }
}

} // namespace chain
} // namespace deip

