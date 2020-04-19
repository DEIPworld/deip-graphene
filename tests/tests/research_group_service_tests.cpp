#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/witness_objects.hpp>

#include <deip/chain/schema/research_group_object.hpp>
#include <deip/chain/services/dbs_research_group.hpp>


#include "database_fixture.hpp"

namespace deip {
namespace chain {

using deip::protocol::research_group_quorum_action;
using deip::protocol::percent_type;

class research_group_service_fixture : public clean_database_fixture
{
 public:
   research_group_service_fixture()
           : data_service(db.obtain_service<dbs_research_group>())
   {

   }

    void create_research_groups()
    {
        std::map<research_group_quorum_action, percent_type> action_quorums;
        for (int i = FIRST_ACTION_QUORUM_TYPE; i <= LAST_ACTION_QUORUM_TYPE; i++)
        {
            action_quorums.insert(std::make_pair(research_group_quorum_action(i), percent_type(1000)));
        }

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 21;
            d.name = "test21";
            d.permlink = "test21";
            d.description = "test";
            d.action_quorums.insert(action_quorums.begin(), action_quorums.end());
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 22;
            d.name = "test22";
            d.permlink = "test22";
            d.description = "test";
            d.action_quorums.insert(action_quorums.begin(), action_quorums.end());
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
        BOOST_CHECK(research_group.action_quorums.size() == 11);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_research_group_test)
{
    try
    {
        std::map<research_group_quorum_action, percent_type> action_quorums;
        for (int i = FIRST_ACTION_QUORUM_TYPE; i <= LAST_ACTION_QUORUM_TYPE; i++)
        {
            action_quorums.insert(std::make_pair(research_group_quorum_action(i), percent_type(DEIP_100_PERCENT)));
        }

        int management_model_v = 0;
        const auto& research_group = data_service.create_dao_voting_research_group(
            "alice", 
            "test", 
            "test", 
            "test",
            management_model_v,
            false,
            false,
            DEIP_100_PERCENT, 
            action_quorums);

        BOOST_CHECK(research_group.name == "test");
        BOOST_CHECK(research_group.permlink == "test");
        BOOST_CHECK(research_group.description == "test");
        BOOST_CHECK(research_group.balance.amount == 0);
        BOOST_CHECK(research_group.is_dao == true);
        BOOST_CHECK(research_group.is_personal == false);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_quorum_test)
{
    try
    {
        create_research_groups();
        data_service.change_quorum(percent_type(24 * DEIP_1_PERCENT), research_group_quorum_action(1), research_group_id_type(21));

        auto& research_group = data_service.get_research_group(21);

        BOOST_CHECK(research_group.action_quorums.at(start_research) == 24 * DEIP_1_PERCENT);
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

        auto research_group_tokens = data_service.get_tokens_by_account("alice");

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

        auto& research_group_token = data_service.get_research_group_token_by_account_and_research_group("alice", 21);

        BOOST_CHECK(research_group_token.id == 23);
        BOOST_CHECK(research_group_token.amount == DEIP_100_PERCENT);
        BOOST_CHECK(research_group_token.owner == "alice");
        BOOST_CHECK(research_group_token.research_group_id == 21);

        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group("alice", 4), fc::exception);
        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group("john", 1), fc::exception);
        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group("john", 5), fc::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_research_group_token_test)
{
    try
    {
        auto research_group_token = data_service.add_member_to_research_group("alice", 21, 34 * DEIP_1_PERCENT, account_name_type());
        BOOST_CHECK(research_group_token.research_group_id == 21);
        BOOST_CHECK(research_group_token.amount == 34 * DEIP_1_PERCENT);
        BOOST_CHECK(research_group_token.owner == "alice");

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(is_research_group_member_test)
{
    try
    {
        create_research_group_tokens();
        BOOST_CHECK(data_service.is_research_group_member("alice", 21));
        BOOST_CHECK(data_service.is_research_group_member("bob", 22));
        BOOST_CHECK(!data_service.is_research_group_member("john", 22));
        BOOST_CHECK(!data_service.is_research_group_member("alice", 23));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
