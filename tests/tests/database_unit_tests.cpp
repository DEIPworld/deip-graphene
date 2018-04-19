#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/database.hpp>
#include <deip/chain/research_token_object.hpp>
#include <deip/chain/util/reward.hpp>
#include <deip/chain/grant_objects.hpp>

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

    void create_researches()
    {
        db.create<research_object>([&](research_object& d) {
            d.id = 1;
            d.research_group_id = 1;
            d.title = "name1";
            d.abstract = "abstract1";
            d.permlink = "permlink1";
            d.owned_tokens = 100 * DEIP_1_PERCENT;
            d.review_share_in_percent = 15 * DEIP_1_PERCENT;
        });

        db.create<research_object>([&](research_object& d) {
            d.id = 2;
            d.research_group_id = 2;
            d.title = "name2";
            d.abstract = "abstract2";
            d.permlink = "permlink2";
            d.owned_tokens = 50 * DEIP_1_PERCENT;
            d.review_share_in_percent = 15 * DEIP_1_PERCENT;
        });
    }

    void create_research_tokens()
    {
        db.create<research_token_object>([&](research_token_object& d) {
            d.id = 1;
            d.account_name = "alice";
            d.research_id = 2;
            d.amount = 20 * DEIP_1_PERCENT;
        });

        db.create<research_token_object>([&](research_token_object& d) {
            d.id = 2;
            d.account_name = "bob";
            d.research_id = 2;
            d.amount = 30 * DEIP_1_PERCENT;
        });
    }

    void create_research_groups()
    {
        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 1;
            d.permlink = "permlink";
            d.total_tokens_amount = 10000;
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 2;
            d.permlink = "permlink2";
            d.total_tokens_amount = 10000;
        });
    }

    void create_research_group_tokens()
    {
        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 1;
            d.research_group_id = 1;
            d.owner = "alice";
            d.amount = 2000;
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 2;
            d.research_group_id = 1;
            d.owner = "alex";
            d.amount = 8000;
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 3;
            d.research_group_id = 2;
            d.owner = "alex";
            d.amount = 2000;
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 4;
            d.research_group_id = 2;
            d.owner = "jack";
            d.amount = 8000;
        });
    }

    void create_research_contents()
    {

        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 1;
            d.research_id = 1;
            d.type = review;
            d.authors = {"alice"};
            d.references.insert(2);
        });

        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 2;
            d.research_id = 2;
            d.type = review;
            d.authors = {"alex", "jack"};
            d.references.insert(1);
        });
    }

    void create_votes()
    {
        db.create<vote_object>([&](vote_object& d) {
            d.id = 1;
            d.discipline_id = 10;
            d.voter = "bob";
            d.research_id = 1;
            d.research_content_id = 1;
            d.weight = 10;
        });

        db.create<vote_object>([&](vote_object& d) {
            d.id = 2;
            d.discipline_id = 10;
            d.voter = "john";
            d.research_id = 1;
            d.research_content_id = 1;
            d.weight = 30;
        });

        db.create<vote_object>([&](vote_object& d) {
            d.id = 3;
            d.discipline_id = 10;
            d.voter = "alice";
            d.research_id = 2;
            d.research_content_id = 2;
            d.weight = 60;
        });
    }

    void create_total_votes()
    {
        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 1;
            d.discipline_id = 10;
            d.research_id = 1;
            d.research_content_id = 1;
            d.total_weight = 40;
            d.total_curators_reward_weight = 40;
            d.total_review_reward_weight = 40;
            d.total_research_reward_weight = 40;
            d.total_active_research_reward_weight = 40;
        });

        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 2;
            d.discipline_id = 10;
            d.research_id = 2;
            d.research_content_id = 2;
            d.total_weight = 60;
            d.total_curators_reward_weight = 60;
            d.total_review_reward_weight = 60;
            d.total_research_reward_weight = 60;
            d.total_active_research_reward_weight = 60;
        });
    }

    void create_discipline_with_weight()
    {
        dbs_discipline& discipline_service = db.obtain_service<dbs_discipline>();

        db.create<discipline_object>([&](discipline_object& d) {
            d.id = discipline_service.get_disciplines().size();
            d.parent_id = 1;
            d.name = "Test Discipline With Weight";
            d.total_active_review_reward_weight = 100;
            d.total_active_research_reward_weight = 100;
            d.total_active_reward_weight = 100;
        });
    }

    void create_grant()
    {
        db.create<grant_object>([&](grant_object& d) {
            d.id = 1;
            d.owner = "bob";
            d.target_discipline = db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").id;
            d.balance = asset(100, DEIP_SYMBOL);
            d.per_block = 100;
            d.start_block = int(db.head_block_num());
            d.end_block = int(db.head_block_num());
        });
    }

    void create_grant_test_case()
    {
        dbs_discipline& discipline_service = db.obtain_service<dbs_discipline>();

        db.create<discipline_object>([&](discipline_object& d) {
            d.id = discipline_service.get_disciplines().size();
            d.parent_id = 1;
            d.name = "Test Discipline For Grant With Weight";
            d.total_active_review_reward_weight = 150;
            d.total_active_research_reward_weight = 150;
            d.total_active_reward_weight = 150;
        });

        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 3;
            d.discipline_id = db.get<discipline_object, by_discipline_name>("Test Discipline For Grant With Weight").id;
            d.research_id = 3;
            d.research_content_id = 3;
            d.total_weight = 50;
            d.total_curators_reward_weight = 50;
            d.total_review_reward_weight = 50;
            d.total_research_reward_weight = 50;
            d.total_active_research_reward_weight = 50;
        });

        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 3;
            d.research_id = 2;
            d.type = final_result;
            d.authors = {"jack"};
            d.references.insert(1);
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
        ACTORS((bob)(john));
        create_discipline_with_weight();
        fund("bob", 100);
        fund("john", 100);

        share_type reward = 30;

        create_votes();
        create_total_votes();

        BOOST_CHECK_NO_THROW(db.reward_voters(1, db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").id, reward));

        BOOST_CHECK(db.get_account("bob").balance.amount == 100 + util::calculate_share(reward, db.get<vote_object>(1).weight, db.get<total_votes_object>(1).total_weight));
        BOOST_CHECK(db.get_account("john").balance.amount == 100 + util::calculate_share(reward, db.get<vote_object>(2).weight, db.get<total_votes_object>(1).total_weight));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_with_expertise)
{
    try
    {
        ACTOR(alice);
        create_discipline_with_weight();

        share_type reward = 30;

        BOOST_CHECK_NO_THROW(db.reward_with_expertise("alice", 10, reward));
        
        auto alice_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alice", 10));        
        BOOST_CHECK(alice_expert_token.amount == reward);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_reviews)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_research_contents();
        create_researches();
        create_votes();
        create_total_votes();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();

        share_type reward = 1000;

        BOOST_CHECK_NO_THROW(db.reward_reviews(1, 10, reward));

        BOOST_CHECK(db.get_account("alice").balance.amount == util::calculate_share(reward, DEIP_100_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT - DEIP_REFERENCES_REWARD_SHARE_PERCENT) +
                                  util::calculate_share((reward * DEIP_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_object>(2).owned_tokens) / (DEIP_100_PERCENT * DEIP_100_PERCENT),
                                  db.get<research_token_object>(1).amount, DEIP_100_PERCENT - db.get<research_object>(2).owned_tokens));

        BOOST_CHECK(db.get_account("bob").balance.amount == util::calculate_share((reward * DEIP_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_object>(2).owned_tokens) / (DEIP_100_PERCENT * DEIP_100_PERCENT),
                                  db.get<research_token_object>(2).amount, DEIP_100_PERCENT - db.get<research_object>(2).owned_tokens) + util::calculate_share((reward * DEIP_CURATORS_REWARD_SHARE_PERCENT) /DEIP_100_PERCENT,
                                  db.get<vote_object>(1).weight, db.get<total_votes_object>(1).total_weight));

        BOOST_CHECK(db.get_account("john").balance.amount == util::calculate_share((reward * DEIP_CURATORS_REWARD_SHARE_PERCENT) / DEIP_100_PERCENT,
                                                                                   db.get<vote_object>(2).weight, db.get<total_votes_object>(1).total_weight));

        auto alice_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alice", 10)); 
        BOOST_CHECK(alice_expert_token.amount == util::calculate_share(reward, DEIP_100_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT - DEIP_REFERENCES_REWARD_SHARE_PERCENT));

        BOOST_CHECK(db.get<research_group_object>(2).funds == util::calculate_share(reward, (DEIP_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_object>(2).owned_tokens) / DEIP_100_PERCENT));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_references)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_research_contents();
        create_researches();
        create_total_votes();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();

        share_type reward = 1000;

        BOOST_CHECK_NO_THROW(db.reward_references(1, 10, reward, reward));

        BOOST_CHECK(db.get<research_group_object>(2).funds == (db.get<research_object>(2).owned_tokens * reward) / DEIP_100_PERCENT);

        auto alex_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alex", 10));
        auto jack_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("jack", 10));

        BOOST_CHECK(alex_expert_token.amount == (reward * db.get<research_group_token_object>(3).amount) / db.get<research_group_object>(2).total_tokens_amount);
        BOOST_CHECK(jack_expert_token.amount == (reward * db.get<research_group_token_object>(4).amount) / db.get<research_group_object>(2).total_tokens_amount);

        BOOST_CHECK(db.get_account("alice").balance.amount == (db.get<research_token_object>(1).amount * reward) / DEIP_100_PERCENT);
        BOOST_CHECK(db.get_account("bob").balance.amount == (db.get<research_token_object>(2).amount * reward) / DEIP_100_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_research_token_holders)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_researches();
        create_research_groups();
        create_research_group_tokens();
        create_research_tokens();

        share_type reward = 1000;

        BOOST_CHECK_NO_THROW(db.reward_research_token_holders(db.get<research_object>(2), 1, reward));

        BOOST_CHECK(db.get<research_group_object>(2).funds == (db.get<research_object>(2).owned_tokens * reward) / DEIP_100_PERCENT);

        BOOST_CHECK(db.get_account("alice").balance.amount == (db.get<research_token_object>(1).amount * reward) / DEIP_100_PERCENT);
        BOOST_CHECK(db.get_account("bob").balance.amount == (db.get<research_token_object>(2).amount * reward) / DEIP_100_PERCENT);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_research_content)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_research_contents();
        create_researches();
        create_votes();
        create_total_votes();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();

        share_type reward = 1000;
        BOOST_CHECK_NO_THROW(db.reward_research_content(1, 10, reward));

        BOOST_CHECK(db.get<research_group_object>(1).funds == util::calculate_share(reward, DEIP_100_PERCENT -
                DEIP_CURATORS_REWARD_SHARE_PERCENT - DEIP_REFERENCES_REWARD_SHARE_PERCENT - db.get<research_object>(1).review_share_in_percent));
        BOOST_CHECK(db.get<research_group_object>(2).funds == util::calculate_share(reward,
                    (DEIP_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_object>(2).owned_tokens) / DEIP_100_PERCENT));

        auto john_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alice", 10)); 
        auto alex_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alex", 10)); 
        auto jack_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("jack", 10));       

        BOOST_CHECK(john_expert_token.amount == util::calculate_share(reward * (DEIP_100_PERCENT -
                DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT - db.get<research_object>(1).review_share_in_percent) / DEIP_100_PERCENT, DEIP_100_PERCENT));

        BOOST_CHECK(alex_expert_token.amount == util::calculate_share((reward * DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT) / DEIP_100_PERCENT, db.get<research_group_token_object>(3).amount,
                db.get<research_group_object>(2).total_tokens_amount));

        BOOST_CHECK(jack_expert_token.amount == util::calculate_share((reward * DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT) / DEIP_100_PERCENT, db.get<research_group_token_object>(4).amount,
                db.get<research_group_object>(2).total_tokens_amount));

        BOOST_CHECK(db.get_account("alice").balance.amount == util::calculate_share((reward * DEIP_REFERENCES_REWARD_SHARE_PERCENT) / DEIP_100_PERCENT,
                    db.get<research_token_object>(1).amount, DEIP_100_PERCENT));
        BOOST_CHECK(db.get_account("bob").balance.amount == util::calculate_share((reward * DEIP_REFERENCES_REWARD_SHARE_PERCENT) / DEIP_100_PERCENT,
                    db.get<research_token_object>(2).amount, DEIP_100_PERCENT) + util::calculate_share((reward * DEIP_CURATORS_REWARD_SHARE_PERCENT) / DEIP_100_PERCENT,
                    db.get<vote_object>(1).weight, db.get<total_votes_object>(1).total_weight));
        BOOST_CHECK(db.get_account("john").balance.amount == util::calculate_share((reward * DEIP_CURATORS_REWARD_SHARE_PERCENT) / DEIP_100_PERCENT,
                    db.get<vote_object>(2).weight, db.get<total_votes_object>(1).total_weight));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_researches_in_discipline)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_research_contents();
        create_researches();
        create_votes();
        create_total_votes();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();

        share_type reward = 100000;

        BOOST_CHECK_NO_THROW(db.reward_researches_in_discipline(db.get<discipline_object, by_discipline_name>("Test Discipline With Weight"), reward));

        share_type reward_total_vote_first = (reward * db.get<total_votes_object>(1).total_active_research_reward_weight) / db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_review_reward_weight;
        share_type reward_total_vote_second = (reward * db.get<total_votes_object>(2).total_active_research_reward_weight) / db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_review_reward_weight;

        BOOST_CHECK(db.get<research_group_object>(1).funds == util::calculate_share(reward_total_vote_first, (DEIP_100_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT -
                DEIP_REFERENCES_REWARD_SHARE_PERCENT - db.get<research_object>(1).review_share_in_percent), DEIP_100_PERCENT) +
                util::calculate_share(reward_total_vote_second, DEIP_REFERENCES_REWARD_SHARE_PERCENT, DEIP_100_PERCENT));
        BOOST_CHECK(db.get<research_group_object>(2).funds == util::calculate_share(reward_total_vote_second, ((DEIP_100_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT -
                DEIP_REFERENCES_REWARD_SHARE_PERCENT - db.get<research_object>(1).review_share_in_percent) * db.get<research_object>(2).owned_tokens) / DEIP_100_PERCENT,
                DEIP_100_PERCENT) + util::calculate_share(reward_total_vote_first, (DEIP_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_object>(2).owned_tokens) / DEIP_100_PERCENT, DEIP_100_PERCENT));

        BOOST_CHECK(db.get_account("alice").balance.amount == util::calculate_share(reward_total_vote_first, (DEIP_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_token_object>(1).amount) /
                DEIP_100_PERCENT, DEIP_100_PERCENT) + util::calculate_share(reward_total_vote_second, ((DEIP_100_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT -
                DEIP_REFERENCES_REWARD_SHARE_PERCENT - db.get<research_object>(2).review_share_in_percent) * db.get<research_token_object>(1).amount) / DEIP_100_PERCENT, DEIP_100_PERCENT) +
                util::calculate_share(reward_total_vote_second, (DEIP_CURATORS_REWARD_SHARE_PERCENT * db.get<vote_object>(3).weight) / db.get<total_votes_object>(2).total_weight, DEIP_100_PERCENT));
        BOOST_CHECK(db.get_account("bob").balance.amount == util::calculate_share(reward_total_vote_first, (DEIP_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_token_object>(2).amount) /
                DEIP_100_PERCENT, DEIP_100_PERCENT) + util::calculate_share(reward_total_vote_first, (DEIP_CURATORS_REWARD_SHARE_PERCENT * db.get<vote_object>(1).weight) / db.get<total_votes_object>(1).total_weight, DEIP_100_PERCENT) +
                util::calculate_share(reward_total_vote_second, ((DEIP_100_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT - DEIP_REFERENCES_REWARD_SHARE_PERCENT - db.get<research_object>(2).review_share_in_percent) *
                db.get<research_token_object>(2).amount) / DEIP_100_PERCENT, DEIP_100_PERCENT));
        BOOST_CHECK(db.get_account("john").balance.amount == util::calculate_share(reward_total_vote_first, (DEIP_CURATORS_REWARD_SHARE_PERCENT * db.get<vote_object>(2).weight) / db.get<total_votes_object>(1).total_weight, DEIP_100_PERCENT));

        auto alice_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alice", 10));
        auto alex_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alex", 10));
        auto jack_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("jack", 10));

        BOOST_CHECK(alice_expert_token.amount == 
            util::calculate_share(reward_total_vote_first, DEIP_100_PERCENT - DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT - db.get<research_object>(1).review_share_in_percent , DEIP_100_PERCENT) + 
            util::calculate_share(reward_total_vote_second, (DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_group_token_object>(1).amount) / db.get<research_group_object>(1).total_tokens_amount, DEIP_100_PERCENT)
        );
        
        BOOST_CHECK(alex_expert_token.amount == 
            util::calculate_share(reward_total_vote_first, (DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_group_token_object>(3).amount) / db.get<research_group_object>(2).total_tokens_amount, DEIP_100_PERCENT) +
            util::calculate_share(reward_total_vote_second, DEIP_100_PERCENT - DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT - db.get<research_object>(2).review_share_in_percent, DEIP_100_PERCENT) +
            util::calculate_share(reward_total_vote_second, (DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_group_token_object>(2).amount) / db.get<research_group_object>(1).total_tokens_amount, DEIP_100_PERCENT)
        );

        BOOST_CHECK(jack_expert_token.amount == 
            util::calculate_share(reward_total_vote_first, (DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_group_token_object>(4).amount) / db.get<research_group_object>(2).total_tokens_amount, DEIP_100_PERCENT)
        );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(distribute_reward)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_research_contents();
        create_researches();
        create_votes();
        create_total_votes();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();

        db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& dgpo) { dgpo.total_active_disciplines_reward_weight = db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_reward_weight.value; });

        share_type reward = 1000000;

        BOOST_CHECK_NO_THROW(db.distribute_reward(reward));

        share_type all_disciplines_pool_share = util::calculate_share(reward, DEIP_ALL_DISCIPLINES_POOL_SHARE_PERCENT);
        auto total_discipline_reward_share = util::calculate_share(all_disciplines_pool_share,
                                                                   db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_reward_weight,
                                                                   db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_reward_weight);
        auto discipline_review_reward_pool_share = util::calculate_share(total_discipline_reward_share, DEIP_REVIEW_REWARD_POOL_SHARE_PERCENT);
        auto discipline_reward_share = total_discipline_reward_share - discipline_review_reward_pool_share;

        share_type reward_total_vote_first = (discipline_reward_share * db.get<total_votes_object>(1).total_active_research_reward_weight) / db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_review_reward_weight;
        share_type reward_total_vote_second = (discipline_reward_share * db.get<total_votes_object>(2).total_active_research_reward_weight) / db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_review_reward_weight;

        share_type reward_total_vote_pool_share_first = (discipline_review_reward_pool_share * db.get<total_votes_object>(1).total_active_research_reward_weight) / db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_review_reward_weight;
        share_type reward_total_vote_pool_share_second = (discipline_review_reward_pool_share * db.get<total_votes_object>(2).total_active_research_reward_weight) / db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_review_reward_weight;

        BOOST_CHECK(db.get<research_group_object>(1).funds == util::calculate_share(reward_total_vote_first, (DEIP_100_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT -
                DEIP_REFERENCES_REWARD_SHARE_PERCENT - db.get<research_object>(1).review_share_in_percent), DEIP_100_PERCENT) +
                util::calculate_share(reward_total_vote_second, DEIP_REFERENCES_REWARD_SHARE_PERCENT, DEIP_100_PERCENT) +
                util::calculate_share(reward_total_vote_pool_share_second, DEIP_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_object>(1).owned_tokens, DEIP_100_PERCENT * DEIP_100_PERCENT));
        BOOST_CHECK(db.get<research_group_object>(2).funds == util::calculate_share(reward_total_vote_second, ((DEIP_100_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT -
                DEIP_REFERENCES_REWARD_SHARE_PERCENT - db.get<research_object>(1).review_share_in_percent) * db.get<research_object>(2).owned_tokens) / DEIP_100_PERCENT,
                DEIP_100_PERCENT) + util::calculate_share(reward_total_vote_first, (DEIP_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_object>(2).owned_tokens) / DEIP_100_PERCENT, DEIP_100_PERCENT) +
                util::calculate_share(reward_total_vote_pool_share_first, DEIP_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_object>(2).owned_tokens, DEIP_100_PERCENT * DEIP_100_PERCENT));

        BOOST_CHECK(db.get_account("alice").balance.amount == util::calculate_share(reward_total_vote_first, (DEIP_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_token_object>(1).amount) /
                DEIP_100_PERCENT, DEIP_100_PERCENT) + util::calculate_share(reward_total_vote_second, ((DEIP_100_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT -
                DEIP_REFERENCES_REWARD_SHARE_PERCENT - db.get<research_object>(2).review_share_in_percent) * db.get<research_token_object>(1).amount) / DEIP_100_PERCENT, DEIP_100_PERCENT) +
                util::calculate_share(reward_total_vote_second, (DEIP_CURATORS_REWARD_SHARE_PERCENT * db.get<vote_object>(3).weight) / db.get<total_votes_object>(2).total_weight, DEIP_100_PERCENT) +
                util::calculate_share(reward_total_vote_pool_share_first, DEIP_100_PERCENT - DEIP_REFERENCES_REWARD_SHARE_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT, DEIP_100_PERCENT) +
                util::calculate_share(reward_total_vote_pool_share_second, DEIP_CURATORS_REWARD_SHARE_PERCENT, DEIP_100_PERCENT) +
                util::calculate_share((reward_total_vote_pool_share_first * DEIP_CURATORS_REWARD_SHARE_PERCENT) / DEIP_100_PERCENT, db.get<research_token_object>(1).amount , db.get<research_object>(2).owned_tokens));
        BOOST_CHECK(db.get_account("bob").balance.amount == util::calculate_share(reward_total_vote_first, (DEIP_REFERENCES_REWARD_SHARE_PERCENT * db.get<research_token_object>(2).amount) /
                DEIP_100_PERCENT, DEIP_100_PERCENT) + util::calculate_share(reward_total_vote_first, (DEIP_CURATORS_REWARD_SHARE_PERCENT * db.get<vote_object>(1).weight) / db.get<total_votes_object>(1).total_weight, DEIP_100_PERCENT) +
                util::calculate_share(reward_total_vote_second, ((DEIP_100_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT - DEIP_REFERENCES_REWARD_SHARE_PERCENT - db.get<research_object>(2).review_share_in_percent) *
                db.get<research_token_object>(2).amount) / DEIP_100_PERCENT, DEIP_100_PERCENT) + util::calculate_share((reward_total_vote_pool_share_first * DEIP_CURATORS_REWARD_SHARE_PERCENT) / DEIP_100_PERCENT,
                db.get<vote_object>(1).weight, db.get<total_votes_object>(1).total_weight) + util::calculate_share((reward_total_vote_pool_share_first * DEIP_CURATORS_REWARD_SHARE_PERCENT) / DEIP_100_PERCENT, db.get<research_token_object>(2).amount , db.get<research_object>(2).owned_tokens));
        BOOST_CHECK(db.get_account("john").balance.amount == util::calculate_share(reward_total_vote_first, (DEIP_CURATORS_REWARD_SHARE_PERCENT * db.get<vote_object>(2).weight) / db.get<total_votes_object>(1).total_weight, DEIP_100_PERCENT) +
                    util::calculate_share((reward_total_vote_pool_share_first * DEIP_CURATORS_REWARD_SHARE_PERCENT) / DEIP_100_PERCENT, db.get<vote_object>(2).weight, db.get<total_votes_object>(1).total_weight));

        auto john_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alice", 10));
        auto alex_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alex", 10));
        auto jack_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("jack", 10));

        auto test_value1 = db.get<research_group_object>(1).total_tokens_amount;
    
        BOOST_CHECK(john_expert_token.amount
                    == util::calculate_share(reward_total_vote_first,
                                             DEIP_100_PERCENT - DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT
                                                 - db.get<research_object>(1).review_share_in_percent,
                                             DEIP_100_PERCENT)
                        + util::calculate_share(reward_total_vote_second,
                                                (DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT
                                                 * db.get<research_group_token_object>(1).amount)
                                                    / db.get<research_group_object>(1).total_tokens_amount,
                                                DEIP_100_PERCENT)
                        + util::calculate_share((reward * DEIP_ALL_DISCIPLINES_POOL_SHARE_PERCENT
                                                 * DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT)
                                                    / (DEIP_100_PERCENT * DEIP_100_PERCENT),
                                                db.get<total_votes_object>(1).total_active_research_reward_weight
                                                    * (DEIP_100_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT
                                                       - DEIP_REFERENCES_REWARD_SHARE_PERCENT),
                                                db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_review_reward_weight
                                                    * DEIP_100_PERCENT));
        BOOST_CHECK(alex_expert_token.amount
                    == util::calculate_share(reward_total_vote_first,
                                             (DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT
                                              * db.get<research_group_token_object>(3).amount)
                                                 / db.get<research_group_object>(2).total_tokens_amount,
                                             DEIP_100_PERCENT)
                        + util::calculate_share(reward_total_vote_second,
                                                DEIP_100_PERCENT - DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT
                                                    - db.get<research_object>(2).review_share_in_percent,
                                                DEIP_100_PERCENT)
                        + util::calculate_share(reward_total_vote_second,
                                                (DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT
                                                 * db.get<research_group_token_object>(2).amount)
                                                    / db.get<research_group_object>(1).total_tokens_amount,
                                                DEIP_100_PERCENT)
                        + util::calculate_share((reward * DEIP_ALL_DISCIPLINES_POOL_SHARE_PERCENT
                                                 * DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT)
                                                    / (DEIP_100_PERCENT * DEIP_100_PERCENT),
                                                db.get<total_votes_object>(2).total_active_research_reward_weight
                                                    * (DEIP_100_PERCENT - DEIP_CURATORS_REWARD_SHARE_PERCENT
                                                       - DEIP_REFERENCES_REWARD_SHARE_PERCENT),
                                                db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_review_reward_weight
                                                    * DEIP_100_PERCENT));
        BOOST_CHECK(jack_expert_token.amount
                    == util::calculate_share(reward_total_vote_first,
                                             (DEIP_EXPERTISE_REFERENCES_REWARD_SHARE_PERCENT
                                              * db.get<research_group_token_object>(4).amount)
                                                 / db.get<research_group_object>(2).total_tokens_amount,
                                             DEIP_100_PERCENT));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(grant_researches_in_discipline)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_research_contents();
        create_researches();
        create_votes();
        create_total_votes();
        create_research_groups();
        create_research_group_tokens();
        create_grant_test_case();

        share_type grant = 100000;

        BOOST_CHECK_NO_THROW(db.grant_researches_in_discipline(db.get<discipline_object, by_discipline_name>("Test Discipline For Grant With Weight").id, grant));

        BOOST_CHECK(db.get<research_group_object>(1).funds == util::calculate_share(grant, db.get<total_votes_object>(1).total_weight, db.get<discipline_object, by_discipline_name>("Test Discipline For Grant With Weight").total_active_reward_weight - db.get<total_votes_object>(3).total_weight));
        BOOST_CHECK(db.get<research_group_object>(2).funds == util::calculate_share(grant, db.get<total_votes_object>(2).total_weight, db.get<discipline_object, by_discipline_name>("Test Discipline For Grant With Weight").total_active_reward_weight - db.get<total_votes_object>(3).total_weight));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(process_grants)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: process_grants");

       ACTORS((alice)(alex)(jack)(bob)(john));

       generate_block();

       create_discipline_with_weight();
       create_research_contents();
       create_researches();
       create_votes();
       create_total_votes();
       create_research_groups();
       create_research_group_tokens();
       create_grant();

       BOOST_CHECK_NO_THROW(db.process_grants());

       BOOST_CHECK(db.get<research_group_object>(1).funds == util::calculate_share(100, db.get<total_votes_object>(1).total_weight, db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_reward_weight));
       BOOST_CHECK(db.get<research_group_object>(2).funds == util::calculate_share(100, db.get<total_votes_object>(2).total_weight, db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_reward_weight));

   }
   FC_LOG_AND_RETHROW()
}

    
BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
