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
            s_o.remained_certs = 10;
            s_o.remained_sharings = 10;
            s_o.remained_contracts = 10;
            s_o.external_plan_id = 2;
            s_o.plan_certs = 100;
            s_o.plan_sharings = 100;
            s_o.plan_contracts = 100;
            s_o.additional_certs = 1;
            s_o.additional_sharings = 2;
            s_o.additional_contracts = 3;
            s_o.period = billing_period::month;
            s_o.first_billing_date = fc::time_point_sec(1548864000);
            s_o.billing_date = fc::time_point_sec(1548864000);
            s_o.month_subscriptions_count = 0;
        });

        db.create<subscription_object>([&](subscription_object& s_o) {
            s_o.id = 1;
            s_o.research_group_id = 2;
            s_o.remained_certs = 10;
            s_o.remained_sharings = 10;
            s_o.remained_contracts = 10;
            s_o.external_plan_id = 2;
            s_o.plan_certs = 100;
            s_o.plan_sharings = 100;
            s_o.plan_contracts = 100;
            s_o.additional_certs = 1;
            s_o.additional_sharings = 2;
            s_o.additional_contracts = 3;
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
        std::string data = "{\"external_plan_id\":3,\"plan_certs\":100,\"plan_sharings\":\"100\",\"plan_contracts\":\"100\",\"period\":\"1\",\"billing_date\":\"2019-10-18T15:02:31\"}";
        auto& subscription = data_service.create(data, 30, "alice");

        BOOST_CHECK(subscription.id == 0);
        BOOST_CHECK(subscription.research_group_id == 30);
        BOOST_CHECK(subscription.owner == "alice");
        BOOST_CHECK(subscription.external_plan_id == 3);
        BOOST_CHECK(subscription.plan_certs == 100);
        BOOST_CHECK(subscription.remained_certs == 100);
        BOOST_CHECK(subscription.plan_sharings == 100);
        BOOST_CHECK(subscription.remained_sharings == 100);
        BOOST_CHECK(subscription.plan_contracts == 100);
        BOOST_CHECK(subscription.remained_contracts == 100);

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
        BOOST_CHECK(subscription.external_plan_id == 2);
        BOOST_CHECK(subscription.plan_certs == 100);
        BOOST_CHECK(subscription.remained_certs == 10);
        BOOST_CHECK(subscription.plan_sharings == 100);
        BOOST_CHECK(subscription.remained_sharings == 10);
        BOOST_CHECK(subscription.plan_contracts == 100);
        BOOST_CHECK(subscription.remained_contracts == 10);

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
        BOOST_CHECK(subscription.external_plan_id == 2);
        BOOST_CHECK(subscription.plan_certs == 100);
        BOOST_CHECK(subscription.remained_certs == 10);
        BOOST_CHECK(subscription.plan_sharings == 100);
        BOOST_CHECK(subscription.remained_sharings == 10);
        BOOST_CHECK(subscription.plan_contracts == 100);
        BOOST_CHECK(subscription.remained_contracts == 10);

        BOOST_CHECK(subscription.period == billing_period::month);
        BOOST_CHECK(subscription.billing_date == fc::time_point_sec(1548864000));
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
        BOOST_CHECK_THROW(data_service.check_subscription_existence(2), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_subscription_existence_by_research_group)
{
    try
    {
        create_subscriptions();

        BOOST_CHECK_NO_THROW(data_service.check_subscription_existence_by_research_group(1));
        BOOST_CHECK_THROW(data_service.check_subscription_existence_by_research_group(3), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(adjust_additional_limits)
{
    try
    {
        create_subscriptions();
        auto& subscription = data_service.get(0);

        std::string data = "{\"additional_certs\":100,\"additional_sharings\":105,\"additional_contracts\":0}";
        data_service.adjust_additional_limits(subscription, data);

        BOOST_CHECK(subscription.additional_certs == 101);
        BOOST_CHECK(subscription.additional_sharings == 107);
        BOOST_CHECK(subscription.additional_contracts == 3);

        std::string data2 = "{\"additional_certs\":0,\"additional_sharings\":2,\"additional_contracts\":10}";
        data_service.adjust_additional_limits(subscription, data2);

        BOOST_CHECK(subscription.additional_certs == 101);
        BOOST_CHECK(subscription.additional_sharings == 109);
        BOOST_CHECK(subscription.additional_contracts == 13);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(update_subscription)
{
    try
    {
        create_subscriptions();
        auto& subscription = data_service.get(0);

        std::string data = "{\"external_plan_id\":4,\"plan_certs\":1000,\"plan_sharings\":\"1000\",\"plan_contracts\":\"1000\",\"period\":\"1\",\"billing_date\":\"2019-10-29T15:02:31\"}";
        data_service.update(subscription, data);

        BOOST_CHECK(subscription.additional_certs == 11);
        BOOST_CHECK(subscription.additional_sharings == 12);
        BOOST_CHECK(subscription.additional_contracts == 13);

        BOOST_CHECK(subscription.plan_certs == 1000);
        BOOST_CHECK(subscription.plan_sharings == 1000);
        BOOST_CHECK(subscription.plan_contracts == 1000);

        BOOST_CHECK(subscription.external_plan_id == 4);
        BOOST_CHECK(subscription.billing_date == fc::time_point_sec(1572361351));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
