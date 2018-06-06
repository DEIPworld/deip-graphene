#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/witness_objects.hpp>

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
            d.id = 21;
            d.name = "test21";
            d.permlink = "test21";
            d.description = "test";
            d.quorum_percent = 40 * DEIP_1_PERCENT;
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 22;
            d.name = "test22";
            d.permlink = "test22";
            d.description = "test";
            d.quorum_percent = 60 * DEIP_1_PERCENT;
          });
    }

    void create_research_group_tokens()
    {
        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 21;
            d.research_group_id = 22;
            d.amount = 55 * DEIP_1_PERCENT;
            d.owner = "alice";
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 22;
            d.research_group_id = 22;
            d.amount = 45 * DEIP_1_PERCENT;
            d.owner = "bob";
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 23;
            d.research_group_id = 21;
            d.amount = 100 * DEIP_1_PERCENT;
            d.owner = "alice";
        });
    }

    void create_research_group_tokens_for_decrease()
    {
        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 21;
            d.research_group_id = 21;
            d.amount = 5000;
            d.owner = "alice";
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 22;
            d.research_group_id = 21;
            d.amount = 4000;
            d.owner = "bob";
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 23;
            d.research_group_id = 21;
            d.amount = 100;
            d.owner = "john";
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 24;
            d.research_group_id = 21;
            d.amount = 900;
            d.owner = "alex";
        });
    }

    void create_research_group_tokens_for_increase()
    {
        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 21;
            d.research_group_id = 21;
            d.amount = 1491;
            d.owner = "alice";
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 22;
            d.research_group_id = 21;
            d.amount = 4645;
            d.owner = "bob";
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
        auto& research_group = data_service.get_research_group(21);

        BOOST_CHECK(research_group.name == "test21");
        BOOST_CHECK(research_group.permlink == "test21");
        BOOST_CHECK(research_group.description == "test");
        BOOST_CHECK(research_group.quorum_percent == 40 * DEIP_1_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_research_group_test)
{
    try
    {
        auto& research_group = data_service.create_research_group("test", "test", "test", 34 * DEIP_1_PERCENT, false);

        BOOST_CHECK(research_group.name == "test");
        BOOST_CHECK(research_group.permlink == "test");
        BOOST_CHECK(research_group.description == "test");
        BOOST_CHECK(research_group.quorum_percent == 34 * DEIP_1_PERCENT);
        BOOST_CHECK(research_group.balance.amount == 0);
        BOOST_CHECK(research_group.is_personal == false);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_quorum_test)
{
    try
    {
        create_research_groups();
        data_service.change_quorum(24 * DEIP_1_PERCENT, 21);

        auto& research_group = data_service.get_research_group(21);

        BOOST_CHECK(research_group.quorum_percent == 24 * DEIP_1_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_research_group_existence_test)
{
    try
    {
        create_research_groups();

        BOOST_CHECK_NO_THROW(data_service.check_research_group_existence(21));
        BOOST_CHECK_THROW(data_service.check_research_group_existence(432), fc::assert_exception);
        BOOST_CHECK_THROW(data_service.check_research_group_existence(251), fc::assert_exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_token_by_id_test)
{
    try
    {
        create_research_group_tokens();

        auto& research_group_token = data_service.get_research_group_token_by_id(21);

        BOOST_CHECK(research_group_token.research_group_id == 22);
        BOOST_CHECK(research_group_token.amount == 55 * DEIP_1_PERCENT);
        BOOST_CHECK(research_group_token.owner == "alice");

        BOOST_CHECK_THROW(data_service.get_research_group_token_by_id(54), fc::exception);
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

        auto research_group_tokens = data_service.get_research_group_tokens(22);

        BOOST_CHECK(research_group_tokens.size() == 2);
        for (const research_group_token_object& token : research_group_tokens)
        {
            BOOST_CHECK(token.research_group_id == 22);
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_token_by_account_and_research_id_test)
{
    try
    {
        create_research_group_tokens();

        auto& research_group_token = data_service.get_research_group_token_by_account_and_research_group_id("alice", 21);

        BOOST_CHECK(research_group_token.id == 23);
        BOOST_CHECK(research_group_token.amount == DEIP_100_PERCENT);
        BOOST_CHECK(research_group_token.owner == "alice");
        BOOST_CHECK(research_group_token.research_group_id == 21);

        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group_id("alice", 4), fc::exception);
        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group_id("john", 1), fc::exception);
        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group_id("john", 5), fc::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_research_group_token_test)
{
    try
    {
        auto research_group_token = data_service.create_research_group_token(21, 34 * DEIP_1_PERCENT, "alice");

        BOOST_CHECK(research_group_token.research_group_id == 21);
        BOOST_CHECK(research_group_token.amount == 34 * DEIP_1_PERCENT);
        BOOST_CHECK(research_group_token.owner == "alice");

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(remove_token_test)
{
    try
    {
        create_research_group_tokens();

        BOOST_CHECK_NO_THROW(data_service.remove_token("alice", 21));
        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group_id("alice", 21), fc::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_research_group_token_existence_test)
{
    try
    {
        create_research_group_tokens();

        BOOST_CHECK_NO_THROW(data_service.check_research_group_token_existence("alice", 21));
        BOOST_CHECK_NO_THROW(data_service.check_research_group_token_existence("bob", 22));
        BOOST_CHECK_THROW(data_service.check_research_group_token_existence("john", 22), fc::assert_exception);
        BOOST_CHECK_THROW(data_service.check_research_group_token_existence("alice", 23), fc::assert_exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(decrease_research_group_tokens_amount)
{
    try
    {
        create_research_group_tokens_for_decrease();

        BOOST_CHECK(data_service.decrease_research_group_tokens_amount(21, 1000) == 990);
        auto& alice_token = db.get<research_group_token_object>(21);
        auto& bob_token = db.get<research_group_token_object>(22);
        auto& john_token = db.get<research_group_token_object>(23);
        auto& alex_token = db.get<research_group_token_object>(24);

        BOOST_CHECK(alice_token.amount == 4500);
        BOOST_CHECK(bob_token.amount == 3600);
        BOOST_CHECK(john_token.amount == 100);
        BOOST_CHECK(alex_token.amount == 810);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(increase_research_group_tokens_amount)
{
    try
    {
        create_research_group_tokens_for_increase();

        BOOST_CHECK_NO_THROW(data_service.increase_research_group_tokens_amount(21, 3864));
        auto& alice_token = db.get<research_group_token_object>(21);
        auto& bob_token = db.get<research_group_token_object>(22);

        BOOST_CHECK(alice_token.amount == 2429);
        BOOST_CHECK(bob_token.amount == 7571);
        BOOST_CHECK(alice_token.amount + bob_token.amount == DEIP_100_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(set_new_research_group_token_amount)
{
    try
    {
        create_research_group_tokens();

        auto& alice_token = data_service.set_new_research_group_token_amount(22, "alice", 4000);

        BOOST_CHECK(alice_token.amount == 40 * DEIP_1_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
