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
            v.sender = "alice";
            v.receiver = "bob";
            v.amount = 100;
            v.withdrawal_periods = 4;
            v.contract_duration = time_point_sec(DAYS_TO_SECONDS(365));
        });

        db.create<vesting_contract_object>([&](vesting_contract_object& v) {
            v.id = 2;
            v.sender = "jack";
            v.receiver = "bob";
            v.amount = 1000;
            v.withdrawal_periods = 2;
            v.contract_duration = time_point_sec(DAYS_TO_SECONDS(730));
        });

        db.create<vesting_contract_object>([&](vesting_contract_object& v) {
            v.id = 3;
            v.sender = "john";
            v.receiver = "alice";
            v.amount = 10000;
            v.withdrawal_periods = 3;
            v.contract_duration = time_point_sec(DAYS_TO_SECONDS(900));
        });
    }

    dbs_vesting_contract& data_service;
};

BOOST_FIXTURE_TEST_SUITE(vesting_contract_service_tests, vesting_contract_service_fixture)

BOOST_AUTO_TEST_CASE(create_vesting_contract)
{
    try
    {
        auto& vesting_contract = data_service.create("alice", "bob", 1000, 4, DAYS_TO_SECONDS(365));

        BOOST_CHECK(vesting_contract.sender == "alice");
        BOOST_CHECK(vesting_contract.receiver == "bob");
        BOOST_CHECK(vesting_contract.amount == 1000);
        BOOST_CHECK(vesting_contract.withdrawal_periods == 4);
        BOOST_CHECK(vesting_contract.start_date == db.head_block_time());
        BOOST_CHECK(vesting_contract.expiration_date.sec_since_epoch() == db.head_block_time().sec_since_epoch() + DAYS_TO_SECONDS(365));
        BOOST_CHECK(vesting_contract.contract_duration == fc::time_point_sec(DAYS_TO_SECONDS(365)));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_vesting_contract)
{
    try
    {
        create_vesting_contracts();
        auto& vesting_contract = data_service.get(1);

        BOOST_CHECK(vesting_contract.sender == "alice");
        BOOST_CHECK(vesting_contract.receiver == "bob");
        BOOST_CHECK(vesting_contract.amount == 100);
        BOOST_CHECK(vesting_contract.withdrawal_periods == 4);
        BOOST_CHECK(vesting_contract.contract_duration == fc::time_point_sec(DAYS_TO_SECONDS(365)));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_vesting_contract_get_by_sender_and_reviever)
{
    try
    {
        create_vesting_contracts();
        auto& vesting_contract = data_service.get_by_sender_and_reviever("jack", "bob");

        BOOST_CHECK(vesting_contract.sender == "jack");
        BOOST_CHECK(vesting_contract.receiver == "bob");
        BOOST_CHECK(vesting_contract.amount == 1000);
        BOOST_CHECK(vesting_contract.withdrawal_periods == 2);
        BOOST_CHECK(vesting_contract.contract_duration == fc::time_point_sec(DAYS_TO_SECONDS(730)));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_vesting_contract_get_by_receiver)
{
    try
    {
        create_vesting_contracts();
        auto vesting_contracts = data_service.get_by_receiver("bob");

        BOOST_CHECK(vesting_contracts.size() == 2);
        BOOST_CHECK(std::any_of(vesting_contracts.begin(), vesting_contracts.end(), [](std::reference_wrapper<const vesting_contract_object> wrapper){
            const vesting_contract_object &vesting_contract = wrapper.get();
            return  vesting_contract.id == 1 &&
                    vesting_contract.sender == "alice" &&
                    vesting_contract.receiver == "bob" &&
                    vesting_contract.amount == 100 &&
                    vesting_contract.withdrawal_periods == 4 &&
                    vesting_contract.contract_duration == time_point_sec(DAYS_TO_SECONDS(365));
        }));

        BOOST_CHECK(std::any_of(vesting_contracts.begin(), vesting_contracts.end(), [](std::reference_wrapper<const vesting_contract_object> wrapper){
            const vesting_contract_object &vesting_contract = wrapper.get();
            return  vesting_contract.id == 2 &&
                    vesting_contract.sender == "jack" &&
                    vesting_contract.receiver == "bob" &&
                    vesting_contract.amount == 1000 &&
                    vesting_contract.withdrawal_periods == 2 &&
                    vesting_contract.contract_duration == time_point_sec(DAYS_TO_SECONDS(730));
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

        BOOST_CHECK_NO_THROW(data_service.withdraw(vesting_contract, 50));

        BOOST_CHECK(vesting_contract.amount == 50);

        BOOST_CHECK_THROW(data_service.withdraw(vesting_contract, 60), fc::assert_exception);

        BOOST_CHECK_NO_THROW(data_service.withdraw(vesting_contract, 50));

        BOOST_CHECK_THROW((db.get<vesting_contract_object, by_id>(1)), std::out_of_range);

    }
    FC_LOG_AND_RETHROW()
}

//BOOST_AUTO_TEST_CASE(get_researches_by_research_group)
//{
//    try
//    {
//        create_researches();
//
//        const auto& researches = data_service.get_researches_by_research_group(2);
//
//        BOOST_CHECK(researches.size() == 2);
//
//        BOOST_CHECK(std::any_of(researches.begin(), researches.end(), [](std::reference_wrapper<const research_object> wrapper){
//            const research_object &research = wrapper.get();
//            return  research.id == 2 &&
//                    research.permlink == "Second" &&
//                    research.research_group_id == 2 &&
//                    research.review_share_in_percent == 10 &&
//                    research.dropout_compensation_in_percent == DROPOUT_COMPENSATION_IN_PERCENT &&
//                    research.is_finished == false &&
//                    research.abstract == ABSTRACT &&
//                    research.owned_tokens == DEIP_100_PERCENT;
//        }));
//
//        BOOST_CHECK(std::any_of(researches.begin(), researches.end(), [](std::reference_wrapper<const research_object> wrapper){
//            const research_object &research = wrapper.get();
//            return  research.id == 3 &&
//                    research.permlink == "Third" &&
//                    research.research_group_id == 2 &&
//                    research.review_share_in_percent == 10 &&
//                    research.dropout_compensation_in_percent == DROPOUT_COMPENSATION_IN_PERCENT &&
//                    research.is_finished == false &&
//                    research.abstract == ABSTRACT &&
//                    research.owned_tokens == DEIP_100_PERCENT;
//        }));
//    }
//    FC_LOG_AND_RETHROW()
//}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
