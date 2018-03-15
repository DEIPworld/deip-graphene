#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/database.hpp>
#include <deip/chain/research_token_object.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class database_unit_service_fixture : public clean_database_fixture
{
public:
    database_unit_service_fixture()
            : account_service(db.obtain_service<dbs_account>()),
              vote_service(db.obtain_service<dbs_vote>()),
              research_content_service(db.obtain_service<dbs_research_content>())
    {
    }

    void create_votes()
    {
        db.create<vote_object>([&](vote_object& d) {
            d.id = 1;
            d.discipline_id = 1;
            d.voter = "alice";
            d.research_id = 1;
            d.research_content_id = 1;
            d.weight = 10;
        });

        db.create<vote_object>([&](vote_object& d) {
            d.id = 2;
            d.discipline_id = 1;
            d.voter = "bob";
            d.research_id = 1;
            d.research_content_id = 1;
            d.weight = 20;
        });

        db.create<vote_object>([&](vote_object& d) {
            d.id = 3;
            d.discipline_id = 1;
            d.voter = "john";
            d.research_id = 1;
            d.research_content_id = 1;
            d.weight = 30;
        });
    }

    void create_total_votes()
    {
        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 1;
            d.discipline_id = 1;
            d.research_id = 1;
            d.research_content_id = 1;
            d.total_curators_reward_weight = 60;
        });
    }

    void create_total_votes_for_rewards()
    {
        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 1;
            d.discipline_id = 1;
            d.research_id = 1;
            d.research_content_id = 1;
            d.total_review_reward_weight = 20;
            d.total_research_reward_weight = 20;
        });

        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 2;
            d.discipline_id = 1;
            d.research_id = 2;
            d.research_content_id = 2;
            d.total_review_reward_weight = 30;
            d.total_research_reward_weight = 30;
        });

        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 3;
            d.discipline_id = 1;
            d.research_id = 3;
            d.research_content_id = 3;
            d.total_review_reward_weight = 50;
            d.total_research_reward_weight = 50;
        });
    }

    void create_expert_token()
    {
        db.create<expert_token_object>([&](expert_token_object& d) {
            d.id = 0;
            d.account_name = "alice";
            d.discipline_id = 1;
        });
    }

    void create_research_contents()
    {
        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 1;
            d.research_id = 1;
            d.type = review;
            d.authors = {"alice"};
            d.research_references = {1, 2, 3};
        });

        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 2;
            d.research_id = 1;
            d.type = review;
            d.authors = {"bob"};
            d.research_references = {2, 3};
        });

        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 3;
            d.research_id = 1;
            d.type = review;
            d.authors = {"john"};
            d.research_references = {1, 2};
        });
    }

    void create_researches()
    {
        db.create<research_object>([&](research_object& d) {
            d.id = 1;
            d.research_group_id = 1;
            d.name = "name1";
            d.abstract = "abstract1";
            d.permlink = "permlink1";
            d.owned_tokens = 5000;
        });

        db.create<research_object>([&](research_object& d) {
            d.id = 2;
            d.research_group_id = 1;
            d.name = "name2";
            d.abstract = "abstract2";
            d.permlink = "permlink2";
            d.owned_tokens = 10000;
        });

        db.create<research_object>([&](research_object& d) {
            d.id = 3;
            d.research_group_id = 1;
            d.name = "name3";
            d.abstract = "abstract3";
            d.permlink = "permlink3";
            d.owned_tokens = 10000;
        });
    }

    void create_research_tokens()
    {
        db.create<research_token_object>([&](research_token_object& d) {
            d.id = 1;
            d.account_name = "alice";
            d.research_id = 1;
            d.amount = 20;
        });

        db.create<research_token_object>([&](research_token_object& d) {
            d.id = 2;
            d.account_name = "bob";
            d.research_id = 1;
            d.amount = 30;
        });

        db.create<research_token_object>([&](research_token_object& d) {
            d.id = 3;
            d.account_name = "john";
            d.research_id = 1;
            d.amount = 50;
        });
    }

    void create_research_group()
    {
        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 1;
            d.permlink = "permlink";
            d.total_tokens_amount = 100;
        });
    }

    void create_research_group_tokens()
    {
        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 1;
            d.research_group_id = 1;
            d.owner = "alice";
            d.amount = 20;
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 2;
            d.research_group_id = 1;
            d.owner = "bob";
            d.amount = 30;
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 3;
            d.research_group_id = 1;
            d.owner = "john";
            d.amount = 50;
        });
    }

    dbs_account& account_service;
    dbs_vote& vote_service;
    dbs_research_content& research_content_service;
};

BOOST_FIXTURE_TEST_SUITE(database_unit_service, database_unit_service_fixture)

BOOST_AUTO_TEST_CASE(reward_voters)
{
    try
    {
        ACTORS((alice)(bob)(john));
        fund("alice", 100);
        fund("bob", 100);
        fund("john", 100);

        create_votes();
        create_total_votes();

        BOOST_CHECK_NO_THROW(db.reward_voters(1, 1, 60));

        auto& alice_account = db.get_account("alice");
        auto& bob_account = db.get_account("bob");
        auto& john_account = db.get_account("john");

        BOOST_CHECK(alice_account.balance.amount == 110);
        BOOST_CHECK(bob_account.balance.amount == 120);
        BOOST_CHECK(john_account.balance.amount == 130);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_with_expertise)
{
    try
    {
        create_expert_token();

        BOOST_CHECK_NO_THROW(db.reward_with_expertise("alice", 1, 30));
        BOOST_CHECK(db.get<expert_token_object>(0).amount == 30);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_reviews)
{
    try
    {
        ACTORS((alice)(bob)(john));
        fund("alice", 100);
        fund("bob", 100);
        fund("john", 100);

        create_research_contents();
        create_researches();
        create_research_tokens();
        create_total_votes_for_rewards();
        create_research_group();
        create_research_group_tokens();

        BOOST_CHECK_NO_THROW(db.reward_reviews(1, 1, 1000000));

        BOOST_CHECK(db.get_account("alice").balance.amount == 170108);
        BOOST_CHECK(db.get_account("bob").balance.amount == 255112);
        BOOST_CHECK(db.get_account("john").balance.amount == 425120);

        BOOST_CHECK(db.get<expert_token_object>(0).amount == 170000);
        BOOST_CHECK(db.get<expert_token_object>(1).amount == 255000);
        BOOST_CHECK(db.get<expert_token_object>(2).amount == 425000);

        BOOST_CHECK(db.get<research_group_object>(1).funds == 900000);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_references)
{
    try
    {
        ACTORS((alice)(bob)(john));
        fund("alice", 100);
        fund("bob", 100);
        fund("john", 100);

        create_research_contents();
        create_researches();
        create_research_tokens();
        create_total_votes_for_rewards();
        create_research_group();
        create_research_group_tokens();

        BOOST_CHECK_NO_THROW(db.reward_references(1, 1, 10000, 10000));

        BOOST_CHECK(db.get<research_group_object>(1).funds == 9000);

        BOOST_CHECK(db.get<expert_token_object>(0).amount == 2000);
        BOOST_CHECK(db.get<expert_token_object>(1).amount == 3000);
        BOOST_CHECK(db.get<expert_token_object>(2).amount == 5000);

        BOOST_CHECK(db.get_account("alice").balance.amount == 104);
        BOOST_CHECK(db.get_account("bob").balance.amount == 106);
        BOOST_CHECK(db.get_account("john").balance.amount == 110);


    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_research_token_holders)
{
    try
    {
        ACTORS((alice)(bob)(john));
        fund("alice", 100);
        fund("bob", 100);
        fund("john", 100);

        create_researches();
        create_research_group();
        create_research_group_tokens();
        create_expert_token();
        create_research_tokens();

        BOOST_CHECK_NO_THROW(db.reward_research_token_holders(db.get<research_object>(1), 1, 1000, 1000));

        BOOST_CHECK(db.get<research_group_object>(1).funds == 500);

        BOOST_CHECK(db.get<expert_token_object>(0).amount == 200);
        BOOST_CHECK(db.get<expert_token_object>(1).amount == 300);
        BOOST_CHECK(db.get<expert_token_object>(2).amount == 500);

        BOOST_CHECK(db.get_account("alice").balance.amount == 200);
        BOOST_CHECK(db.get_account("bob").balance.amount == 250);
        BOOST_CHECK(db.get_account("john").balance.amount == 350);

    }
    FC_LOG_AND_RETHROW()
}



BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
