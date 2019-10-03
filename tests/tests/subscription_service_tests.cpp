#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/services/dbs_subscription.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class subscription_service_fixture : public clean_database_fixture
{
 public:
    subscription_service_fixture()
           : data_service(db.obtain_service<dbs_subscription>())
   {

   }

    void create_subscriptions()
    {
        db.create<subscription_object>([&](subscription_object& s_o) {
            s_o.id = 0;
            s_o.research_group_id = 1;
            s_o.owner = "alice";
            s_o.current_file_certificate_quota_units = 10;
            s_o.current_nda_protected_file_quota_units = 10;
            s_o.current_nda_contract_quota_units = 10;
            s_o.external_plan_id = 2;
            s_o.file_certificate_quota = 100;
            s_o.nda_protected_file_quota = 100;
            s_o.nda_contract_quota = 100;
            s_o.extra_file_certificate_quota_units = 1;
            s_o.extra_nda_protected_file_quota_units = 2;
            s_o.extra_nda_contract_quota_units = 3;
            s_o.period = billing_period::month;
            s_o.first_billing_date = fc::time_point_sec(1548864000);
            s_o.billing_date = fc::time_point_sec(1548864000);
            s_o.month_subscriptions_count = 0;
        });

        db.create<subscription_object>([&](subscription_object& s_o) {
            s_o.id = 1;
            s_o.research_group_id = 2;
            s_o.owner = "alice";
            s_o.current_file_certificate_quota_units = 10;
            s_o.current_nda_protected_file_quota_units = 10;
            s_o.current_nda_contract_quota_units = 10;
            s_o.external_plan_id = 2;
            s_o.file_certificate_quota = 100;
            s_o.nda_protected_file_quota = 100;
            s_o.nda_contract_quota = 100;
            s_o.extra_file_certificate_quota_units = 1;
            s_o.extra_nda_protected_file_quota_units = 2;
            s_o.extra_nda_contract_quota_units = 3;
            s_o.period = billing_period::month;
            s_o.first_billing_date = fc::time_point_sec(1548950400);
            s_o.billing_date = fc::time_point_sec(1561910400);
            s_o.month_subscriptions_count = 5;
        });

        db.create<subscription_object>([&](subscription_object& s_o) {
            s_o.id = 2;
            s_o.research_group_id = 3;
            s_o.owner = "bob";
            s_o.current_file_certificate_quota_units = 10;
            s_o.current_nda_protected_file_quota_units = 10;
            s_o.current_nda_contract_quota_units = 10;
            s_o.external_plan_id = 2;
            s_o.file_certificate_quota = 100;
            s_o.nda_protected_file_quota = 100;
            s_o.nda_contract_quota = 100;
            s_o.extra_file_certificate_quota_units = 1;
            s_o.extra_nda_protected_file_quota_units = 2;
            s_o.extra_nda_contract_quota_units = 3;
            s_o.period = billing_period::month;
            s_o.first_billing_date = fc::time_point_sec(1548950400);
            s_o.billing_date = fc::time_point_sec(1561910400);
            s_o.month_subscriptions_count = 5;
        });
    }

    dbs_subscription& data_service;
};

BOOST_FIXTURE_TEST_SUITE(subscription_service_tests, subscription_service_fixture)

BOOST_AUTO_TEST_CASE(create_subscription_test)
{
    try
    {
        std::string data = "{\"external_plan_id\":3,\"file_certificate_quota\":100,\"nda_protected_file_quota\":\"100\",\"nda_contract_quota\":\"100\",\"period\":\"1\",\"billing_date\":\"2019-10-18T15:02:31\"}";
        auto& subscription = data_service.create(data, 30, "alice");

        BOOST_CHECK(subscription.id == 0);
        BOOST_CHECK(subscription.research_group_id == 30);
        BOOST_CHECK(subscription.owner == "alice");
        BOOST_CHECK(subscription.external_plan_id == 3);
        BOOST_CHECK(subscription.file_certificate_quota == 100);
        BOOST_CHECK(subscription.current_file_certificate_quota_units == 100);
        BOOST_CHECK(subscription.nda_protected_file_quota == 100);
        BOOST_CHECK(subscription.current_nda_protected_file_quota_units == 100);
        BOOST_CHECK(subscription.nda_contract_quota == 100);
        BOOST_CHECK(subscription.current_nda_contract_quota_units == 100);

        BOOST_CHECK(subscription.period == billing_period::month);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get)
{
    try
    {
        create_subscriptions();
        auto& subscription = data_service.get(0);

        BOOST_CHECK(subscription.id == 0);
        BOOST_CHECK(subscription.research_group_id == 1);
        BOOST_CHECK(subscription.owner == "alice");
        BOOST_CHECK(subscription.external_plan_id == 2);
        BOOST_CHECK(subscription.file_certificate_quota == 100);
        BOOST_CHECK(subscription.current_file_certificate_quota_units == 10);
        BOOST_CHECK(subscription.nda_protected_file_quota == 100);
        BOOST_CHECK(subscription.current_nda_protected_file_quota_units == 10);
        BOOST_CHECK(subscription.nda_contract_quota == 100);
        BOOST_CHECK(subscription.current_nda_contract_quota_units == 10);

        BOOST_CHECK(subscription.period == billing_period::month);
        BOOST_CHECK(subscription.billing_date == fc::time_point_sec(1548864000));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_by_research_group)
{
    try
    {
        create_subscriptions();
        auto& subscription = data_service.get_by_research_group(1);

        BOOST_CHECK(subscription.id == 0);
        BOOST_CHECK(subscription.research_group_id == 1);
        BOOST_CHECK(subscription.owner == "alice");
        BOOST_CHECK(subscription.external_plan_id == 2);
        BOOST_CHECK(subscription.file_certificate_quota == 100);
        BOOST_CHECK(subscription.current_file_certificate_quota_units == 10);
        BOOST_CHECK(subscription.nda_protected_file_quota == 100);
        BOOST_CHECK(subscription.current_nda_protected_file_quota_units == 10);
        BOOST_CHECK(subscription.nda_contract_quota == 100);
        BOOST_CHECK(subscription.current_nda_contract_quota_units == 10);

        BOOST_CHECK(subscription.period == billing_period::month);
        BOOST_CHECK(subscription.billing_date == fc::time_point_sec(1548864000));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_by_owner)
{
    try
    {
        create_subscriptions();
        auto subscriptions = data_service.get_by_owner("alice");

        BOOST_CHECK(subscriptions.size() == 2);

        BOOST_CHECK(std::any_of(subscriptions.begin(), subscriptions.end(), [](std::reference_wrapper<const subscription_object> wrapper){
            const subscription_object &subscription = wrapper.get();
            return subscription.id == 0 &&
                    subscription.research_group_id == 1 &&
                    subscription.owner == "alice" &&
                    subscription.current_file_certificate_quota_units == 10 &&
                    subscription.current_nda_protected_file_quota_units == 10 &&
                    subscription.current_nda_contract_quota_units == 10 &&
                    subscription.external_plan_id == 2 &&
                    subscription.file_certificate_quota == 100 &&
                    subscription.nda_protected_file_quota == 100 &&
                    subscription.nda_contract_quota == 100 &&
                    subscription.extra_file_certificate_quota_units == 1 &&
                    subscription.extra_nda_contract_quota_units == 3 &&
                    subscription.extra_nda_protected_file_quota_units == 2 &&
                    subscription.period == billing_period::month &&
                    subscription.billing_date == fc::time_point_sec(1548864000) &&
                    subscription.first_billing_date == fc::time_point_sec(1548864000) &&
                    subscription.month_subscriptions_count == 0;
        }));

        BOOST_CHECK(std::any_of(subscriptions.begin(), subscriptions.end(), [](std::reference_wrapper<const subscription_object> wrapper){
            const subscription_object &subscription = wrapper.get();
            return subscription.id == 1 &&
                    subscription.research_group_id == 2 &&
                    subscription.owner == "alice" &&
                    subscription.current_file_certificate_quota_units == 10 &&
                    subscription.current_nda_protected_file_quota_units == 10 &&
                    subscription.current_nda_contract_quota_units == 10 &&
                    subscription.external_plan_id == 2 &&
                    subscription.file_certificate_quota == 100 &&
                    subscription.nda_protected_file_quota == 100 &&
                    subscription.nda_contract_quota == 100 &&
                    subscription.extra_file_certificate_quota_units == 1 &&
                    subscription.extra_nda_contract_quota_units == 3 &&
                    subscription.extra_nda_protected_file_quota_units == 2 &&
                    subscription.period == billing_period::month &&
                    subscription.billing_date == fc::time_point_sec(1561910400) &&
                    subscription.first_billing_date == fc::time_point_sec(1548950400) &&
                    subscription.month_subscriptions_count == 5;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(set_new_billing_date)
{
    try
    {
        create_subscriptions();
        auto& subscription = data_service.get(0);
        data_service.set_new_billing_date(subscription);

        BOOST_CHECK(subscription.billing_date == fc::time_point_sec(1551369600));

        auto& subscription2 = data_service.get(1);

        data_service.set_new_billing_date(subscription2);

        BOOST_CHECK(subscription2.billing_date == fc::time_point_sec(1564588800));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_subscription_existence)
{
    try
    {
        create_subscriptions();

        BOOST_CHECK_NO_THROW(data_service.check_subscription_existence(0));
        BOOST_CHECK_THROW(data_service.check_subscription_existence(4), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_subscription_existence_by_research_group)
{
    try
    {
        create_subscriptions();

        BOOST_CHECK_NO_THROW(data_service.check_subscription_existence_by_research_group(1));
        BOOST_CHECK_THROW(data_service.check_subscription_existence_by_research_group(4), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(adjust_extra_quota_units)
{
    try
    {
        create_subscriptions();
        auto& subscription = data_service.get(0);

        std::string data = "{\"extra_file_certificate_quota_units\":100,\"extra_nda_protected_file_quota_units\":105,\"extra_nda_contract_quota_units\":0}";
        data_service.adjust_extra_quota_units(subscription, data);

        BOOST_CHECK(subscription.extra_file_certificate_quota_units == 101);
        BOOST_CHECK(subscription.extra_nda_contract_quota_units == 3);
        BOOST_CHECK(subscription.extra_nda_protected_file_quota_units == 107);

        std::string data2 = "{\"extra_file_certificate_quota_units\":0,\"extra_nda_protected_file_quota_units\":2,\"extra_nda_contract_quota_units\":10}";
        data_service.adjust_extra_quota_units(subscription, data2);

        BOOST_CHECK(subscription.extra_file_certificate_quota_units == 101);
        BOOST_CHECK(subscription.extra_nda_contract_quota_units == 13);
        BOOST_CHECK(subscription.extra_nda_protected_file_quota_units == 109);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(update_subscription)
{
    try
    {
        create_subscriptions();
        auto& subscription = data_service.get(0);

        std::string data = "{\"external_plan_id\":4,\"file_certificate_quota\":1000,\"nda_protected_file_quota\":\"1000\",\"nda_contract_quota\":\"1000\",\"period\":\"1\",\"billing_date\":\"2019-10-29T15:02:31\"}";
        data_service.update(subscription, data);

        BOOST_CHECK(subscription.extra_file_certificate_quota_units == 1001);
        BOOST_CHECK(subscription.extra_nda_contract_quota_units == 1003);
        BOOST_CHECK(subscription.extra_nda_protected_file_quota_units == 1002);

        BOOST_CHECK(subscription.file_certificate_quota == 1000);
        BOOST_CHECK(subscription.nda_contract_quota == 1000);
        BOOST_CHECK(subscription.nda_protected_file_quota == 1000);

        BOOST_CHECK(subscription.external_plan_id == 4);
        BOOST_CHECK(subscription.billing_date == fc::time_point_sec(1572361351));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
