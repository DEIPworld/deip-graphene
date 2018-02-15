#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/expert_token_object.hpp>
#include <deip/chain/dbs_expert_token.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class expert_token_service_fixture : public clean_database_fixture
{
public:
    expert_token_service_fixture()
            : data_service(db.obtain_service<dbs_expert_token>())
    {
    }

    void create_expert_tokens()
    {
        db.create<expert_token_object>([&](expert_token_object& d) {
            d.id = 1;
            d.account_name = "alice";
            d.discipline_id = 1111;
            d.amount = 100;
        });

        db.create<expert_token_object>([&](expert_token_object& d) {
            d.id = 2;
            d.account_name = "bob";
            d.discipline_id = 2222;
            d.amount = 200;
        });

        db.create<expert_token_object>([&](expert_token_object& d) {
            d.id = 3;
            d.account_name = "alice";
            d.discipline_id = 3333;
            d.amount = 300;
        });

        db.create<expert_token_object>([&](expert_token_object& d) {
            d.id = 4;
            d.account_name = "john";
            d.discipline_id = 1111;
            d.amount = 150;
        });
        
    }

    dbs_expert_token& data_service;
};

BOOST_FIXTURE_TEST_SUITE(expert_token_service, expert_token_service_fixture)

BOOST_AUTO_TEST_CASE(create)
{
    try
    {
        auto token = data_service.create("alice", 1000, 1000);

        BOOST_CHECK(token.id == 0);
        BOOST_CHECK(token.account_name == "alice");
        BOOST_CHECK(token.discipline_id == 1000);
        BOOST_CHECK(token.amount == 1000);
        BOOST_CHECK(token.voting_power == DEIP_100_PERCENT);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_expert_token_by_id)
{
    try
    {
        create_expert_tokens();
        auto token = data_service.get_expert_token(2);

        BOOST_CHECK(token.id == 2);
        BOOST_CHECK(token.account_name == "bob");
        BOOST_CHECK(token.discipline_id == 2222);
        BOOST_CHECK(token.amount == 200);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(throw_on_get_expert_token_by_non_existing_id)
{
    try
    {
        create_expert_tokens();
        BOOST_CHECK_THROW(data_service.get_expert_token(9), boost::exception);
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(get_expert_tokens_vector_by_account_name)
{
    try
    {
        create_expert_tokens();
        auto expert_tokens = data_service.get_expert_tokens_by_account_name("alice");

        BOOST_CHECK(expert_tokens.size() == 2);
        BOOST_CHECK(std::any_of(expert_tokens.begin(), expert_tokens.end(), [](std::reference_wrapper<const expert_token_object> wrapper){
            const expert_token_object &token = wrapper.get();
            return token.id == 1 && token.account_name == "alice" && token.discipline_id == 1111 && token.amount == 100;
        }));
        BOOST_CHECK(std::any_of(expert_tokens.begin(), expert_tokens.end(), [](std::reference_wrapper<const expert_token_object> wrapper){
            const expert_token_object &token = wrapper.get();
            return token.id == 3 && token.account_name == "alice" && token.discipline_id == 3333 && token.amount == 300;
        }));
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(get_empty_expert_tokens_vector_by_account_name)
{
    try
    {
        create_expert_tokens();
        auto expert_tokens = data_service.get_expert_tokens_by_account_name("somebody");

        BOOST_CHECK(expert_tokens.size() == 0);
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(get_expert_tokens_vector_by_discipline_id)
{
    try
    {
        create_expert_tokens();
        auto expert_tokens = data_service.get_expert_tokens_by_discipline_id(1111);

        BOOST_CHECK(expert_tokens.size() == 2);
        BOOST_CHECK(std::any_of(expert_tokens.begin(), expert_tokens.end(), [](std::reference_wrapper<const expert_token_object> wrapper){
            const expert_token_object &token = wrapper.get();
            return token.id == 1 && token.account_name == "alice" && token.discipline_id == 1111 && token.amount == 100;
        }));
        BOOST_CHECK(std::any_of(expert_tokens.begin(), expert_tokens.end(), [](std::reference_wrapper<const expert_token_object> wrapper){
            const expert_token_object &token = wrapper.get();
            return token.id == 4 && token.account_name == "john" && token.discipline_id == 1111 && token.amount == 150;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_empty_expert_tokens_vector_by_discipline_id)
{
    try
    {
        create_expert_tokens();
        auto expert_tokens = data_service.get_expert_tokens_by_discipline_id(1212);

        BOOST_CHECK(expert_tokens.size() == 0);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_expert_token_by_account_and_discipline_id)
{
    try
    {
        create_expert_tokens();
        auto expert_token = data_service.get_expert_token_by_account_and_discipline("alice", 1111);

        BOOST_CHECK(expert_token.id == 1);
        BOOST_CHECK(expert_token.amount == 100);
        BOOST_CHECK(expert_token.discipline_id == 1111);
        BOOST_CHECK(expert_token.account_name == "alice");

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_expert_token_existence_by_account_and_discipline_id)
{
    BOOST_CHECK_THROW(data_service.check_expert_token_existence_by_account_and_discipline("alice", 1111), fc::assert_exception);
}




BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
