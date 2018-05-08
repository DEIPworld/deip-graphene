#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/database.hpp>
#include <deip/chain/research_token_object.hpp>
#include <deip/chain/util/reward.hpp>
#include <deip/chain/grant_objects.hpp>
#include <deip/chain/review_object.hpp>

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
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 2;
            d.permlink = "permlink2";
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
            d.type = milestone;
            d.authors = {"alice"};
            d.references.insert(2);
            d.activity_state = research_content_activity_state::active;
        });

        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 2;
            d.research_id = 2;
            d.type = milestone;
            d.authors = {"alex"};
            d.references.insert(1);
            d.activity_state = research_content_activity_state::active;
        });
    }

    void create_reviews()
    {
        db.create<review_object>([&](review_object& d) {
            bip::map<discipline_id_type, share_type> reward_weights_per_discipline;
            reward_weights_per_discipline[10] = 40;
            d.id = 1;
            d.research_content_id = 1;
            d.is_positive = true;
            d.author = "alice";
            d.reward_weights_per_discipline = reward_weights_per_discipline;
            d.expertise_amounts_used[10] = 50;
            d.weight_modifiers[10] = 1;
        });

        db.create<review_object>([&](review_object& d) {
            bip::map<discipline_id_type, share_type> reward_weights_per_discipline;
            reward_weights_per_discipline[10] = 60;
            d.id = 2;
            d.research_content_id = 1;
            d.is_positive = true;
            d.author = "bob";
            d.reward_weights_per_discipline = reward_weights_per_discipline;
            d.expertise_amounts_used[10] = 50;
            d.weight_modifiers[10] = 1;
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

    void create_votes_for_common_discipline()
    {
        db.create<vote_object>([&](vote_object& d) {
            d.id = 1;
            d.discipline_id = 0;
            d.voter = "bob";
            d.research_id = 1;
            d.research_content_id = 1;
            d.weight = 10;
        });

        db.create<vote_object>([&](vote_object& d) {
            d.id = 2;
            d.discipline_id = 0;
            d.voter = "john";
            d.research_id = 1;
            d.research_content_id = 1;
            d.weight = 30;
        });

        db.create<vote_object>([&](vote_object& d) {
            d.id = 3;
            d.discipline_id = 0;
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
            d.total_research_reward_weight = 60;
            d.total_active_research_reward_weight = 60;
        });
    }

    void create_total_votes_for_common_discipline()
    {
        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 1;
            d.discipline_id = 0;
            d.research_id = 1;
            d.research_content_id = 1;
            d.total_weight = 40;
            d.total_curators_reward_weight = 40;
            d.total_research_reward_weight = 40;
            d.total_active_research_reward_weight = 40;
        });

        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 2;
            d.discipline_id = 0;
            d.research_id = 2;
            d.research_content_id = 2;
            d.total_weight = 60;
            d.total_curators_reward_weight = 60;
            d.total_research_reward_weight = 60;
            d.total_active_research_reward_weight = 60;
        });
    }

    void create_review_votes()
    {
        db.create<review_vote_object>([&](review_vote_object& d) {
            d.id = 1;
            d.discipline_id = 10;
            d.voter = "alice";
            d.review_id = 1;
            d.weight = 40;
        });

        db.create<review_vote_object>([&](review_vote_object& d) {
            d.id = 2;
            d.discipline_id = 10;
            d.voter = "bob";
            d.review_id = 1;
            d.weight = 60;
        });

        db.create<review_vote_object>([&](review_vote_object& d) {
            d.id = 3;
            d.discipline_id = 10;
            d.voter = "alice";
            d.review_id = 2;
            d.weight = 40;
        });

        db.create<review_vote_object>([&](review_vote_object& d) {
            d.id = 4;
            d.discipline_id = 10;
            d.voter = "bob";
            d.review_id = 2;
            d.weight = 60;
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

        share_type reward = 40;

        create_votes();
        create_total_votes();

        BOOST_CHECK_NO_THROW(db.reward_voters(1, db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").id, reward));

        BOOST_CHECK(db.get_account("bob").balance.amount == 110);
        BOOST_CHECK(db.get_account("john").balance.amount == 130);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_review_voters)
{
    try
    {
        ACTORS((alice)(bob));
        create_discipline_with_weight();
        create_review_votes();
        create_reviews();
        fund("alice", 100);
        fund("bob", 100);

        share_type reward = 100;
        auto& review = db.get<review_object, by_id>(1);

        BOOST_CHECK_NO_THROW(db.reward_review_voters(review, 10, reward));

        BOOST_CHECK(db.get_account("alice").balance.amount == 140);
        BOOST_CHECK(db.get_account("bob").balance.amount == 160);

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
        create_reviews();

        share_type reward = 1000;
        share_type expertise_reward = 1000;

        BOOST_CHECK_NO_THROW(db.reward_reviews(1, 10, reward, expertise_reward));

        auto k = db.get<research_group_object>(2);
        auto a = db.get_account("alice");
        auto b = db.get_account("bob");

        BOOST_CHECK(db.get_account("alice").balance.amount == 380);
        BOOST_CHECK(db.get_account("bob").balance.amount == 570);

        BOOST_CHECK(db.obtain_service<dbs_expert_token>().get_expert_token_by_account_and_discipline("alice", 10).amount == 400);
        BOOST_CHECK(db.obtain_service<dbs_expert_token>().get_expert_token_by_account_and_discipline("bob", 10).amount == 600);
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

        BOOST_CHECK(db.get<research_group_object>(2).balance.amount == 500);

        auto alex_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alex", 10));

        BOOST_CHECK(alex_expert_token.amount == 1000);

        BOOST_CHECK(db.get_account("alice").balance.amount == 200);
        BOOST_CHECK(db.get_account("bob").balance.amount == 300);

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

        BOOST_CHECK(db.get<research_group_object>(2).balance.amount == 500);

        BOOST_CHECK(db.get_account("alice").balance.amount == 200);
        BOOST_CHECK(db.get_account("bob").balance.amount == 300);
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

        BOOST_CHECK(db.get<research_group_object>(1).balance.amount == 700);
        BOOST_CHECK(db.get<research_group_object>(2).balance.amount == 50);

        auto alice_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alice", 10));

        BOOST_CHECK(alice_expert_token.amount == 800);


        BOOST_CHECK(db.get_account("alice").balance.amount == 20);
        BOOST_CHECK(db.get_account("bob").balance.amount == 42);
        BOOST_CHECK(db.get_account("john").balance.amount == 37);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(fund_review_pool)
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
        create_reviews();
        create_review_votes();

        share_type reward = 1000;

        BOOST_CHECK_NO_THROW(db.fund_review_pool(10, reward));

        BOOST_CHECK(alice.balance.amount == 400);
        BOOST_CHECK(bob.balance.amount == 600);

        BOOST_CHECK(db.obtain_service<dbs_expert_token>().get_expert_token_by_account_and_discipline("alice", 10).amount == 400);
        BOOST_CHECK(db.obtain_service<dbs_expert_token>().get_expert_token_by_account_and_discipline("bob", 10).amount == 600);
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
        create_reviews();
        create_review_votes();

        share_type reward = 100000;

        auto& discipline = db.get<discipline_object, by_discipline_name>("Test Discipline With Weight");

        share_type used_reward = 0;
        BOOST_CHECK_NO_THROW(used_reward = db.reward_researches_in_discipline(discipline, reward));

        auto& group_1 = db.get<research_group_object>(1);
        auto& group_2 = db.get<research_group_object>(2);

        BOOST_CHECK(group_1.balance.amount == 52000);
        BOOST_CHECK(group_2.balance.amount == 14000);

        BOOST_CHECK(alice.balance.amount == 11300);
        BOOST_CHECK(bob.balance.amount == 15575);
        BOOST_CHECK(john.balance.amount == 2625);

        auto alice_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alice", 10));
        auto alex_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alex", 10));

        BOOST_CHECK(alice_expert_token.amount == 61700);
        BOOST_CHECK(alex_expert_token.amount == 27500);

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
        create_reviews();
        create_researches();
        create_votes();
        create_total_votes();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();

        db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& dgpo) { dgpo.total_active_disciplines_reward_weight = db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_reward_weight.value; });

        share_type reward = 10000000;

        BOOST_CHECK_NO_THROW(db.distribute_reward(reward));

        BOOST_CHECK(db.get<research_group_object>(1).balance.amount == 3952000);
        BOOST_CHECK(db.get<research_group_object>(2).balance.amount == 1064000);

        BOOST_CHECK(db.get_account("alice").balance.amount == 842840);
        BOOST_CHECK(db.get_account("bob").balance.amount == 1159760);
        BOOST_CHECK(db.get_account("john").balance.amount == 199500);

        auto john_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alice", 10));
        auto alex_expert_token = db.get<expert_token_object, by_account_and_discipline>(boost::make_tuple("alex", 10));

        BOOST_CHECK(john_expert_token.amount == 4689200);
        BOOST_CHECK(alex_expert_token.amount == 2090000);

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

        BOOST_CHECK(db.get<research_group_object>(1).balance.amount == util::calculate_share(grant, db.get<total_votes_object>(1).total_weight, db.get<discipline_object, by_discipline_name>("Test Discipline For Grant With Weight").total_active_reward_weight - db.get<total_votes_object>(3).total_weight));
        BOOST_CHECK(db.get<research_group_object>(2).balance.amount == util::calculate_share(grant, db.get<total_votes_object>(2).total_weight, db.get<discipline_object, by_discipline_name>("Test Discipline For Grant With Weight").total_active_reward_weight - db.get<total_votes_object>(3).total_weight));
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

       BOOST_CHECK(db.get<research_group_object>(1).balance.amount == util::calculate_share(100, db.get<total_votes_object>(1).total_weight, db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_reward_weight));
       BOOST_CHECK(db.get<research_group_object>(2).balance.amount == util::calculate_share(100, db.get<total_votes_object>(2).total_weight, db.get<discipline_object, by_discipline_name>("Test Discipline With Weight").total_active_reward_weight));

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_research_content_for_common_discipline)
{
    try
    {
        ACTORS((alice)(alex)(jack)(bob)(john));

        create_discipline_with_weight();
        create_research_contents();
        create_researches();
        create_votes_for_common_discipline();
        create_total_votes_for_common_discipline();
        create_research_tokens();
        create_research_groups();
        create_research_group_tokens();

        share_type reward = 1000;
        BOOST_CHECK_NO_THROW(db.reward_research_content(1, 0, reward));

        BOOST_CHECK(db.get<research_group_object>(1).balance.amount == 700);
        BOOST_CHECK(db.get<research_group_object>(2).balance.amount == 50);
        
        BOOST_CHECK_THROW((db.get<expert_token_object, by_account_and_discipline>(std::make_tuple("alice", 0))), std::out_of_range);
        BOOST_CHECK_THROW((db.get<expert_token_object, by_account_and_discipline>(std::make_tuple("bob", 0))), std::out_of_range);
        BOOST_CHECK_THROW((db.get<expert_token_object, by_account_and_discipline>(std::make_tuple("alex", 0))), std::out_of_range);
        BOOST_CHECK_THROW((db.get<expert_token_object, by_account_and_discipline>(std::make_tuple("jack", 0))), std::out_of_range);
        BOOST_CHECK_THROW((db.get<expert_token_object, by_account_and_discipline>(std::make_tuple("john", 0))), std::out_of_range);


        BOOST_CHECK(db.get_account("alice").balance.amount == 20);
        BOOST_CHECK(db.get_account("bob").balance.amount == 42);
        BOOST_CHECK(db.get_account("john").balance.amount == 37);
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
