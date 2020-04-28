#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/database/database.hpp>
#include <deip/chain/util/reward.hpp>

#include <deip/chain/schema/expertise_allocation_proposal_object.hpp>
#include <deip/chain/schema/grant_object.hpp>
#include <deip/chain/schema/grant_application_object.hpp>
#include <deip/chain/schema/grant_application_review_object.hpp>
#include <deip/chain/schema/research_discipline_relation_object.hpp>
#include <deip/chain/schema/research_token_object.hpp>
#include <deip/chain/schema/review_object.hpp>
#include <deip/chain/schema/reward_pool_object.hpp>

#include <deip/chain/services/dbs_account_balance.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class database_unit_service_fixture : public clean_database_fixture
{
public:
    database_unit_service_fixture()
            : account_service(db.obtain_service<dbs_account>()),
              review_votes_service(db.obtain_service<dbs_review_vote>()),
              research_content_service(db.obtain_service<dbs_research_content>()),
              grant_service(db.obtain_service<dbs_grant>()),
              account_balance_service(db.obtain_service<dbs_account_balance>())
    {
    }

    void create_researches()
    {
        db.create<research_object>([&](research_object& d) {
            d.id = 1;
            d.research_group_id = 31;
            d.title = "name1";
            d.abstract = "abstract1";
            d.permlink = "permlink1";
            d.owned_tokens = 100 * DEIP_1_PERCENT;
            d.review_share = 10 * DEIP_1_PERCENT;
        });

        db.create<research_object>([&](research_object& d) {
            d.id = 2;
            d.research_group_id = 32;
            d.title = "name2";
            d.abstract = "abstract2";
            d.permlink = "permlink2";
            d.owned_tokens = 50 * DEIP_1_PERCENT;
            d.review_share = 10 * DEIP_1_PERCENT;
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
            d.id = 31;
            d.permlink = "permlink31";
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 32;
            d.permlink = "permlink32";
        });
    }

    void create_research_group_tokens()
    {
        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 31;
            d.research_group_id = 31;
            d.owner = "alice";
            d.amount = 2000;
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 32;
            d.research_group_id = 31;
            d.owner = "alex";
            d.amount = 8000;
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 33;
            d.research_group_id = 32;
            d.owner = "alex";
            d.amount = 2000;
        });

        db.create<research_group_token_object>([&](research_group_token_object& d) {
            d.id = 34;
            d.research_group_id = 32;
            d.owner = "jack";
            d.amount = 8000;
        });
    }

    void create_research_contents()
    {
        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 1;
            d.research_id = 1;
            d.permlink = "milestone_research_1";
            d.type = milestone_article;
            d.authors = {"alice"};
            d.references.insert(2);
            d.activity_state = research_content_activity_state::active;
        });

        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 2;
            d.research_id = 2;
            d.permlink = "milestone_research_2";
            d.type = milestone_data;
            d.authors = {"alex"};
            d.references.insert(1);
            d.activity_state = research_content_activity_state::active;
        });
    }

    void create_research_contents_for_activity_windows()
    {
        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 1;
            d.research_id = 1;
            d.permlink = "milestone_research_1";
            d.type = milestone_data;
            d.authors = {"alice"};
            d.references.insert(2);
            d.activity_state = research_content_activity_state::active;
            d.activity_round = 2;
            d.activity_window_end = db.head_block_time() + DAYS_TO_SECONDS(7);
        });

        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 2;
            d.research_id = 2;
            d.permlink = "final_result_research_1";
            d.type = final_result;
            d.authors = {"alex"};
            d.references.insert(1);
            d.activity_state = research_content_activity_state::active;
            d.activity_round = 3;
            d.activity_window_end = db.head_block_time() + DAYS_TO_SECONDS(14);
        });
    }

    void create_reward_pools()
    {
        db.create<reward_pool_object>([&](reward_pool_object& d) {
            d.id = 1;
            d.discipline_id = 10;
            d.balance = asset(500, DEIP_SYMBOL);
            d.expertise = 500;
            d.research_content_id = 1;
        });

        db.create<reward_pool_object>([&](reward_pool_object& d) {
            d.id = 2;
            d.discipline_id = 10;
            d.balance = asset(500, DEIP_SYMBOL);
            d.expertise = 500;
            d.research_content_id = 2;
        });
    }

    void create_reviews()
    {
        db.create<review_object>([&](review_object& d) {
            bip::map<discipline_id_type, share_type> reward_weights_per_discipline;
            reward_weights_per_discipline[10] = 100;
            d.id = 1;
            d.research_content_id = 1;
            d.is_positive = true;
            d.author = "alice";
            d.expertise_tokens_amount_by_discipline.insert(reward_weights_per_discipline.begin(), reward_weights_per_discipline.end());
            d.expertise_tokens_amount_by_discipline[10] = 50;
        });

        db.create<review_object>([&](review_object& d) {
            bip::map<discipline_id_type, share_type> reward_weights_per_discipline;
            reward_weights_per_discipline[10] = 100;
            d.id = 2;
            d.research_content_id = 1;
            d.is_positive = true;
            d.author = "bob";
            d.expertise_tokens_amount_by_discipline.insert(reward_weights_per_discipline.begin(), reward_weights_per_discipline.end());
            d.expertise_tokens_amount_by_discipline[10] = 50;
        });
    }


    void create_review_votes()
    {
        db.create<review_vote_object>([&](review_vote_object& d) {
            d.id = 1;
            d.discipline_id = 10;
            d.voter = "jack";
            d.review_id = 1;
            d.weight = 40;
        });

        db.create<review_vote_object>([&](review_vote_object& d) {
            d.id = 2;
            d.discipline_id = 10;
            d.voter = "john";
            d.review_id = 1;
            d.weight = 60;
        });

        db.create<review_vote_object>([&](review_vote_object& d) {
            d.id = 3;
            d.discipline_id = 10;
            d.voter = "jack";
            d.review_id = 2;
            d.weight = 40;
        });

        db.create<review_vote_object>([&](review_vote_object& d) {
            d.id = 4;
            d.discipline_id = 10;
            d.voter = "john";
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
        });
    }



    void create_grant_test_case()
    {
        dbs_discipline& discipline_service = db.obtain_service<dbs_discipline>();

        db.create<discipline_object>([&](discipline_object& d) {
            d.id = discipline_service.get_disciplines().size();
            d.parent_id = 1;
            d.name = "Test Discipline For Grant With Weight";
        });

//        db.create<expertise_contribution_object>([&](expertise_contribution_object& d) {
//            d.id = 3;
//            d.discipline_id = db.get<discipline_object, by_discipline_name>("Test Discipline For Grant With Weight").id;
//            d.research_id = 3;
//            d.research_content_id = 3;
//            d.total_weight = 50;
//        });

        db.create<research_content_object>([&](research_content_object& d) {
            d.id = 3;
            d.research_id = 2;
            d.type = final_result;
            d.authors = {"jack"};
            d.references.insert(1);
        });
    }

    void create_expertise_allocation_proposals()
    {
        db.create<expertise_allocation_proposal_object>([&](expertise_allocation_proposal_object& eap_o) {
            eap_o.id = 0;
            eap_o.claimer = "bob";
            eap_o.discipline_id = 2;
            eap_o.total_voted_expertise = 0;
            eap_o.description = "test1";
            eap_o.expiration_time = time_point_sec(132);
            eap_o.quorum = 15 * DEIP_1_PERCENT;
        });
        db.create<expertise_allocation_proposal_object>([&](expertise_allocation_proposal_object& eap_o) {
            eap_o.id = 1;
            eap_o.claimer = "mike";
            eap_o.discipline_id = 2;
            eap_o.total_voted_expertise = 0;
            eap_o.description = "test2";
            eap_o.expiration_time = time_point_sec(0xffffffff);
            eap_o.quorum = 15 * DEIP_1_PERCENT;
        });
        db.create<expertise_allocation_proposal_object>([&](expertise_allocation_proposal_object& eap_o) {
            eap_o.id = 2;
            eap_o.claimer = "alice";
            eap_o.discipline_id = 21;
            eap_o.total_voted_expertise = 100000;
            eap_o.description = "test3";
            eap_o.expiration_time = time_point_sec(0xffffffff);
            eap_o.quorum = 15 * DEIP_1_PERCENT;
        });
    }

    void create_grants()
    {
        db.create<grant_object>([&](grant_object& ga) {
            ga.id = 1;
            ga.target_disciplines = {1};
            ga.min_number_of_positive_reviews = 1;
            ga.min_number_of_applications = 2;
            ga.amount = asset(1000, DEIP_SYMBOL);
            ga.start_date = db.head_block_time() - DAYS_TO_SECONDS(30);
            ga.end_date = db.head_block_time() - DAYS_TO_SECONDS(1);
            ga.grantor = "bob";
            ga.review_committee_id = 1;
        });

        db.create<grant_object>([&](grant_object& ga) {
            ga.id = 2;
            ga.target_disciplines = {2};
            ga.max_number_of_research_to_grant = 3;
            ga.min_number_of_positive_reviews = 1;
            ga.min_number_of_applications = 2;
            ga.amount = asset(1000, DEIP_SYMBOL);
            ga.start_date = db.head_block_time() - DAYS_TO_SECONDS(30);
            ga.end_date = db.head_block_time() - DAYS_TO_SECONDS(1);
            ga.grantor = "jack";
            ga.review_committee_id = 1;
        });
    }

    void create_grant_applications()
    {
        db.create<grant_application_object>([&](grant_application_object& ga_o) {
            ga_o.id = 1;
            ga_o.grant_id = 1;
            ga_o.research_id = 1;
            ga_o.creator = "alice";
            ga_o.application_hash = "test1";
        });

        db.create<grant_application_object>([&](grant_application_object& ga_o) {
            ga_o.id = 2;
            ga_o.grant_id = 1;
            ga_o.research_id = 2;
            ga_o.creator = "mike";
            ga_o.application_hash = "test2";
        });

        db.create<grant_application_object>([&](grant_application_object& ga_o) {
            ga_o.id = 3;
            ga_o.grant_id = 1;
            ga_o.research_id = 3;
            ga_o.creator = "john";
            ga_o.application_hash = "test3";
        });

        db.create<grant_application_object>([&](grant_application_object& ga_o) {
            ga_o.id = 4;
            ga_o.grant_id = 2;
            ga_o.research_id = 1;
            ga_o.creator = "alice";
            ga_o.application_hash = "test4";
        });

        db.create<grant_application_object>([&](grant_application_object& ga_o) {
            ga_o.id = 5;
            ga_o.grant_id = 2;
            ga_o.research_id = 2;
            ga_o.creator = "mike";
            ga_o.application_hash = "test5";
        });

        db.create<grant_application_object>([&](grant_application_object& ga_o) {
            ga_o.id = 6;
            ga_o.grant_id = 2;
            ga_o.research_id = 3;
            ga_o.creator = "john";
            ga_o.application_hash = "test6";
        });
    }

    void create_researches_for_grants()
    {
        db.create<research_object>([&](research_object& d) {
            d.id = 1;
            d.research_group_id = 31;
            d.title = "name1";
            d.abstract = "abstract1";
            d.permlink = "permlink1";
            d.owned_tokens = 100 * DEIP_1_PERCENT;
            d.review_share = 10 * DEIP_1_PERCENT;
            d.number_of_positive_reviews = 10;
        });

        db.create<research_object>([&](research_object& d) {
            d.id = 2;
            d.research_group_id = 32;
            d.title = "name2";
            d.abstract = "abstract2";
            d.permlink = "permlink2";
            d.owned_tokens = 100 * DEIP_1_PERCENT;
            d.review_share = 10 * DEIP_1_PERCENT;
            d.number_of_positive_reviews = 10;
        });

        db.create<research_object>([&](research_object& d) {
            d.id = 3;
            d.research_group_id = 33;
            d.title = "name3";
            d.abstract = "abstract3";
            d.permlink = "permlink3";
            d.owned_tokens = 100 * DEIP_1_PERCENT;
            d.review_share = 10 * DEIP_1_PERCENT;
            d.number_of_positive_reviews = 10;
        });
    }

    void create_research_groups_for_grants()
    {
        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 31;
            d.permlink = "permlink1";
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 32;
            d.permlink = "permlink2";
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 33;
            d.permlink = "permlink3";
        });
    }

    void create_rd_relations_for_grants()
    {
        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
            r.id = 0,
            r.research_id = 1,
            r.discipline_id = 1;
            r.research_eci = 5000;
        });

        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
            r.id = 1,
            r.research_id = 1,
            r.discipline_id = 2;
            r.research_eci = 5000;
        });

        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
            r.id = 2,
            r.research_id = 2,
            r.discipline_id = 1;
            r.research_eci = 3000;
        });

        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
            r.id = 3,
            r.research_id = 2,
            r.discipline_id = 2;
            r.research_eci = 3000;
        });

        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
            r.id = 4,
            r.research_id = 3,
            r.discipline_id = 1;
            r.research_eci = 2000;
        });

        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
            r.id = 5,
            r.research_id = 3,
            r.discipline_id = 2;
            r.research_eci = 2000;
        });
    }

    dbs_account& account_service;
    dbs_review_vote& review_votes_service;
    dbs_research_content& research_content_service;
    dbs_grant& grant_service;
    dbs_account_balance& account_balance_service;
};

BOOST_FIXTURE_TEST_SUITE(database_unit_service, database_unit_service_fixture)


BOOST_AUTO_TEST_CASE(clear_expired_group_invite)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: clear_expired_group_invites");

        ACTORS((alice)(alex)(jack)(bob)(john));

        generate_block();

        research_group_invite_create(0, "alice", 0, 0);

        generate_blocks(DEIP_BLOCKS_PER_HOUR * 2);

        BOOST_CHECK_THROW(db.get<research_group_invite_object>(0), std::out_of_range);
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
