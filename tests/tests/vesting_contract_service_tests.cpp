#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/vesting_balance_object.hpp>
#include <deip/chain/dbs_vesting_balance.hpp>

#include "database_fixture.hpp"


namespace deip {
namespace chain {

class vesting_balance_service_fixture : public clean_database_fixture
{
public:
    vesting_balance_service_fixture()
            : data_service(db.obtain_service<dbs_vesting_balance>())
    {
    }

    void create_vesting_balances() {
        db.create<vesting_balance_object>([&](vesting_balance_object& v) {
            v.id = 1;
            v.owner = "bob";
            v.balance = asset(100, DEIP_SYMBOL);
            v.vesting_duration_seconds = DAYS_TO_SECONDS(365);
            v.vesting_cliff_seconds = 0;
            v.period_duration_seconds = DAYS_TO_SECONDS(5);
        });

        db.create<vesting_balance_object>([&](vesting_balance_object& v) {
            v.id = 2;
            v.owner = "bob";
            v.balance = asset(1000, DEIP_SYMBOL);
            v.vesting_duration_seconds = DAYS_TO_SECONDS(730);
            v.vesting_cliff_seconds = 0;
            v.period_duration_seconds = DAYS_TO_SECONDS(5);
        });

        db.create<vesting_balance_object>([&](vesting_balance_object& v) {
            v.id = 3;
            v.owner = "alice";
            v.balance = asset(10000, DEIP_SYMBOL);
            v.vesting_duration_seconds = DAYS_TO_SECONDS(900);
            v.vesting_cliff_seconds = 0;
            v.period_duration_seconds = DAYS_TO_SECONDS(5);
        });
    }

    dbs_vesting_balance& data_service;
};

BOOST_FIXTURE_TEST_SUITE(vesting_balance_service_tests, vesting_balance_service_fixture)

BOOST_AUTO_TEST_CASE(create_vesting_balance)
{
    try
    {
        auto& vesting_balance = data_service.create("bob", asset(1000, DEIP_SYMBOL), DAYS_TO_SECONDS(365), DAYS_TO_SECONDS(5), 0);

        BOOST_CHECK(vesting_balance.owner == "bob");
        BOOST_CHECK(vesting_balance.balance.amount == 1000);
        BOOST_CHECK(vesting_balance.start_timestamp == db.head_block_time());
        BOOST_CHECK(vesting_balance.vesting_duration_seconds == DAYS_TO_SECONDS(365));
        BOOST_CHECK(vesting_balance.period_duration_seconds == DAYS_TO_SECONDS(5));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_vesting_balance)
{
    try
    {
        create_vesting_balances();
        auto& vesting_balance = data_service.get(1);

        BOOST_CHECK(vesting_balance.owner == "bob");
        BOOST_CHECK(vesting_balance.balance.amount == 100);
        BOOST_CHECK(vesting_balance.vesting_duration_seconds == DAYS_TO_SECONDS(365));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_vesting_balance_get_by_receiver)
{
    try
    {
        create_vesting_balances();
        auto vesting_balances = data_service.get_by_owner("bob");

        BOOST_CHECK(vesting_balances.size() == 2);
        BOOST_CHECK(std::any_of(vesting_balances.begin(), vesting_balances.end(), [](std::reference_wrapper<const vesting_balance_object> wrapper){
            const vesting_balance_object &vesting_balance = wrapper.get();
            return  vesting_balance.id == 1 &&
                    vesting_balance.owner == "bob" &&
                    vesting_balance.balance.amount == 100 &&
                    vesting_balance.vesting_duration_seconds == DAYS_TO_SECONDS(365);
        }));

        BOOST_CHECK(std::any_of(vesting_balances.begin(), vesting_balances.end(), [](std::reference_wrapper<const vesting_balance_object> wrapper){
            const vesting_balance_object &vesting_balance = wrapper.get();
            return  vesting_balance.id == 2 &&
                    vesting_balance.owner == "bob" &&
                    vesting_balance.balance.amount == 1000 &&
                    vesting_balance.vesting_duration_seconds == DAYS_TO_SECONDS(730);
        }));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(withdraw_vesting_balance)
{
    try
    {
        create_vesting_balances();
        auto& vesting_balance = db.get<vesting_balance_object, by_id>(1);

        BOOST_CHECK_NO_THROW(data_service.withdraw(1, asset(50, DEIP_SYMBOL)));

        BOOST_CHECK(vesting_balance.balance.amount == 50);

        BOOST_CHECK_THROW(data_service.withdraw(1, asset(60, DEIP_SYMBOL)), fc::assert_exception);

        BOOST_CHECK_NO_THROW(data_service.withdraw(1, asset(50, DEIP_SYMBOL)));

        BOOST_CHECK(vesting_balance.balance.amount == 0);

    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
