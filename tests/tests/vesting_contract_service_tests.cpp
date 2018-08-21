#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/vesting_contract_object.hpp>
#include <deip/chain/dbs_vesting_contract.hpp>

#include "database_fixture.hpp"


namespace deip {
namespace chain {

class vesting_contract_service_fixture : public clean_database_fixture
{
public:
    vesting_contract_service_fixture()
            : data_service(db.obtain_service<dbs_vesting_contract>())
    {
    }

    void create_vesting_contracts() {
        db.create<vesting_contract_object>([&](vesting_contract_object& v) {
            v.id = 1;
            v.creator = "alice";
            v.owner = "bob";
            v.balance = asset(100, DEIP_SYMBOL);
            v.vesting_duration_seconds = DAYS_TO_SECONDS(365);
            v.vesting_cliff_seconds = 0;
            v.period_duration_seconds = DAYS_TO_SECONDS(5);
        });

        db.create<vesting_contract_object>([&](vesting_contract_object& v) {
            v.id = 2;
            v.creator = "jack";
            v.owner = "bob";
            v.balance = asset(1000, DEIP_SYMBOL);
            v.vesting_duration_seconds = DAYS_TO_SECONDS(730);
            v.vesting_cliff_seconds = 0;
            v.period_duration_seconds = DAYS_TO_SECONDS(5);
        });

        db.create<vesting_contract_object>([&](vesting_contract_object& v) {
            v.id = 3;
            v.creator = "john";
            v.owner = "alice";
            v.balance = asset(10000, DEIP_SYMBOL);
            v.vesting_duration_seconds = DAYS_TO_SECONDS(900);
            v.vesting_cliff_seconds = 0;
            v.period_duration_seconds = DAYS_TO_SECONDS(5);
        });
    }

    dbs_vesting_contract& data_service;
};

BOOST_FIXTURE_TEST_SUITE(vesting_contract_service_tests, vesting_contract_service_fixture)

BOOST_AUTO_TEST_CASE(create_vesting_contract)
{
    try
    {
        auto& vesting_contract = data_service.create("alice", "bob", asset(1000, DEIP_SYMBOL), DAYS_TO_SECONDS(365), DAYS_TO_SECONDS(5), 0);

        BOOST_CHECK(vesting_contract.creator == "alice");
        BOOST_CHECK(vesting_contract.owner == "bob");
        BOOST_CHECK(vesting_contract.balance.amount == 1000);
        BOOST_CHECK(vesting_contract.start_timestamp == db.head_block_time());
        BOOST_CHECK(vesting_contract.vesting_duration_seconds == DAYS_TO_SECONDS(365));
        BOOST_CHECK(vesting_contract.period_duration_seconds == DAYS_TO_SECONDS(5));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_vesting_contract)
{
    try
    {
        create_vesting_contracts();
        auto& vesting_contract = data_service.get(1);

        BOOST_CHECK(vesting_contract.creator == "alice");
        BOOST_CHECK(vesting_contract.owner == "bob");
        BOOST_CHECK(vesting_contract.balance.amount == 100);
        BOOST_CHECK(vesting_contract.vesting_duration_seconds == DAYS_TO_SECONDS(365));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_vesting_contract_get_by_sender_and_reviever)
{
    try
    {
        create_vesting_contracts();
        auto& vesting_contract = data_service.get_by_creator_and_owner("jack", "bob");

        BOOST_CHECK(vesting_contract.creator == "jack");
        BOOST_CHECK(vesting_contract.owner == "bob");
        BOOST_CHECK(vesting_contract.balance.amount == 1000);
        BOOST_CHECK(vesting_contract.vesting_duration_seconds == DAYS_TO_SECONDS(730));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_vesting_contract_get_by_receiver)
{
    try
    {
        create_vesting_contracts();
        auto vesting_contracts = data_service.get_by_owner("bob");

        BOOST_CHECK(vesting_contracts.size() == 2);
        BOOST_CHECK(std::any_of(vesting_contracts.begin(), vesting_contracts.end(), [](std::reference_wrapper<const vesting_contract_object> wrapper){
            const vesting_contract_object &vesting_contract = wrapper.get();
            return  vesting_contract.id == 1 &&
                    vesting_contract.creator == "alice" &&
                    vesting_contract.owner == "bob" &&
                    vesting_contract.balance.amount == 100 &&
                    vesting_contract.vesting_duration_seconds == DAYS_TO_SECONDS(365);
        }));

        BOOST_CHECK(std::any_of(vesting_contracts.begin(), vesting_contracts.end(), [](std::reference_wrapper<const vesting_contract_object> wrapper){
            const vesting_contract_object &vesting_contract = wrapper.get();
            return  vesting_contract.id == 2 &&
                    vesting_contract.creator == "jack" &&
                    vesting_contract.owner == "bob" &&
                    vesting_contract.balance.amount == 1000 &&
                    vesting_contract.vesting_duration_seconds == DAYS_TO_SECONDS(730);
        }));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(withdraw_vesting_contract)
{
    try
    {
        create_vesting_contracts();
        auto& vesting_contract = db.get<vesting_contract_object, by_id>(1);

        BOOST_CHECK_NO_THROW(data_service.withdraw(1, asset(50, DEIP_SYMBOL)));

        BOOST_CHECK(vesting_contract.balance.amount == 50);

        BOOST_CHECK_THROW(data_service.withdraw(1, asset(60, DEIP_SYMBOL)), fc::assert_exception);

        BOOST_CHECK_NO_THROW(data_service.withdraw(1, asset(50, DEIP_SYMBOL)));

        BOOST_CHECK(vesting_contract.balance.amount == 0);

    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
