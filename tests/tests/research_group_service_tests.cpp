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
            d.name = "test1";
            d.permlink = "test1";
            d.description = "test";
            d.quorum_percent = 40;
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 2;
            d.name = "test2";
            d.permlink = "test2";
            d.description = "test";
            d.quorum_percent = 60;
          });
    }

    void create_research_group_tokens()
    {
        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 1;
            d.research_group_id = 2;
            d.amount = 55 * DEIP_1_PERCENT;
            d.owner = "alice";
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 2;
            d.research_group_id = 2;
            d.amount = 45 * DEIP_1_PERCENT;
            d.owner = "bob";
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 3;
            d.research_group_id = 1;
            d.amount = 100 * DEIP_1_PERCENT;
            d.owner = "alice";
        });
    }

            dbs_research_group& data_service;
        };

        BOOST_FIXTURE_TEST_SUITE(research_group_service_tests, research_group_service_fixture)

            BOOST_AUTO_TEST_CASE(get_research_group_by_id_test)
            {
                try
                {
                    create_research_groups();
                    auto& research_group = data_service.get_research_group(1);

        BOOST_CHECK(research_group.name == "test1");
        BOOST_CHECK(research_group.permlink == "test1");
        BOOST_CHECK(research_group.description == "test");
        BOOST_CHECK(research_group.quorum_percent == 40);

                }
                FC_LOG_AND_RETHROW()
            }

BOOST_AUTO_TEST_CASE(create_research_group_test)
{
    try
    {
        auto& research_group = data_service.create_research_group("test", "test", "test", 34);

        BOOST_CHECK(research_group.name == "test");
        BOOST_CHECK(research_group.permlink == "test");
        BOOST_CHECK(research_group.description == "test");
        BOOST_CHECK(research_group.quorum_percent == 34);
        BOOST_CHECK(research_group.funds == 0);

                }
                FC_LOG_AND_RETHROW()
            }

            BOOST_AUTO_TEST_CASE(change_quorum_test)
            {
                try
                {
                    create_research_groups();
                    data_service.change_quorum(24, 1);

                    auto& research_group = data_service.get_research_group(1);

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

                    auto& research_group_token = data_service.get_research_group_token_by_id(1);

        BOOST_CHECK(research_group_token.research_group_id == 2);
        BOOST_CHECK(research_group_token.amount == 55 * DEIP_1_PERCENT);
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
                    for (const research_group_token_object& token : research_group_tokens)
                    {
                        BOOST_CHECK(token.owner == "alice");
                    }
                }
                FC_LOG_AND_RETHROW()
            }

            BOOST_AUTO_TEST_CASE(get_research_group_tokens_test)
            {
                try
                {
                    create_research_group_tokens();

                    auto research_group_tokens = data_service.get_research_group_tokens(2);

                    BOOST_CHECK(research_group_tokens.size() == 2);
                    for (const research_group_token_object& token : research_group_tokens)
                    {
                        BOOST_CHECK(token.research_group_id == 2);
                    }
                }
                FC_LOG_AND_RETHROW()
            }

            BOOST_AUTO_TEST_CASE(get_research_group_token_by_account_and_research_id_test)
            {
                try
                {
                    create_research_group_tokens();

                    auto& research_group_token = data_service.get_research_group_token_by_account_and_research_group_id("alice", 1);

        BOOST_CHECK(research_group_token.id == 3);
        BOOST_CHECK(research_group_token.amount == DEIP_100_PERCENT);
        BOOST_CHECK(research_group_token.owner == "alice");
        BOOST_CHECK(research_group_token.research_group_id == 1);

                    BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group_id("alice", 4), boost::exception);
                    BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group_id("john", 1), boost::exception);
                    BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group_id("john", 5), boost::exception);
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
                    BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group_id("alice", 1), std::exception);
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

BOOST_AUTO_TEST_CASE(adjust_research_group_tokens_amount)
{
    try
    {
        create_research_group_tokens();

        BOOST_CHECK_NO_THROW(data_service.adjust_research_group_tokens_amount(2, -1000));
        auto& alice_token = db.get<research_group_token_object>(1);
        auto& bob_token = db.get<research_group_token_object>(2);

        BOOST_CHECK(alice_token.amount == 4950);
        BOOST_CHECK(bob_token.amount == 4050);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(set_new_research_group_token_amount)
{
    try
    {
        create_research_group_tokens();

        auto& alice_token = data_service.set_new_research_group_token_amount(2, "alice", 4000);

        BOOST_CHECK(alice_token.amount == 40 * DEIP_1_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

    } // namespace chain
} // namespace deip

#endif
