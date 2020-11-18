#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/schema/asset_object.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class account_balance_service_fixture : public clean_database_fixture
{
public:
    account_balance_service_fixture()
            : data_service(db.obtain_service<dbs_account_balance>())
    {
    }

    void create_account_balances()
    {
        db.create<account_balance_object>([&](account_balance_object& ab_o) {
            ab_o.id = 30;
            ab_o.owner = "alice";
            ab_o.symbol = (uint64_t(2) | (uint64_t('U') << 8) | (uint64_t('S') << 16) | (uint64_t('D') << 24));
            ab_o.asset_id = 1;
            ab_o.amount = 5000;
        });

        db.create<account_balance_object>([&](account_balance_object& ab_o) {
            ab_o.id = 31;
            ab_o.owner = "alice";
            ab_o.symbol = (uint64_t(2) | (uint64_t('E') << 8) | (uint64_t('U') << 16) | (uint64_t('R') << 24));
            ab_o.asset_id = 2;
            ab_o.amount = 500;
        });

        db.create<account_balance_object>([&](account_balance_object& ab_o) {
            ab_o.id = 32;
            ab_o.owner = "bob";
            ab_o.symbol = (uint64_t(2) | (uint64_t('U') << 8) | (uint64_t('S') << 16) | (uint64_t('D') << 24));
            ab_o.asset_id = 1;
            ab_o.amount = 1000;
        });
    }

    void create_assets()
    {
        db.create<asset_object>([&](asset_object& a_o) {
            a_o.id = 1;
            a_o.symbol = (uint64_t(2) | (uint64_t('U') << 8) | (uint64_t('S') << 16) | (uint64_t('D') << 24));
            fc::from_string(a_o.string_symbol, "USD");
            a_o.precision = 2;
        });

        db.create<asset_object>([&](asset_object& a_o) {
            a_o.id = 2;
            a_o.symbol = (uint64_t(2) | (uint64_t('E') << 8) | (uint64_t('U') << 16) | (uint64_t('R') << 24));
            fc::from_string(a_o.string_symbol, "EUR");
            a_o.precision = 2;
        });
    }

    dbs_account_balance& data_service;
};

BOOST_FIXTURE_TEST_SUITE(account_balance_service_tests, account_balance_service_fixture)

BOOST_AUTO_TEST_CASE(create)
{
    try
    {
        create_assets();
        auto& asset1 = db.get<asset_object>(1);
        auto& account_balance = data_service.create("alice", asset1.symbol, 1000);

        BOOST_CHECK(account_balance.owner == "alice");
        BOOST_CHECK(account_balance.asset_id ==  1);
        BOOST_CHECK(account_balance.amount == 1000);

        BOOST_CHECK_THROW(data_service.create("alice", asset1.symbol, 10), std::logic_error);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get)
{
    try
    {
        create_account_balances();

        auto& account_balance = data_service.get(30);

        BOOST_CHECK(account_balance.id == 30);
        BOOST_CHECK(account_balance.owner == "alice");
        BOOST_CHECK(account_balance.asset_id == 1);
        BOOST_CHECK(account_balance.amount == 5000);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_balance_exists_by_owner_and_asset)
{
    try
    {
        create_account_balances();

        BOOST_CHECK(data_service.account_balance_exists_by_owner_and_asset("alice", (uint64_t(2) | (uint64_t('U') << 8) | (uint64_t('S') << 16) | (uint64_t('D') << 24))) == true);
        BOOST_CHECK(data_service.account_balance_exists_by_owner_and_asset("bob", (uint64_t(2) | (uint64_t('E') << 8) | (uint64_t('U') << 16) | (uint64_t('R') << 24))) == false);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_account_balances_by_owner)
{
    try
    {
        create_account_balances();
        auto account_balances = data_service.get_account_balances_by_owner("alice");

        BOOST_CHECK(account_balances.size() == 2);
        BOOST_CHECK(std::any_of(account_balances.begin(), account_balances.end(), [=](std::reference_wrapper<const account_balance_object> wrapper){
            const account_balance_object& account_balance = wrapper.get();

            return  account_balance.id == 30 &&
                    account_balance.owner == "alice" &&
                    account_balance.asset_id == 1 &&
                    account_balance.amount == 5000;
        }));

        BOOST_CHECK(std::any_of(account_balances.begin(), account_balances.end(), [=](std::reference_wrapper<const account_balance_object> wrapper){
            const account_balance_object& account_balance = wrapper.get();

            return  account_balance.id == 31 &&
                    account_balance.owner == "alice" &&
                    account_balance.asset_id == 2 &&
                    account_balance.amount == 500;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_account_balance_existence)
{
    try
    {
        create_account_balances();

        BOOST_CHECK_NO_THROW(data_service.check_existence(30));
        BOOST_CHECK_THROW(data_service.check_existence(40), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(remove)
{
    try
    {
        create_account_balances();

        auto& account_balance = data_service.get(30);

        BOOST_CHECK_NO_THROW(data_service.remove(account_balance));
        BOOST_CHECK_THROW(data_service.get(30), std::out_of_range);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_account_balance_by_owner_and_asset)
{
    try
    {
        create_account_balances();

        auto& account_balance = data_service.get_account_balance_by_owner_and_asset("alice", (uint64_t(2) | (uint64_t('U') << 8) | (uint64_t('S') << 16) | (uint64_t('D') << 24)));

        BOOST_CHECK(account_balance.id == 30);
        BOOST_CHECK(account_balance.owner == "alice");
        BOOST_CHECK(account_balance.asset_id == 1);
        BOOST_CHECK(account_balance.amount == 5000);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif