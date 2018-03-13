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
            d.permlink = "test1";
            d.description = "test";
            d.funds = 100;
            d.quorum_percent = 40;
            d.total_tokens_amount = 300;
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 2;
            d.permlink = "test2";
            d.description = "test";
            d.funds = 200;
            d.quorum_percent = 60;
            d.total_tokens_amount = 500;
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

BOOST_AUTO_TEST_CASE(create_research_group)
{
    try
    {
        auto& research_group = data_service.create_research_group("test", "test", 34, 1240);

        BOOST_CHECK(research_group.permlink == "test");
        BOOST_CHECK(research_group.description == "test");
        BOOST_CHECK(research_group.quorum_percent == 34);
        BOOST_CHECK(research_group.total_tokens_amount == 1240);
        BOOST_CHECK(research_group.funds == 0);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_by_permlink)
{
    try
    {
        create_research_groups();
        auto& research_group = data_service.get_research_group_by_permlink("test2");

        BOOST_CHECK(research_group.id == 2);
        BOOST_CHECK(research_group.permlink == "test2");
        BOOST_CHECK(research_group.description == "test");
        BOOST_CHECK(research_group.funds == 200);
        BOOST_CHECK(research_group.quorum_percent == 60);
        BOOST_CHECK(research_group.total_tokens_amount == 500);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_by_id)
{
    try
    {
        create_research_groups();
        auto& research_group = data_service.get_research_group(1);

        BOOST_CHECK(research_group.id == 1);
        BOOST_CHECK(research_group.permlink == "test1");
        BOOST_CHECK(research_group.description == "test");
        BOOST_CHECK(research_group.funds == 100);
        BOOST_CHECK(research_group.quorum_percent == 40);
        BOOST_CHECK(research_group.total_tokens_amount == 300);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_quorum)
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

BOOST_AUTO_TEST_CASE(check_research_group_existence)
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

BOOST_AUTO_TEST_CASE(check_research_group_existence_by_permlink)
{
    try
    {
        create_research_groups();

        BOOST_CHECK_NO_THROW(data_service.check_research_group_existence_by_permlink("test2"));
        BOOST_CHECK_THROW(data_service.check_research_group_existence_by_permlink("perm"), fc::assert_exception);
        BOOST_CHECK_THROW(data_service.check_research_group_existence_by_permlink("test3"), fc::assert_exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_token_by_id)
{
    try
    {
        create_research_group_tokens();

        auto& research_group_token = data_service.get_research_group_token_by_id(1);

        BOOST_CHECK(research_group_token.research_group_id == 2);
        BOOST_CHECK(research_group_token.amount == 25);
        BOOST_CHECK(research_group_token.owner == "alice");

        BOOST_CHECK_THROW(data_service.get_research_group_token_by_id(5), boost::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_tokens_by_account_name)
{
    try
    {
        create_research_group_tokens();

        auto research_group_tokens = data_service.get_research_group_tokens_by_account_name("alice");

        BOOST_CHECK(research_group_tokens.size() == 2);

        BOOST_CHECK(std::any_of(research_group_tokens.begin(), research_group_tokens.end(), [](std::reference_wrapper<const research_group_token_object> wrapper){
            const research_group_token_object &research_group_token = wrapper.get();
            return  research_group_token.id == 1 && research_group_token.research_group_id == 2 &&
                    research_group_token.owner == "alice" &&
                    research_group_token.amount == 25;
        }));

        BOOST_CHECK(std::any_of(research_group_tokens.begin(), research_group_tokens.end(), [](std::reference_wrapper<const research_group_token_object> wrapper){
            const research_group_token_object &research_group_token = wrapper.get();
            return  research_group_token.id == 3 && research_group_token.research_group_id == 1 &&
                    research_group_token.owner == "alice" &&
                    research_group_token.amount == 35;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_tokens_by_research_group_id)
{
    try
    {
        create_research_group_tokens();

        auto research_group_tokens = data_service.get_research_group_tokens_by_research_group_id(2);

        BOOST_CHECK(research_group_tokens.size() == 2);

        BOOST_CHECK(std::any_of(research_group_tokens.begin(), research_group_tokens.end(), [](std::reference_wrapper<const research_group_token_object> wrapper){
            const research_group_token_object &research_group_token = wrapper.get();
            return  research_group_token.id == 1 && research_group_token.research_group_id == 2 &&
                    research_group_token.owner == "alice" &&
                    research_group_token.amount == 25;
        }));

        BOOST_CHECK(std::any_of(research_group_tokens.begin(), research_group_tokens.end(), [](std::reference_wrapper<const research_group_token_object> wrapper){
            const research_group_token_object &research_group_token = wrapper.get();
            return  research_group_token.id == 2 && research_group_token.research_group_id == 2 &&
                    research_group_token.owner == "bob" &&
                    research_group_token.amount == 20;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_token_by_account_and_research_id)
{
    try
    {
        create_research_group_tokens();

        auto& research_group_token = data_service.get_research_group_token_by_account_and_research_group_id("alice", 1);

        BOOST_CHECK(research_group_token.id == 3);
        BOOST_CHECK(research_group_token.amount == 35);
        BOOST_CHECK(research_group_token.owner == "alice");
        BOOST_CHECK(research_group_token.research_group_id == 1);

        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group_id("alice", 4), boost::exception);
        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group_id("john", 1), boost::exception);
        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group_id("john", 5), boost::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_research_group_token)
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

BOOST_AUTO_TEST_CASE(remove_token)
{
    try
    {
        create_research_group_tokens();

        BOOST_CHECK_NO_THROW(data_service.remove_token("alice", 1 ));
        BOOST_CHECK_THROW(data_service.get_research_group_token_by_account_and_research_group_id("alice", 1), std::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_research_group_token_existence)
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

BOOST_AUTO_TEST_CASE(increase_research_group_total_tokens_amount)
{
    try
    {
        create_research_groups();

        BOOST_CHECK_NO_THROW(data_service.increase_research_group_total_tokens_amount(1, 25));

        auto research_group = db.get<research_group_object>(1);

        BOOST_CHECK(research_group.total_tokens_amount == 325);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(decrease_research_group_total_tokens_amount)
{
    try
    {
        create_research_groups();

        BOOST_CHECK_NO_THROW(data_service.decrease_research_group_total_tokens_amount(1, 25));

        auto research_group = db.get<research_group_object>(1);

        BOOST_CHECK(research_group.total_tokens_amount == 275);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(increase_research_group_funds)
{
    try
    {
        create_research_groups();

        BOOST_CHECK_NO_THROW(data_service.increase_research_group_funds(1, 25));

        auto research_group = db.get<research_group_object>(1);

        BOOST_CHECK(research_group.funds == 125);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(decrease_research_group_funds)
{
    try
    {
        create_research_groups();

        BOOST_CHECK_NO_THROW(data_service.decrease_research_group_funds(2, 25));

        auto research_group = db.get<research_group_object>(2);

        BOOST_CHECK(research_group.funds == 175);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(increase_research_group_token_amount)
{
    try
    {
        create_research_group_tokens();

        BOOST_CHECK_NO_THROW(data_service.increase_research_group_token_amount(1, "alice", 25));

        auto research_group = db.get<research_group_token_object>(3);

        BOOST_CHECK(research_group.amount == 175);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif