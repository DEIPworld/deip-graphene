#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/research_token_object.hpp>
#include <deip/chain/dbs_research_token.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class research_token_service_fixture : public clean_database_fixture
{
public:
    research_token_service_fixture()
            : data_service(db.obtain_service<dbs_research_token>())
    {
    }

    void create_research_tokens()
    {
        db.create<research_token_object>([&](research_token_object& d) {
            d.id = 1;
            d.account_name = "alice";
            d.research_id = 2;
            d.amount = 200;
        });

        db.create<research_token_object>([&](research_token_object& d) {
            d.id = 2;
            d.account_name = "alice";
            d.research_id = 3;
            d.amount = 100;
        });

        db.create<research_token_object>([&](research_token_object& d) {
            d.id = 3;
            d.account_name = "alice";
            d.research_id = 4;
            d.amount = 300;
        });

        db.create<research_token_object>([&](research_token_object& d) {
            d.id = 4;
            d.account_name = "bob";
            d.research_id = 3;
            d.amount = 400;
        });

        db.create<research_token_object>([&](research_token_object& d) {
            d.id = 5;
            d.account_name = "bob";
            d.research_id = 4;
            d.amount = 200;
        });
    }


    dbs_research_token& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_token_service, research_token_service_fixture)

BOOST_AUTO_TEST_CASE(create_research_token)
{
    try
    {
        const research_token_object& research_token = data_service.create_research_token("john", 250, 34);

        BOOST_CHECK(research_token.account_name == "john");
        BOOST_CHECK(research_token.amount == 250);
        BOOST_CHECK(research_token.research_id == 34);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_token_by_id)
{
     try
     {
         create_research_tokens();
         auto research_token = data_service.get_research_token(1);

         BOOST_CHECK(research_token.account_name == "alice");
         BOOST_CHECK(research_token.research_id == 2);
         BOOST_CHECK(research_token.amount == 200);

     }
     FC_LOG_AND_RETHROW()
 }

BOOST_AUTO_TEST_CASE(get_research_tokens_by_account_name)
{
     try
     {
         create_research_tokens();
         auto research_tokens = data_service.get_research_tokens_by_account_name("alice");

         BOOST_CHECK(research_tokens.size() == 3);

     }
     FC_LOG_AND_RETHROW()
 }

BOOST_AUTO_TEST_CASE(get_research_tokens_by_research_id)
{
     try
     {
         create_research_tokens();
         auto research_tokens = data_service.get_research_tokens_by_research_id(4);

         BOOST_CHECK(research_tokens.size() == 2);

     }
     FC_LOG_AND_RETHROW()
 }

BOOST_AUTO_TEST_CASE(get_research_tokens_by_account_name_and_research_id)
{
     try
     {
         create_research_tokens();
         const research_token_object& research_token = data_service.get_research_token_by_account_name_and_research_id("alice", 4);

         BOOST_CHECK(research_token.account_name == "alice");
         BOOST_CHECK(research_token.amount == 300);
         BOOST_CHECK(research_token.research_id == 4);

     }
     FC_LOG_AND_RETHROW()
 }

BOOST_AUTO_TEST_CASE(check_research_token_existence_by_account_name_and_research_id)
{
    try
    {
        create_research_tokens();

        BOOST_CHECK_NO_THROW(data_service.check_research_token_existence_by_account_name_and_research_id("alice", 2));
        BOOST_CHECK_NO_THROW(data_service.check_research_token_existence_by_account_name_and_research_id("bob", 3));
        BOOST_CHECK(data_service.is_research_token_exists_by_account_name_and_research_id("john", 2) == false);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(increase_research_token_amount)
{
    try
    {
        create_research_tokens();

        auto& research_token = db.get<research_token_object>(1);

        BOOST_CHECK_NO_THROW(data_service.increase_research_token_amount(research_token, 20));

        BOOST_CHECK(research_token.amount == 220);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(decrease_research_token_amount)
{
    try
    {
        create_research_tokens();

        auto& research_token = db.get<research_token_object>(2);

        BOOST_CHECK_NO_THROW(data_service.decrease_research_token_amount(research_token, 20));

        BOOST_CHECK(research_token.amount == 80);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif//
// Created by dzeranov on 22.1.18.
//

