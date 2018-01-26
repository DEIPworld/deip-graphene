#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/research_group_object.hpp>
#include <deip/chain/dbs_research_group.hpp>


#include "database_fixture.hpp"

namespace deip {
namespace chain {

class research_group_service_fixture : public clean_database_fixture
{
public:
    research_group_service_fixture()
            : data_service(db.obtain_service<dbs_research_group>())
    {
    }

    void create_research_groups()
    {
        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 1;
            d.permlink = "test";
            d.description = "test";
            d.quorum_percent = 40;
            d.total_tokens_amount = 300;
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 2;
            d.permlink = "test";
            d.description = "test";
            d.quorum_percent = 37;
            d.total_tokens_amount = 200;
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 3;
            d.permlink = "test";
            d.description = "test";
            d.quorum_percent = 80;
            d.total_tokens_amount = 140;
        });
    }

    void create_research_group_tokens()
    {
        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 1;
            d.research_group_id = 2;
            d.amount = 25;
            d.owner = "alice";
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 2;
            d.research_group_id = 2;
            d.amount = 20;
            d.owner = "bob";
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 3;
            d.research_group_id = 1;
            d.amount = 35;
            d.owner = "alice";
        });
    }

    dbs_research_group& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_group_service, research_group_service_fixture)

BOOST_AUTO_TEST_CASE(get_research_group_by_id_test)
{
    try
    {
        create_research_groups();
        auto research_group = data_service.get_research_group(3);

        BOOST_CHECK(research_group.permlink == "test");
        BOOST_CHECK(research_group.description == "test");
        BOOST_CHECK(research_group.quorum_percent == 80);
        BOOST_CHECK(research_group.total_tokens_amount == 140);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_research_group_test)
{
    try
    {
        auto research_group = data_service.create_research_group("test", "test", 34, 1240);

        BOOST_CHECK(research_group.permlink == "test");
        BOOST_CHECK(research_group.description == "test");
        BOOST_CHECK(research_group.quorum_percent == 34);
        BOOST_CHECK(research_group.total_tokens_amount == 1240);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_quorum_test)
{
    try
    {
        create_research_groups();
        data_service.change_quorum(24, 1);

        auto research_group = db.get<research_group_object, by_id>(1);

        BOOST_CHECK(research_group.quorum_percent == 24);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_research_group_existence_test)
{
    try
    {
        create_research_groups();

        BOOST_CHECK_NO_THROW(data_service.check_research_group_existence(1));
        BOOST_CHECK_NO_THROW(data_service.check_research_group_existence(2));
        BOOST_CHECK_THROW(data_service.check_research_group_existence(4), fc::assert_exception);
        BOOST_CHECK_THROW(data_service.check_research_group_existence(25), fc::assert_exception);


    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_token_by_id_test)
{
    try
    {
        create_research_group_tokens();

        auto research_group_token = data_service.get_research_group_token_by_id(1);

        BOOST_CHECK(research_group_token.research_group_id == 2);
        BOOST_CHECK(research_group_token.amount == 25);
        BOOST_CHECK(research_group_token.owner == "alice");

        BOOST_CHECK_THROW(data_service.get_research_group_token_by_id(5), boost::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_tokens_by_account_name_test)
{
    try
    {
        create_research_group_tokens();

        auto research_group_tokens = data_service.get_research_group_tokens_by_account_name("alice");

        BOOST_CHECK(research_group_tokens.size() == 2);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_token_by_account_and_research_id_test)
{
    try
    {
        create_research_group_tokens();

        auto research_group_token = data_service.get_research_group_token_by_account_and_research_id("alice", 1);

        BOOST_CHECK(research_group_token.id == 3);
        BOOST_CHECK(research_group_token.amount == 35);

        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_id("alice", 4), boost::exception);
        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_id("john", 1), boost::exception);
        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_id("john", 5), boost::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_research_group_token_test)
{
    try
    {
        auto research_group_token = data_service.create_research_group_token(1, 34, "alice");

        BOOST_CHECK(research_group_token.research_group_id == 1);
        BOOST_CHECK(research_group_token.amount == 34);
        BOOST_CHECK(research_group_token.owner == "alice");

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(remove_token_test)
{
    try
    {
        create_research_group_tokens();

        BOOST_CHECK_NO_THROW(data_service.remove_token("alice", 1 ));
        BOOST_CHECK_THROW((db.get<research_group_token_object, by_owner>(boost::make_tuple("alice", 1))), std::exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_research_group_token_existence_test)
{
    try
    {
      create_research_group_tokens();

      BOOST_CHECK_NO_THROW(data_service.check_research_group_token_existence("alice", 1));
      BOOST_CHECK_NO_THROW(data_service.check_research_group_token_existence("bob", 2));
      BOOST_CHECK_THROW(data_service.check_research_group_token_existence("john", 2), fc::assert_exception);
      BOOST_CHECK_THROW(data_service.check_research_group_token_existence("alice", 3), fc::assert_exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(adjust_research_group_token_amount_test)
{
    try
    {
        create_research_groups();

        BOOST_CHECK_NO_THROW(data_service.adjust_research_group_token_amount(1, 25));

        auto research_group = db.get<research_group_object, by_id>(1);

        BOOST_CHECK(research_group.total_tokens_amount == 325);

    }
    FC_LOG_AND_RETHROW()
}
BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif//

//
// Created by dzeranov on 25.1.18.
//
