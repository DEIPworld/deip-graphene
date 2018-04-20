#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/protocol/exceptions.hpp>

#include <deip/chain/database.hpp>
#include <deip/chain/database_exceptions.hpp>
#include <deip/chain/hardfork.hpp>
#include <deip/chain/deip_objects.hpp>

#include <deip/chain/util/reward.hpp>

#include <deip/witness/witness_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <deip/chain/research_discipline_relation_object.hpp>
#include <deip/chain/expert_token_object.hpp>

#include <deip/chain/dbs_research_token.hpp>

using namespace deip;
using namespace deip::chain;
using namespace deip::protocol;
using fc::string;

BOOST_AUTO_TEST_SUITE(test_account_create_operation_get_authorities)

BOOST_AUTO_TEST_CASE(there_is_no_owner_authority)
{
    try
    {
        account_create_operation op;
        op.creator = "alice";
        op.new_account_name = "bob";

        flat_set<account_name_type> authorities;

        op.get_required_owner_authorities(authorities);

        BOOST_CHECK(authorities.empty() == true);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(there_is_no_posting_authority)
{
    try
    {
        account_create_operation op;
        op.creator = "alice";
        op.new_account_name = "bob";

        flat_set<account_name_type> authorities;

        op.get_required_posting_authorities(authorities);

        BOOST_CHECK(authorities.empty() == true);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(creator_have_active_authority)
{
    try
    {
        account_create_operation op;
        op.creator = "alice";
        op.new_account_name = "bob";

        flat_set<account_name_type> authorities;

        op.get_required_active_authorities(authorities);

        const flat_set<account_name_type> expected = { "alice" };

        BOOST_CHECK(authorities == expected);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
//
BOOST_FIXTURE_TEST_SUITE(operation_tests, clean_database_fixture)
//
BOOST_AUTO_TEST_CASE(make_review_research_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: make_review_research_apply");

        ACTORS_WITH_EXPERT_TOKENS((alice));

        generate_block();

        auto& research1 = research_create(1, "test_research", "abstract", "permlink", 1, 10, 1500);
        auto& research2 = research_create(2, "test_research2", "abstract2", "permlink2", 1, 10, 1500);
        research_content_create(0, research2.id._id, milestone, "title",
                                "content", 1, active, fc::time_point_sec(),
                                fc::time_point_sec(), {"alice"}, {}, {});

        private_key_type priv_key = generate_private_key("alice");

        make_research_review_operation op;

        std::vector<int64_t> research_references {0};
        op.author = "alice";
        op.research_id = research1.id._id;
        op.title = "test";
        op.content = "test";
        op.references = research_references;
        op.external_references = {"one", "two", "three"};

        BOOST_TEST_MESSAGE("--- Test normal research review creation");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& research_content = db.get<research_content_object, by_id>(1);
        BOOST_CHECK(db.get<research_object>(research_content.research_id).last_update_time == db.head_block_time());
        BOOST_CHECK(research_content.type == review);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(vote_apply_failure)
{

    BOOST_TEST_MESSAGE("Testing: vote_apply failure cases");

    ACTORS_WITH_EXPERT_TOKENS((alice)(bob));

    generate_block();

    auto& research = research_create(1, "test_research", "abstract", "permlink", 1, 10, 1500);
    auto& discipline_service = db.obtain_service<dbs_discipline>();
    auto& discipline = discipline_service.get_discipline(1);

    db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
        r.discipline_id = discipline.id;
        r.research_id = research.id;
        r.votes_count = 0;
    });

    auto& content = db.create<research_content_object>([&](research_content_object& c) {
        c.id = 1;
        c.created_at = fc::time_point_sec(db.head_block_time() - 60 * 60 * 5);
        c.research_id = research.id;
        c.authors = { "alice", "bob" };
        c.content = "content";
        c.references = {};
        c.external_references = { "http://google.com" };
        c.type = research_content_type::milestone;
    });

    private_key_type priv_key = generate_private_key("alice");

    vote_operation op;

    signed_transaction tx;
    tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);

    BOOST_TEST_MESSAGE("--- Testing voting on a non-existent research");

    tx.operations.clear();
    tx.signatures.clear();

    op.research_id = 100;
    op.research_content_id = content.id._id;
    op.discipline_id = discipline.id._id;
    op.weight = 50 * DEIP_1_PERCENT;
    op.voter = "alice";

    tx.operations.push_back(op);
    tx.sign(priv_key, db.get_chain_id());
    tx.validate();

    DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

    validate_database();

    BOOST_TEST_MESSAGE("--- Testing voting on a non-existent content");

    tx.operations.clear();
    tx.signatures.clear();

    op.research_id = research.id._id;
    op.research_content_id = 100;
    op.discipline_id = discipline.id._id;
    op.weight = 50 * DEIP_1_PERCENT;
    op.voter = "alice";

    tx.operations.push_back(op);
    tx.sign(priv_key, db.get_chain_id());
    tx.validate();

    DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

    validate_database();

    BOOST_TEST_MESSAGE("--- Testing voting with non-existent discipline");

    tx.operations.clear();
    tx.signatures.clear();

    op.research_id = research.id._id;
    op.research_content_id = content.id._id;
    op.discipline_id = 100;
    op.weight = 50 * DEIP_1_PERCENT;
    op.voter = "alice";

    tx.operations.push_back(op);
    tx.sign(priv_key, db.get_chain_id());
    tx.validate();

    DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

    validate_database();

    BOOST_TEST_MESSAGE("--- Testing voting with a weight of 0");

    tx.operations.clear();
    tx.signatures.clear();

    op.research_id = research.id._id;
    op.research_content_id = content.id._id;
    op.discipline_id = discipline.id._id;
    op.weight = 0;
    op.voter = "alice";

    tx.operations.push_back(op);
    tx.sign(alice_private_key, db.get_chain_id());

    DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

    validate_database();
}

BOOST_AUTO_TEST_CASE(vote_apply_success)
{
    BOOST_TEST_MESSAGE("Testing: vote_apply success cases");

    ACTORS_WITH_EXPERT_TOKENS((alice)(bob));

    generate_block();

    auto& research = research_create(1, "test_research", "abstract", "permlink", 1, 10, 1500);
    auto& discipline_service = db.obtain_service<dbs_discipline>();
    auto& discipline = discipline_service.get_discipline(1);

    db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
        r.discipline_id = discipline.id;
        r.research_id = research.id;
        r.votes_count = 0;
    });

    auto& expert_token_service = db.obtain_service<dbs_expert_token>();
    auto& token = expert_token_service.get_expert_token_by_account_and_discipline("alice", 1);

    auto& content = db.create<research_content_object>([&](research_content_object& c) {
        c.id = 1;
        c.created_at = fc::time_point_sec(db.head_block_time() - 60 * 60 * 5);
        c.research_id = research.id;
        c.authors = { "alice", "bob" };
        c.content = "content";
        c.references = {};
        c.external_references = { "http://google.com" };
        c.type = research_content_type::milestone;
        c.activity_state = research_content_activity_state::active;
    });

    private_key_type priv_key = generate_private_key("alice");

    vote_operation op;

    signed_transaction tx;
    tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);

    BOOST_TEST_MESSAGE("--- Testing success");

    tx.operations.clear();
    tx.signatures.clear();

    auto old_voting_power = token.voting_power;

    op.research_id = research.id._id;
    op.research_content_id = content.id._id;
    op.discipline_id = discipline.id._id;
    op.weight = 50 * DEIP_1_PERCENT;
    op.voter = "alice";

    tx.operations.clear();
    tx.signatures.clear();
    tx.operations.push_back(op);
    tx.sign(alice_private_key, db.get_chain_id());

    db.push_transaction(tx, 0);

    // Validate token
    BOOST_REQUIRE(token.voting_power == old_voting_power - (old_voting_power * op.weight / DEIP_100_PERCENT / 10));
    BOOST_REQUIRE(token.last_vote_time == db.head_block_time());

    // Validate vote & total_votes objects
    auto& vote_service = db.obtain_service<dbs_vote>();
    auto& total_votes = vote_service.get_total_votes_by_content_and_discipline(op.research_content_id, op.discipline_id);

    const auto& vote_idx = db._temporary_public_impl().get_index<vote_index>().indices().get<by_voter_discipline_and_content>();
    auto itr = vote_idx.find(std::make_tuple(op.voter, op.discipline_id, op.research_content_id));
    auto research_reward_curve = curve_id::power1dot5;
    auto curator_reward_curve = curve_id::power1dot5;
    auto review_reward_curve = curve_id::power1dot5;

    // vote
    BOOST_REQUIRE(itr != vote_idx.end());
    auto& vote = *itr;
    BOOST_REQUIRE(vote.voting_power == (old_voting_power * op.weight / DEIP_100_PERCENT / 10));
    int64_t expected_tokens_amount = (token.amount.value * old_voting_power * op.weight) / (10 * DEIP_100_PERCENT * DEIP_100_PERCENT);
    BOOST_REQUIRE(vote.tokens_amount.value == expected_tokens_amount);
    BOOST_REQUIRE(vote.voting_time == db.head_block_time());
    BOOST_REQUIRE(vote.voter == op.voter);
    BOOST_REQUIRE(vote.discipline_id == op.discipline_id);
    BOOST_REQUIRE(vote.research_id == op.research_id);
    BOOST_REQUIRE(vote.research_content_id == op.research_content_id);

    // Calculate vote weight
    uint64_t expected_curator_reward_weight = util::evaluate_reward_curve(expected_tokens_amount, curator_reward_curve).to_uint64();
    /// discount weight by time
    uint128_t w(expected_curator_reward_weight);
    uint64_t delta_t = std::min(uint64_t((vote.voting_time - content.created_at).to_seconds()),
                                uint64_t(DEIP_REVERSE_AUCTION_WINDOW_SECONDS));

    w *= delta_t;
    w /= DEIP_REVERSE_AUCTION_WINDOW_SECONDS;
    expected_curator_reward_weight = w.to_uint64();
    BOOST_REQUIRE(vote.weight == expected_curator_reward_weight);

    // total_votes
    BOOST_REQUIRE(total_votes.total_weight == expected_tokens_amount);
    BOOST_REQUIRE(total_votes.total_active_weight == expected_tokens_amount);

    uint64_t expected_research_reward_weight = util::evaluate_reward_curve(expected_tokens_amount, research_reward_curve).to_uint64();
    BOOST_REQUIRE(total_votes.total_research_reward_weight == expected_research_reward_weight);
    BOOST_REQUIRE(total_votes.total_active_research_reward_weight == expected_research_reward_weight);

    uint64_t expected_review_reward_weight = content.type == research_content_type::review
                                             ? util::evaluate_reward_curve(expected_tokens_amount, review_reward_curve).to_uint64()
                                             : 0;
    BOOST_REQUIRE(total_votes.total_review_reward_weight == expected_review_reward_weight);
    BOOST_REQUIRE(total_votes.total_active_review_reward_weight == expected_review_reward_weight);

    BOOST_REQUIRE(total_votes.total_curators_reward_weight == expected_curator_reward_weight);
    BOOST_REQUIRE(total_votes.total_active_curators_reward_weight == expected_curator_reward_weight);

    // Validate discipline

    BOOST_REQUIRE(discipline.total_active_reward_weight == expected_tokens_amount);
    BOOST_REQUIRE(discipline.total_active_research_reward_weight == expected_research_reward_weight);
    BOOST_REQUIRE(discipline.total_active_review_reward_weight == expected_review_reward_weight);

    // Validate glopal properties object
    auto& dgpo = db.get_dynamic_global_properties();
    BOOST_REQUIRE(dgpo.total_active_disciplines_reward_weight == expected_tokens_amount);

    validate_database();
}

BOOST_AUTO_TEST_CASE(approve_research_group_invite_apply)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob));

        generate_block();

        auto& research_service = db.obtain_service<dbs_research>();
        auto& research_group_service = db.obtain_service<dbs_research_group>();
        auto& research_token_service = db.obtain_service<dbs_research_token>();

        vector<account_name_type> accounts = { "alice" };

        private_key_type bob_priv_key = generate_private_key("bob");
        private_key_type alice_priv_key = generate_private_key("alice");

           //////////////////////////////////////////////////
          /// Сreate two research groups and invite Bob  ///
         ///                                            ///
        //////////////////////////////////////////////////

        auto& _research_group_1
            = research_group_create_by_operation("alice", "name rg1", "permlink rg1", "description rg1", 50, 100);
        auto& _research_group_2
            = research_group_create_by_operation("alice", "name rg2", "permlink rg2", "description rg2", 50, 200);

        research_group_invite_create(0, "bob", 0, 100);
        research_group_invite_create(1, "bob", 1, 100);

        approve_research_group_invite_operation approve_invite_bob_to_rg1_op;

        approve_invite_bob_to_rg1_op.research_group_invite_id = 0;
        approve_invite_bob_to_rg1_op.owner = "bob";

        approve_research_group_invite_operation approve_invite_bob_to_rg2_op;

        approve_invite_bob_to_rg2_op.research_group_invite_id = 1;
        approve_invite_bob_to_rg2_op.owner = "bob";

        signed_transaction approve_invite_bob_to_rg1_and_rg2_tx;
        approve_invite_bob_to_rg1_and_rg2_tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        approve_invite_bob_to_rg1_and_rg2_tx.operations.push_back(approve_invite_bob_to_rg1_op);
        approve_invite_bob_to_rg1_and_rg2_tx.operations.push_back(approve_invite_bob_to_rg2_op);
        approve_invite_bob_to_rg1_and_rg2_tx.sign(bob_priv_key, db.get_chain_id());
        approve_invite_bob_to_rg1_and_rg2_tx.validate();
        db.push_transaction(approve_invite_bob_to_rg1_and_rg2_tx, 0);

        auto& _research_group_1_token = db.get<research_group_token_object, by_owner>(std::make_tuple("bob", 0));
        auto& _research_group_2_token = db.get<research_group_token_object, by_owner>(std::make_tuple("bob", 1));

        BOOST_CHECK(_research_group_1.total_tokens_amount == 200);
        BOOST_CHECK(_research_group_2.total_tokens_amount == 300);

        BOOST_CHECK(_research_group_1_token.owner == "bob" && _research_group_2_token.owner == "bob");
        BOOST_CHECK(_research_group_1_token.research_group_id == 0 && _research_group_2_token.research_group_id == 1);
        BOOST_CHECK(_research_group_1_token.amount == 100 && _research_group_2_token.amount == 100);

        BOOST_CHECK_THROW((db.get<research_group_invite_object, by_id>(0)), boost::exception);
        BOOST_CHECK_THROW((db.get<research_group_invite_object, by_id>(1)), boost::exception);


           //////////////////////////////////////////////////////////////////////////////////
          /// Сreate three research. Research #1 and Research #2 from Research Group #1. ///
         ///  Research #3 from Research Group #2                                        ///
        //////////////////////////////////////////////////////////////////////////////////

        const std::string create_research_1_proposal_json = "{\"title\":\"research #1\","
                                                            "\"research_group_id\":0,"
                                                            "\"abstract\":\"abstract r1\","
                                                            "\"permlink\":\"permlink r1\","
                                                            "\"review_share_in_percent\": 1000,"
                                                            "\"dropout_compensation_in_percent\": 5000,"
                                                            "\"disciplines\": [1, 2, 3]}";
        const std::string create_research_2_proposal_json = "{\"title\":\"research #2\","
                                                            "\"research_group_id\":0,"
                                                            "\"abstract\":\"abstract r2\","
                                                            "\"permlink\":\"permlink r2\","
                                                            "\"review_share_in_percent\": 1000,"
                                                            "\"dropout_compensation_in_percent\": 2000,"
                                                            "\"disciplines\": [1, 2, 3]}";
        const std::string create_research_3_proposal_json = "{\"title\":\"research #3\","
                                                            "\"research_group_id\":1,"
                                                            "\"abstract\":\"abstract r3\","
                                                            "\"permlink\":\"permlink r3\","
                                                            "\"review_share_in_percent\": 1000,"
                                                            "\"dropout_compensation_in_percent\": 1000,"
                                                            "\"disciplines\": [1, 2, 3]}";

        create_proposal_by_operation("alice", 0, create_research_1_proposal_json,
                                     dbs_proposal::action_t::start_research,
                                     fc::time_point_sec(db.head_block_time().sec_since_epoch() + DAYS_TO_SECONDS(2)));
        create_proposal_by_operation("alice", 0, create_research_2_proposal_json,
                                     dbs_proposal::action_t::start_research,
                                     fc::time_point_sec(db.head_block_time().sec_since_epoch() + DAYS_TO_SECONDS(3)));
        create_proposal_by_operation("alice", 1, create_research_3_proposal_json,
                                     dbs_proposal::action_t::start_research,
                                     fc::time_point_sec(db.head_block_time().sec_since_epoch() + DAYS_TO_SECONDS(4)));

        vote_proposal_operation start_research_1_op;

        start_research_1_op.research_group_id = 0;
        start_research_1_op.proposal_id = 0;
        start_research_1_op.voter = "alice";

        vote_proposal_operation start_research_2_op;

        start_research_2_op.research_group_id = 0;
        start_research_2_op.proposal_id = 1;
        start_research_2_op.voter = "alice";

        vote_proposal_operation start_research_3_op;

        start_research_3_op.research_group_id = 1;
        start_research_3_op.proposal_id = 2;
        start_research_3_op.voter = "alice";

        signed_transaction start_research_tx;
        start_research_tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        start_research_tx.operations.push_back(start_research_1_op);
        start_research_tx.operations.push_back(start_research_2_op);
        start_research_tx.operations.push_back(start_research_3_op);
        start_research_tx.sign(alice_priv_key, db.get_chain_id());
        start_research_tx.validate();
        db.push_transaction(start_research_tx, 0);


           ////////////////////////////////////////////////////////////////
          /// Exclude Bob from Research Group #1 and Research Group #1 ///
         ///                                                          ///
        ////////////////////////////////////////////////////////////////

        const std::string exclude_bob_from_rg1_json = "{\"name\":\"bob\",\"research_group_id\": 0}";
        const std::string exclude_bob_from_rg2_json = "{\"name\":\"bob\",\"research_group_id\": 1}";

        create_proposal_by_operation("alice", 0, exclude_bob_from_rg1_json, dbs_proposal::action_t::dropout_member,
                                     fc::time_point_sec(db.head_block_time().sec_since_epoch() + DAYS_TO_SECONDS(2)));
        create_proposal_by_operation("alice", 1, exclude_bob_from_rg2_json, dbs_proposal::action_t::dropout_member,
                                     fc::time_point_sec(db.head_block_time().sec_since_epoch() + DAYS_TO_SECONDS(3)));

        vote_proposal_operation exclude_bob_from_rg1_op;

        exclude_bob_from_rg1_op.research_group_id = 0;
        exclude_bob_from_rg1_op.proposal_id = 3;
        exclude_bob_from_rg1_op.voter = "alice";

        vote_proposal_operation exclude_bob_from_rg2_op;

        exclude_bob_from_rg2_op.research_group_id = 1;
        exclude_bob_from_rg2_op.proposal_id = 4;
        exclude_bob_from_rg2_op.voter = "alice";

        signed_transaction exclude_bob_from_rg1_and_rg2_tx;
        exclude_bob_from_rg1_and_rg2_tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        exclude_bob_from_rg1_and_rg2_tx.operations.push_back(exclude_bob_from_rg1_op);
        exclude_bob_from_rg1_and_rg2_tx.operations.push_back(exclude_bob_from_rg2_op);
        exclude_bob_from_rg1_and_rg2_tx.sign(alice_priv_key, db.get_chain_id());
        exclude_bob_from_rg1_and_rg2_tx.validate();
        db.push_transaction(exclude_bob_from_rg1_and_rg2_tx, 0);

        auto& research_1 = research_service.get_research(0);
        auto& research_2 = research_service.get_research(1);
        auto& research_3 = research_service.get_research(2);

        auto& research_1_token = research_token_service.get_research_token_by_account_name_and_research_id("bob", 0);
        auto& research_2_token = research_token_service.get_research_token_by_account_name_and_research_id("bob", 1);
        auto& research_3_token = research_token_service.get_research_token_by_account_name_and_research_id("bob", 2);

        BOOST_CHECK_THROW(research_group_service.get_research_group_token_by_account_and_research_group_id("bob", 0),
                          std::out_of_range);
        BOOST_CHECK_THROW(research_group_service.get_research_group_token_by_account_and_research_group_id("bob", 1),
                          std::out_of_range);
        BOOST_CHECK(_research_group_1.total_tokens_amount == 100 && _research_group_2.total_tokens_amount == 200);
        BOOST_CHECK(research_1_token.account_name == "bob" && research_2_token.account_name == "bob"
                    && research_3_token.account_name == "bob");
        BOOST_CHECK(research_1_token.amount == 2500 && research_2_token.amount == 1000
                    && research_3_token.amount == 333);
        BOOST_CHECK(research_1.owned_tokens == 7500 && research_2.owned_tokens == 9000
                    && research_3.owned_tokens == 9667);


           //////////////////////////////////////////////////
          /// Invite Bob again to Research Group #1 only ///
         ///                                            ///
        //////////////////////////////////////////////////

        research_group_invite_create(2, "bob", 0, 100);

        approve_research_group_invite_operation approve_invite_2_to_rg1_op;

        approve_invite_2_to_rg1_op.research_group_invite_id = 2;
        approve_invite_2_to_rg1_op.owner = "bob";

        signed_transaction approve_invite_2_to_rg1_tx;
        approve_invite_2_to_rg1_tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        approve_invite_2_to_rg1_tx.operations.push_back(approve_invite_2_to_rg1_op);
        approve_invite_2_to_rg1_tx.sign(bob_priv_key, db.get_chain_id());
        approve_invite_2_to_rg1_tx.validate();
        db.push_transaction(approve_invite_2_to_rg1_tx, 0);

        BOOST_CHECK(_research_group_1.total_tokens_amount == 200);
        BOOST_CHECK(research_1_token.account_name == "bob");
        BOOST_CHECK(research_1_token.amount == 2500 && research_2_token.amount == 1000
                    && research_3_token.amount == 333);
        BOOST_CHECK(research_1.owned_tokens == 7500 && research_2.owned_tokens == 9000
                    && research_3.owned_tokens == 9667);
        BOOST_CHECK_THROW((db.get<research_group_invite_object, by_id>(3)), boost::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reject_research_group_invite_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: reject_research_group_invite_apply");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob));

        generate_block();

        auto& research_group = research_group_create(1, "name", "permlink", "description", 200, 50, 300);
        research_group_invite_create(1, "bob", 1, 50);

        private_key_type priv_key = generate_private_key("bob");

        reject_research_group_invite_operation op;

        op.research_group_invite_id = 1;
        op.owner = "bob";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& _research_group = db.get<research_group_object, by_id>(1);

        BOOST_CHECK(research_group.total_tokens_amount == 300);
        BOOST_CHECK_THROW((db.get<research_group_invite_object, by_id>(1)), boost::exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(approve_research_group_invite_data_validate_apply)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob));

        vector<account_name_type> accounts = { "alice" };

        private_key_type bob_priv_key = generate_private_key("bob");
        private_key_type alice_priv_key = generate_private_key("alice");

           //////////////////////////////////////////////////
          /// Сreate two research groups and invite Bob  ///
         ///                                            ///
        //////////////////////////////////////////////////

        research_group_create_by_operation("alice", "name rg1", "permlink rg1", "description rg1", 50, 100);
        research_group_create_by_operation("alice", "name rg2", "permlink rg2", "description rg2", 50, 200);

        research_group_invite_create(0, "bob", 0, 100);
        research_group_invite_create(1, "bob", 1, 100);

        approve_research_group_invite_operation approve_invite_bob_to_rg2_with_overflow_data_op;

        approve_invite_bob_to_rg2_with_overflow_data_op.research_group_invite_id = 0;
        approve_invite_bob_to_rg2_with_overflow_data_op.owner = "bob";


        approve_research_group_invite_operation approve_invite_bob_to_rg2_with_negative_data_op;

        approve_invite_bob_to_rg2_with_negative_data_op.research_group_invite_id = 1;
        approve_invite_bob_to_rg2_with_negative_data_op.owner = "bob";
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_create_apply)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: account_create_apply");

       generate_blocks(DEIP_BLOCKS_PER_HOUR);

       private_key_type priv_key = generate_private_key("alice");

       const account_object& init = db.get_account(TEST_INIT_DELEGATE_NAME);
       asset init_starting_balance = init.balance;

       const auto& gpo = db.get_dynamic_global_properties();

       account_create_operation op;

       op.fee = asset(100, DEIP_SYMBOL);
       op.new_account_name = "alice";
       op.creator = TEST_INIT_DELEGATE_NAME;
       op.owner = authority(1, priv_key.get_public_key(), 1);
       op.active = authority(2, priv_key.get_public_key(), 2);
       op.memo_key = priv_key.get_public_key();
       op.json_metadata = "{\"foo\":\"bar\"}";

       BOOST_TEST_MESSAGE("--- Test normal account creation");
       signed_transaction tx;
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.operations.push_back(op);
       tx.sign(init_account_priv_key, db.get_chain_id());
       tx.validate();
       db.push_transaction(tx, 0);

       const account_object& acct = db.get_account("alice");
       const account_authority_object& acct_auth = db.get<account_authority_object, by_account>("alice");

       auto vest_shares = gpo.total_vesting_shares;
       auto vests = gpo.total_vesting_fund_deip;

       BOOST_REQUIRE(acct.name == "alice");
       BOOST_REQUIRE(acct_auth.owner == authority(1, priv_key.get_public_key(), 1));
       BOOST_REQUIRE(acct_auth.active == authority(2, priv_key.get_public_key(), 2));
       BOOST_REQUIRE(acct.memo_key == priv_key.get_public_key());
       BOOST_REQUIRE(acct.proxy == "");
       BOOST_REQUIRE(acct.created == db.head_block_time());
       BOOST_REQUIRE(acct.balance.amount.value == ASSET("0.000 TESTS").amount.value);
       BOOST_REQUIRE(acct.id._id == acct_auth.id._id);

       /// because init_witness has created vesting shares and blocks have been produced, 100 DEIP is worth less than
       /// 100 vesting shares due to rounding
       BOOST_REQUIRE(acct.vesting_shares.amount.value == (op.fee * (vest_shares / vests)).amount.value);
       BOOST_REQUIRE(acct.vesting_withdraw_rate.amount.value == ASSET("0.000000 VESTS").amount.value);
       BOOST_REQUIRE(acct.proxied_vsf_votes_total().value == 0);
       BOOST_REQUIRE((init_starting_balance - ASSET("0.100 TESTS")).amount.value == init.balance.amount.value);
       validate_database();

       BOOST_TEST_MESSAGE("--- Test failure of duplicate account creation");
       BOOST_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);

       BOOST_REQUIRE(acct.name == "alice");
       BOOST_REQUIRE(acct_auth.owner == authority(1, priv_key.get_public_key(), 1));
       BOOST_REQUIRE(acct_auth.active == authority(2, priv_key.get_public_key(), 2));
       BOOST_REQUIRE(acct.memo_key == priv_key.get_public_key());
       BOOST_REQUIRE(acct.proxy == "");
       BOOST_REQUIRE(acct.created == db.head_block_time());
       BOOST_REQUIRE(acct.balance.amount.value == ASSET("0.000 DEIP ").amount.value);
       BOOST_REQUIRE(acct.vesting_shares.amount.value == (op.fee * (vest_shares / vests)).amount.value);
       BOOST_REQUIRE(acct.vesting_withdraw_rate.amount.value == ASSET("0.000000 VESTS").amount.value);
       BOOST_REQUIRE(acct.proxied_vsf_votes_total().value == 0);
       BOOST_REQUIRE((init_starting_balance - ASSET("0.100 TESTS")).amount.value == init.balance.amount.value);
       validate_database();

       BOOST_TEST_MESSAGE("--- Test failure when creator cannot cover fee");
       tx.signatures.clear();
       tx.operations.clear();
       op.fee = asset(db.get_account(TEST_INIT_DELEGATE_NAME).balance.amount + 1, DEIP_SYMBOL);
       op.new_account_name = "bob";
       tx.operations.push_back(op);
       tx.sign(init_account_priv_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
       validate_database();

       BOOST_TEST_MESSAGE("--- Test failure covering witness fee");
       generate_block();
       db_plugin->debug_update([=](database& db) {
           db.modify(db.get_witness_schedule_object(), [&](witness_schedule_object& wso) {
               wso.median_props.account_creation_fee = ASSET("10.000 TESTS");
           });
       });
       generate_block();

       tx.clear();
       op.fee = ASSET("1.000 TESTS");
       tx.operations.push_back(op);
       tx.sign(init_account_priv_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_update_validate)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: account_update_validate");

       ACTORS((alice))

       account_update_operation op;
       op.account = "alice";
       op.posting = authority();
       op.posting->weight_threshold = 1;
       op.posting->add_authorities("abcdefghijklmnopq", 1);

       try
       {
           op.validate();

           signed_transaction tx;
           tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
           tx.operations.push_back(op);
           tx.sign(alice_private_key, db.get_chain_id());
           db.push_transaction(tx, 0);

           BOOST_FAIL("An exception was not thrown for an invalid account name");
       }
       catch (fc::exception&)
       {
       }

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_update_authorities)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: account_update_authorities");

       ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
       private_key_type active_key = generate_private_key("new_key");

       db.modify(db.get<account_authority_object, by_account>("alice"),
                 [&](account_authority_object& a) { a.active = authority(1, active_key.get_public_key(), 1); });

       account_update_operation op;
       op.account = "alice";
       op.json_metadata = "{\"success\":true}";

       signed_transaction tx;
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);

       BOOST_TEST_MESSAGE("  Tests when owner authority is not updated ---");
       BOOST_TEST_MESSAGE("--- Test failure when no signature");
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

       BOOST_TEST_MESSAGE("--- Test failure when wrong signature");
       tx.sign(bob_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

       BOOST_TEST_MESSAGE("--- Test failure when containing additional incorrect signature");
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

       BOOST_TEST_MESSAGE("--- Test failure when containing duplicate signatures");
       tx.signatures.clear();
       tx.sign(active_key, db.get_chain_id());
       tx.sign(active_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

       BOOST_TEST_MESSAGE("--- Test success on active key");
       tx.signatures.clear();
       tx.sign(active_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       BOOST_TEST_MESSAGE("--- Test success on owner key alone");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, database::skip_transaction_dupe_check);

       BOOST_TEST_MESSAGE("  Tests when owner authority is updated ---");
       BOOST_TEST_MESSAGE("--- Test failure when updating the owner authority with an active key");
       tx.signatures.clear();
       tx.operations.clear();
       op.owner = authority(1, active_key.get_public_key(), 1);
       tx.operations.push_back(op);
       tx.sign(active_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_owner_auth);

       BOOST_TEST_MESSAGE("--- Test failure when owner key and active key are present");
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

       BOOST_TEST_MESSAGE("--- Test failure when incorrect signature");
       tx.signatures.clear();
       tx.sign(alice_post_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_owner_auth);

       BOOST_TEST_MESSAGE("--- Test failure when duplicate owner keys are present");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

       BOOST_TEST_MESSAGE("--- Test success when updating the owner authority with an owner key");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_update_apply)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: account_update_apply");

       ACTORS_WITH_EXPERT_TOKENS((alice))
       private_key_type new_private_key = generate_private_key("new_key");

       BOOST_TEST_MESSAGE("--- Test normal update");

       account_update_operation op;
       op.account = "alice";
       op.owner = authority(1, new_private_key.get_public_key(), 1);
       op.active = authority(2, new_private_key.get_public_key(), 2);
       op.memo_key = new_private_key.get_public_key();
       op.json_metadata = "{\"bar\":\"foo\"}";

       signed_transaction tx;
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       const account_object& acct = db.get_account("alice");
       const account_authority_object& acct_auth = db.get<account_authority_object, by_account>("alice");

       BOOST_REQUIRE(acct.name == "alice");
       BOOST_REQUIRE(acct_auth.owner == authority(1, new_private_key.get_public_key(), 1));
       BOOST_REQUIRE(acct_auth.active == authority(2, new_private_key.get_public_key(), 2));
       BOOST_REQUIRE(acct.memo_key == new_private_key.get_public_key());

       /* This is being moved out of consensus
       #ifndef IS_LOW_MEM
          BOOST_REQUIRE( acct.json_metadata == "{\"bar\":\"foo\"}" );
       #else
          BOOST_REQUIRE( acct.json_metadata == "" );
       #endif
       */

       validate_database();

       BOOST_TEST_MESSAGE("--- Test failure when updating a non-existent account");
       tx.operations.clear();
       tx.signatures.clear();
       op.account = "bob";
       tx.operations.push_back(op);
       tx.sign(new_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception)
       validate_database();

       BOOST_TEST_MESSAGE("--- Test failure when account authority does not exist");
       tx.clear();
       op = account_update_operation();
       op.account = "alice";
       op.posting = authority();
       op.posting->weight_threshold = 1;
       op.posting->add_authorities("dave", 1);
       tx.operations.push_back(op);
       tx.sign(new_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(vote_validate)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: vote_validate");

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(vote_authorities)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: vote_authorities");

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_validate)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: transfer_validate");

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_authorities)
{
   try
   {
       ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
       fund("alice", 10000);

       BOOST_TEST_MESSAGE("Testing: transfer_authorities");

       transfer_operation op;
       op.from = "alice";
       op.to = "bob";
       op.amount = ASSET("2.500 TESTS");

       signed_transaction tx;
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.operations.push_back(op);

       BOOST_TEST_MESSAGE("--- Test failure when no signatures");
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

       BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the account's authority");
       tx.sign(alice_post_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

       BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

       BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       tx.sign(bob_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

       BOOST_TEST_MESSAGE("--- Test success with witness signature");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(signature_stripping)
{
   try
   {
       // Alice, Bob and Sam all have 2-of-3 multisig on corp.
       // Legitimate tx signed by (Alice, Bob) goes through.
       // Sam shouldn't be able to add or remove signatures to get the transaction to process multiple times.

       ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(sam)(corp))
       fund("corp", 10000);

       account_update_operation update_op;
       update_op.account = "corp";
       update_op.active = authority(2, "alice", 1, "bob", 1, "sam", 1);

       signed_transaction tx;
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.operations.push_back(update_op);

       tx.sign(corp_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       tx.operations.clear();
       tx.signatures.clear();

       transfer_operation transfer_op;
       transfer_op.from = "corp";
       transfer_op.to = "sam";
       transfer_op.amount = ASSET("1.000 TESTS");

       tx.operations.push_back(transfer_op);

       tx.sign(alice_private_key, db.get_chain_id());
       signature_type alice_sig = tx.signatures.back();
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);
       tx.sign(bob_private_key, db.get_chain_id());
       signature_type bob_sig = tx.signatures.back();
       tx.sign(sam_private_key, db.get_chain_id());
       signature_type sam_sig = tx.signatures.back();
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

       tx.signatures.clear();
       tx.signatures.push_back(alice_sig);
       tx.signatures.push_back(bob_sig);
       db.push_transaction(tx, 0);

       tx.signatures.clear();
       tx.signatures.push_back(alice_sig);
       tx.signatures.push_back(sam_sig);
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_apply)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: transfer_apply");

       ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
       fund("alice", 10000);

       const auto& new_alice = db.get_account("alice");
       const auto& new_bob = db.get_account("bob");

       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("10.000 TESTS").amount.value);
       BOOST_REQUIRE(new_bob.balance.amount.value == ASSET(" 0.000 TESTS").amount.value);

       signed_transaction tx;
       transfer_operation op;

       op.from = "alice";
       op.to = "bob";
       op.amount = ASSET("5.000 TESTS");

       BOOST_TEST_MESSAGE("--- Test normal transaction");
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("5.000 TESTS").amount.value);
       BOOST_REQUIRE(new_bob.balance.amount.value == ASSET("5.000 TESTS").amount.value);
       validate_database();

       BOOST_TEST_MESSAGE("--- Generating a block");
       generate_block();

       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("5.000 TESTS").amount.value);
       BOOST_REQUIRE(new_bob.balance.amount.value == ASSET("5.000 TESTS").amount.value);
       validate_database();

       BOOST_TEST_MESSAGE("--- Test emptying an account");
       tx.signatures.clear();
       tx.operations.clear();
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, database::skip_transaction_dupe_check);

       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("0.000 TESTS").amount.value);
       BOOST_REQUIRE(new_bob.balance.amount.value == ASSET("10.000 TESTS").amount.value);
       validate_database();

       BOOST_TEST_MESSAGE("--- Test transferring non-existent funds");
       tx.signatures.clear();
       tx.operations.clear();
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);

       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("0.000 TESTS").amount.value);
       BOOST_REQUIRE(new_bob.balance.amount.value == ASSET("10.000 TESTS").amount.value);
       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_to_vesting_validate)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: transfer_to_vesting_validate");

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_to_vesting_authorities)
{
   try
   {
       ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
       fund("alice", 10000);

       BOOST_TEST_MESSAGE("Testing: transfer_to_vesting_authorities");

       transfer_to_vesting_operation op;
       op.from = "alice";
       op.to = "bob";
       op.amount = ASSET("2.500 TESTS");

       signed_transaction tx;
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.operations.push_back(op);

       BOOST_TEST_MESSAGE("--- Test failure when no signatures");
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

       BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the account's authority");
       tx.sign(alice_post_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

       BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

       BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       tx.sign(bob_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

       BOOST_TEST_MESSAGE("--- Test success with from signature");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_to_vesting_apply)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: transfer_to_vesting_apply");

       ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
       fund("alice", 10000);

       const auto& new_alice = db.get_account("alice");
       const auto& new_bob = db.get_account("bob");

       const auto& gpo = db.get_dynamic_global_properties();

       BOOST_REQUIRE(new_alice.balance == ASSET("10.000 TESTS"));

       auto shares = asset(gpo.total_vesting_shares.amount, VESTS_SYMBOL);
       auto vests = asset(gpo.total_vesting_fund_deip.amount, DEIP_SYMBOL);
       auto alice_shares = new_alice.vesting_shares;
       auto bob_shares = new_bob.vesting_shares;

       transfer_to_vesting_operation op;
       op.from = "alice";
       op.to = "";
       op.amount = ASSET("7.500 TESTS");

       signed_transaction tx;
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       auto new_vest = op.amount * (shares / vests);
       shares += new_vest;
       vests += op.amount;
       alice_shares += new_vest;

       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("2.500 TESTS").amount.value);
       BOOST_REQUIRE(new_alice.vesting_shares.amount.value == alice_shares.amount.value);
       BOOST_REQUIRE(gpo.total_vesting_fund_deip.amount.value == vests.amount.value);
       BOOST_REQUIRE(gpo.total_vesting_shares.amount.value == shares.amount.value);
       validate_database();

       op.to = "bob";
       op.amount = asset(2000, DEIP_SYMBOL);
       tx.operations.clear();
       tx.signatures.clear();
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       new_vest = asset((op.amount * (shares / vests)).amount, VESTS_SYMBOL);
       shares += new_vest;
       vests += op.amount;
       bob_shares += new_vest;

       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("0.500 TESTS").amount.value);
       BOOST_REQUIRE(new_alice.vesting_shares.amount.value == alice_shares.amount.value);
       BOOST_REQUIRE(new_bob.balance.amount.value == ASSET("0.000 TESTS").amount.value);
       BOOST_REQUIRE(new_bob.vesting_shares.amount.value == bob_shares.amount.value);
       BOOST_REQUIRE(gpo.total_vesting_fund_deip.amount.value == vests.amount.value);
       BOOST_REQUIRE(gpo.total_vesting_shares.amount.value == shares.amount.value);
       validate_database();

       DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);

       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("0.500 TESTS").amount.value);
       BOOST_REQUIRE(new_alice.vesting_shares.amount.value == alice_shares.amount.value);
       BOOST_REQUIRE(new_bob.balance.amount.value == ASSET("0.000 TESTS").amount.value);
       BOOST_REQUIRE(new_bob.vesting_shares.amount.value == bob_shares.amount.value);
       BOOST_REQUIRE(gpo.total_vesting_fund_deip.amount.value == vests.amount.value);
       BOOST_REQUIRE(gpo.total_vesting_shares.amount.value == shares.amount.value);
       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(withdraw_vesting_validate)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: withdraw_vesting_validate");

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(withdraw_vesting_authorities)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: withdraw_vesting_authorities");

       ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
       fund("alice", 10000);
       vest("alice", 10000);

       withdraw_vesting_operation op;
       op.account = "alice";
       op.vesting_shares = ASSET("0.001000 VESTS");

       signed_transaction tx;
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);

       BOOST_TEST_MESSAGE("--- Test failure when no signature.");
       DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_missing_active_auth);

       BOOST_TEST_MESSAGE("--- Test success with account signature");
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, database::skip_transaction_dupe_check);

       BOOST_TEST_MESSAGE("--- Test failure with duplicate signature");
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_duplicate_sig);

       BOOST_TEST_MESSAGE("--- Test failure with additional incorrect signature");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       tx.sign(bob_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_irrelevant_sig);

       BOOST_TEST_MESSAGE("--- Test failure with incorrect signature");
       tx.signatures.clear();
       tx.sign(alice_post_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_missing_active_auth);

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(withdraw_vesting_apply)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: withdraw_vesting_apply");

       ACTORS_WITH_EXPERT_TOKENS((alice))
       generate_block();

       vest("alice", ASSET("10.000 TESTS"));

       BOOST_TEST_MESSAGE("--- Test withdraw of existing VESTS");

       {
           const auto& alice = db.get_account("alice");

           withdraw_vesting_operation op;
           op.account = "alice";
           op.vesting_shares = asset(alice.vesting_shares.amount / 2, VESTS_SYMBOL);

           auto old_vesting_shares = alice.vesting_shares;

           signed_transaction tx;
           tx.operations.push_back(op);
           tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
           tx.sign(alice_private_key, db.get_chain_id());
           db.push_transaction(tx, 0);

           BOOST_REQUIRE(alice.vesting_shares.amount.value == old_vesting_shares.amount.value);
           BOOST_REQUIRE(alice.vesting_withdraw_rate.amount.value
                         == (old_vesting_shares.amount / (DEIP_VESTING_WITHDRAW_INTERVALS * 2)).value);
           BOOST_REQUIRE(alice.to_withdraw.value == op.vesting_shares.amount.value);
           BOOST_REQUIRE(alice.next_vesting_withdrawal
                         == db.head_block_time() + DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS);
           validate_database();

           BOOST_TEST_MESSAGE("--- Test changing vesting withdrawal");
           tx.operations.clear();
           tx.signatures.clear();

           op.vesting_shares = asset(alice.vesting_shares.amount / 3, VESTS_SYMBOL);
           tx.operations.push_back(op);
           tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
           tx.sign(alice_private_key, db.get_chain_id());
           db.push_transaction(tx, 0);

           BOOST_REQUIRE(alice.vesting_shares.amount.value == old_vesting_shares.amount.value);
           BOOST_REQUIRE(alice.vesting_withdraw_rate.amount.value
                         == (old_vesting_shares.amount / (DEIP_VESTING_WITHDRAW_INTERVALS * 3)).value);
           BOOST_REQUIRE(alice.to_withdraw.value == op.vesting_shares.amount.value);
           BOOST_REQUIRE(alice.next_vesting_withdrawal
                         == db.head_block_time() + DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS);
           validate_database();

           BOOST_TEST_MESSAGE("--- Test withdrawing more vests than available");

           tx.operations.clear();
           tx.signatures.clear();

           op.vesting_shares = asset(alice.vesting_shares.amount * 2, VESTS_SYMBOL);
           tx.operations.push_back(op);
           tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
           tx.sign(alice_private_key, db.get_chain_id());
           DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

           BOOST_REQUIRE(alice.vesting_shares.amount.value == old_vesting_shares.amount.value);
           BOOST_REQUIRE(alice.vesting_withdraw_rate.amount.value
                         == (old_vesting_shares.amount / (DEIP_VESTING_WITHDRAW_INTERVALS * 3)).value);
           BOOST_REQUIRE(alice.next_vesting_withdrawal
                         == db.head_block_time() + DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS);
           validate_database();

           BOOST_TEST_MESSAGE("--- Test withdrawing 0 to reset vesting withdraw");
           tx.operations.clear();
           tx.signatures.clear();

           op.vesting_shares = asset(0, VESTS_SYMBOL);
           tx.operations.push_back(op);
           tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
           tx.sign(alice_private_key, db.get_chain_id());
           db.push_transaction(tx, 0);

           BOOST_REQUIRE(alice.vesting_shares.amount.value == old_vesting_shares.amount.value);
           BOOST_REQUIRE(alice.vesting_withdraw_rate.amount.value == 0);
           BOOST_REQUIRE(alice.to_withdraw.value == 0);
           BOOST_REQUIRE(alice.next_vesting_withdrawal == fc::time_point_sec::maximum());

           BOOST_TEST_MESSAGE("--- Test cancelling a withdraw when below the account creation fee");
           op.vesting_shares = alice.vesting_shares;
           tx.clear();
           tx.operations.push_back(op);
           tx.sign(alice_private_key, db.get_chain_id());
           db.push_transaction(tx, 0);
           generate_block();
       }

       db_plugin->debug_update(
           [=](database& db) {
               auto& wso = db.get_witness_schedule_object();

               db.modify(wso, [&](witness_schedule_object& w) {
                   w.median_props.account_creation_fee = ASSET("10.000 TESTS");
               });

               db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& gpo) {
                   gpo.current_supply
                       += wso.median_props.account_creation_fee - ASSET("0.001 TESTS") - gpo.total_vesting_fund_deip;
                   gpo.total_vesting_fund_deip = wso.median_props.account_creation_fee - ASSET("0.001 TESTS");
               });
           },
           database::skip_witness_signature);

       withdraw_vesting_operation op;
       signed_transaction tx;
       op.account = "alice";
       op.vesting_shares = ASSET("0.000000 VESTS");
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       BOOST_REQUIRE(db.get_account("alice").vesting_withdraw_rate == ASSET("0.000000 VESTS"));
       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(witness_update_validate)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: withness_update_validate");

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(witness_update_authorities)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: witness_update_authorities");

       ACTORS_WITH_EXPERT_TOKENS((alice)(bob));
       fund("alice", 10000);

       private_key_type signing_key = generate_private_key("new_key");

       witness_update_operation op;
       op.owner = "alice";
       op.url = "foo.bar";
       op.fee = ASSET("1.000 TESTS");
       op.block_signing_key = signing_key.get_public_key();

       signed_transaction tx;
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.operations.push_back(op);

       BOOST_TEST_MESSAGE("--- Test failure when no signatures");
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

       BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the account's authority");
       tx.sign(alice_post_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

       BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

       BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       tx.sign(bob_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

       BOOST_TEST_MESSAGE("--- Test success with witness signature");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       tx.signatures.clear();
       tx.sign(signing_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_missing_active_auth);
       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(witness_update_apply)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: witness_update_apply");

       ACTORS_WITH_EXPERT_TOKENS((alice))

       const auto& new_alice = db.get_account("alice");

       fund("alice", 10000);

       private_key_type signing_key = generate_private_key("new_key");

       BOOST_TEST_MESSAGE("--- Test upgrading an account to a witness");

       witness_update_operation op;
       op.owner = "alice";
       op.url = "foo.bar";
       op.fee = ASSET("1.000 TESTS");
       op.block_signing_key = signing_key.get_public_key();
       op.props.account_creation_fee = asset(DEIP_MIN_ACCOUNT_CREATION_FEE + 10, DEIP_SYMBOL);
       op.props.maximum_block_size = DEIP_MIN_BLOCK_SIZE_LIMIT + 100;

       signed_transaction tx;
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.operations.push_back(op);
       tx.sign(alice_private_key, db.get_chain_id());

       db.push_transaction(tx, 0);

       const witness_object& alice_witness = db.get_witness("alice");

       BOOST_REQUIRE(alice_witness.owner == "alice");
       BOOST_REQUIRE(alice_witness.created == db.head_block_time());
       BOOST_REQUIRE(fc::to_string(alice_witness.url) == op.url);
       BOOST_REQUIRE(alice_witness.signing_key == op.block_signing_key);
       BOOST_REQUIRE(alice_witness.props.account_creation_fee == op.props.account_creation_fee);
       BOOST_REQUIRE(alice_witness.props.maximum_block_size == op.props.maximum_block_size);
       BOOST_REQUIRE(alice_witness.total_missed == 0);
       BOOST_REQUIRE(alice_witness.last_aslot == 0);
       BOOST_REQUIRE(alice_witness.last_confirmed_block_num == 0);
       BOOST_REQUIRE(alice_witness.votes.value == 0);
       BOOST_REQUIRE(alice_witness.virtual_last_update == 0);
       BOOST_REQUIRE(alice_witness.virtual_position == 0);
       BOOST_REQUIRE(alice_witness.virtual_scheduled_time == fc::uint128_t::max_value());
       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("10.000 TESTS").amount.value); // No fee
       validate_database();

       BOOST_TEST_MESSAGE("--- Test updating a witness");

       tx.signatures.clear();
       tx.operations.clear();
       op.url = "bar.foo";
       tx.operations.push_back(op);
       tx.sign(alice_private_key, db.get_chain_id());

       db.push_transaction(tx, 0);

       BOOST_REQUIRE(alice_witness.owner == "alice");
       BOOST_REQUIRE(alice_witness.created == db.head_block_time());
       BOOST_REQUIRE(fc::to_string(alice_witness.url) == "bar.foo");
       BOOST_REQUIRE(alice_witness.signing_key == op.block_signing_key);
       BOOST_REQUIRE(alice_witness.props.account_creation_fee == op.props.account_creation_fee);
       BOOST_REQUIRE(alice_witness.props.maximum_block_size == op.props.maximum_block_size);
       BOOST_REQUIRE(alice_witness.total_missed == 0);
       BOOST_REQUIRE(alice_witness.last_aslot == 0);
       BOOST_REQUIRE(alice_witness.last_confirmed_block_num == 0);
       BOOST_REQUIRE(alice_witness.votes.value == 0);
       BOOST_REQUIRE(alice_witness.virtual_last_update == 0);
       BOOST_REQUIRE(alice_witness.virtual_position == 0);
       BOOST_REQUIRE(alice_witness.virtual_scheduled_time == fc::uint128_t::max_value());
       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("10.000 TESTS").amount.value);
       validate_database();

       BOOST_TEST_MESSAGE("--- Test failure when upgrading a non-existent account");

       tx.signatures.clear();
       tx.operations.clear();
       op.owner = "bob";
       tx.operations.push_back(op);
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_witness_vote_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_witness_vote_validate");

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_witness_vote_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_witness_vote_authorities");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(sam))

        fund("alice", 1000);
        private_key_type alice_witness_key = generate_private_key("alice_witness");
        witness_create("alice", alice_private_key, "foo.bar", alice_witness_key.get_public_key(), 1000);

        account_witness_vote_operation op;
        op.account = "bob";
        op.witness = "alice";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);

        BOOST_TEST_MESSAGE("--- Test failure when no signatures");
        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the account's authority");
        tx.sign(bob_post_key, db.get_chain_id());
        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
        tx.signatures.clear();
        tx.sign(bob_private_key, db.get_chain_id());
        tx.sign(bob_private_key, db.get_chain_id());
        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
        tx.signatures.clear();
        tx.sign(bob_private_key, db.get_chain_id());
        tx.sign(alice_private_key, db.get_chain_id());
        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("--- Test success with witness signature");
        tx.signatures.clear();
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("--- Test failure with proxy signature");
        proxy("bob", "sam");
        tx.signatures.clear();
        tx.sign(sam_private_key, db.get_chain_id());
        DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_missing_active_auth);

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_witness_vote_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_witness_vote_apply");

        ACTORS((alice)(bob)(sam))

        expert_token("sam", 1, 1000);

        fund("alice", 5000);

        for (int i = 1; i < 4; ++i)
            expert_token("alice", i, i * 100);

        vest("alice", 5000);
        fund("sam", 1000);

        const auto& new_alice = db.get_account("alice");

        private_key_type sam_witness_key = generate_private_key("sam_key");
        witness_create("sam", sam_private_key, "foo.bar", sam_witness_key.get_public_key(), 1000);
        const witness_object& sam_witness = db.get_witness("sam");

        const auto& witness_vote_idx = db.get_index<witness_vote_index>().indices().get<by_witness_account>();

        BOOST_TEST_MESSAGE("--- Test normal vote");
        account_witness_vote_operation op;
        op.account = "alice";
        op.witness = "sam";
        op.approve = true;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        auto expert_tokens_by_account = db.get_index<expert_token_index>().indices().get<by_account_name>().equal_range("alice");

        auto it = expert_tokens_by_account.first;
        const auto it_end = expert_tokens_by_account.second;

        share_type alice_total_vote_weight;
        while (it != it_end)
        {
            alice_total_vote_weight += it->amount;
            ++it;
        }

        BOOST_REQUIRE(sam_witness.votes == alice_total_vote_weight);
        BOOST_REQUIRE(witness_vote_idx.find(std::make_tuple(sam_witness.id, new_alice.id)) != witness_vote_idx.end());
        validate_database();

        BOOST_TEST_MESSAGE("--- Test revoke vote");
        op.approve = false;
        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);
        BOOST_REQUIRE(sam_witness.votes.value == 0);
        BOOST_REQUIRE(witness_vote_idx.find(std::make_tuple(sam_witness.id, new_alice.id)) == witness_vote_idx.end());

        BOOST_TEST_MESSAGE("--- Test failure when attempting to revoke a non-existent vote");

        DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);
        BOOST_REQUIRE(sam_witness.votes.value == 0);
        BOOST_REQUIRE(witness_vote_idx.find(std::make_tuple(sam_witness.id, new_alice.id)) == witness_vote_idx.end());

        BOOST_TEST_MESSAGE("--- Test failure when voting for a non-existent account");
        tx.operations.clear();
        tx.signatures.clear();
        op.witness = "dave";
        op.approve = true;
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());

        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test failure when voting for an account that is not a witness");
        tx.operations.clear();
        tx.signatures.clear();
        op.witness = "alice";
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());

        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_witness_proxy_validate)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: account_witness_proxy_validate");

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_witness_proxy_authorities)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: account_witness_proxy_authorities");

       ACTORS_WITH_EXPERT_TOKENS((alice)(bob))

       account_witness_proxy_operation op;
       op.account = "bob";
       op.proxy = "alice";

       signed_transaction tx;
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.operations.push_back(op);

       BOOST_TEST_MESSAGE("--- Test failure when no signatures");
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

       BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the account's authority");
       tx.sign(bob_post_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

       BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
       tx.signatures.clear();
       tx.sign(bob_private_key, db.get_chain_id());
       tx.sign(bob_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

       BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
       tx.signatures.clear();
       tx.sign(bob_private_key, db.get_chain_id());
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

       BOOST_TEST_MESSAGE("--- Test success with witness signature");
       tx.signatures.clear();
       tx.sign(bob_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       BOOST_TEST_MESSAGE("--- Test failure with proxy signature");
       tx.signatures.clear();
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_missing_active_auth);

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_recovery)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: account recovery");

       ACTORS_WITH_EXPERT_TOKENS((alice));
       fund("alice", 1000000);

       BOOST_TEST_MESSAGE("Creating account bob with alice");

       account_create_with_delegation_operation acc_create;
       acc_create.fee = ASSET("10.000 TESTS");
       acc_create.delegation = ASSET("0.000000 VESTS");
       acc_create.creator = "alice";
       acc_create.new_account_name = "bob";
       acc_create.owner = authority(1, generate_private_key("bob_owner").get_public_key(), 1);
       acc_create.active = authority(1, generate_private_key("bob_active").get_public_key(), 1);
       acc_create.posting = authority(1, generate_private_key("bob_posting").get_public_key(), 1);
       acc_create.memo_key = generate_private_key("bob_memo").get_public_key();
       acc_create.json_metadata = "";

       signed_transaction tx;
       tx.operations.push_back(acc_create);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       generate_block();
       expert_token("bob", 1, 10000);

       const auto& bob_auth = db.get<account_authority_object, by_account>("bob");
       BOOST_REQUIRE(bob_auth.owner == acc_create.owner);

       BOOST_TEST_MESSAGE("Changing bob's owner authority");

       account_update_operation acc_update;
       acc_update.account = "bob";
       acc_update.owner = authority(1, generate_private_key("bad_key").get_public_key(), 1);
       acc_update.memo_key = acc_create.memo_key;
       acc_update.json_metadata = "";

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(acc_update);
       tx.sign(generate_private_key("bob_owner"), db.get_chain_id());
       db.push_transaction(tx, 0);

       BOOST_REQUIRE(bob_auth.owner == *acc_update.owner);

       BOOST_TEST_MESSAGE("Creating recover request for bob with alice");

       request_account_recovery_operation request;
       request.recovery_account = "alice";
       request.account_to_recover = "bob";
       request.new_owner_authority = authority(1, generate_private_key("new_key").get_public_key(), 1);

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(request);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       BOOST_REQUIRE(bob_auth.owner == *acc_update.owner);

       BOOST_TEST_MESSAGE("Recovering bob's account with original owner auth and new secret");

       generate_blocks(db.head_block_time() + DEIP_OWNER_UPDATE_LIMIT);

       recover_account_operation recover;
       recover.account_to_recover = "bob";
       recover.new_owner_authority = request.new_owner_authority;
       recover.recent_owner_authority = acc_create.owner;

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(recover);
       tx.sign(generate_private_key("bob_owner"), db.get_chain_id());
       tx.sign(generate_private_key("new_key"), db.get_chain_id());
       db.push_transaction(tx, 0);
       const auto& owner1 = db.get<account_authority_object, by_account>("bob").owner;

       BOOST_REQUIRE(owner1 == recover.new_owner_authority);

       BOOST_TEST_MESSAGE("Creating new recover request for a bogus key");

       request.new_owner_authority = authority(1, generate_private_key("foo bar").get_public_key(), 1);

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(request);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       BOOST_TEST_MESSAGE("Testing failure when bob does not have new authority");

       generate_blocks(db.head_block_time() + DEIP_OWNER_UPDATE_LIMIT + fc::seconds(DEIP_BLOCK_INTERVAL));

       recover.new_owner_authority = authority(1, generate_private_key("idontknow").get_public_key(), 1);

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(recover);
       tx.sign(generate_private_key("bob_owner"), db.get_chain_id());
       tx.sign(generate_private_key("idontknow"), db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
       const auto& owner2 = db.get<account_authority_object, by_account>("bob").owner;
       BOOST_REQUIRE(owner2 == authority(1, generate_private_key("new_key").get_public_key(), 1));

       BOOST_TEST_MESSAGE("Testing failure when bob does not have old authority");

       recover.recent_owner_authority = authority(1, generate_private_key("idontknow").get_public_key(), 1);
       recover.new_owner_authority = authority(1, generate_private_key("foo bar").get_public_key(), 1);

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(recover);
       tx.sign(generate_private_key("foo bar"), db.get_chain_id());
       tx.sign(generate_private_key("idontknow"), db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
       const auto& owner3 = db.get<account_authority_object, by_account>("bob").owner;
       BOOST_REQUIRE(owner3 == authority(1, generate_private_key("new_key").get_public_key(), 1));

       BOOST_TEST_MESSAGE("Testing using the same old owner auth again for recovery");

       recover.recent_owner_authority = authority(1, generate_private_key("bob_owner").get_public_key(), 1);
       recover.new_owner_authority = authority(1, generate_private_key("foo bar").get_public_key(), 1);

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(recover);
       tx.sign(generate_private_key("bob_owner"), db.get_chain_id());
       tx.sign(generate_private_key("foo bar"), db.get_chain_id());
       db.push_transaction(tx, 0);

       const auto& owner4 = db.get<account_authority_object, by_account>("bob").owner;
       BOOST_REQUIRE(owner4 == recover.new_owner_authority);

       BOOST_TEST_MESSAGE("Creating a recovery request that will expire");

       request.new_owner_authority = authority(1, generate_private_key("expire").get_public_key(), 1);

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(request);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       const auto& request_idx = db.get_index<account_recovery_request_index>().indices();
       auto req_itr = request_idx.begin();

       BOOST_REQUIRE(req_itr->account_to_recover == "bob");
       BOOST_REQUIRE(req_itr->new_owner_authority == authority(1, generate_private_key("expire").get_public_key(), 1));
       BOOST_REQUIRE(req_itr->expires == db.head_block_time() + DEIP_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD);
       auto expires = req_itr->expires;
       ++req_itr;
       BOOST_REQUIRE(req_itr == request_idx.end());

       generate_blocks(time_point_sec(expires - DEIP_BLOCK_INTERVAL), true);

       const auto& new_request_idx = db.get_index<account_recovery_request_index>().indices();
       BOOST_REQUIRE(new_request_idx.begin() != new_request_idx.end());

       generate_block();

       BOOST_REQUIRE(new_request_idx.begin() == new_request_idx.end());

       recover.new_owner_authority = authority(1, generate_private_key("expire").get_public_key(), 1);
       recover.recent_owner_authority = authority(1, generate_private_key("bob_owner").get_public_key(), 1);

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(recover);
       tx.set_expiration(db.head_block_time());
       tx.sign(generate_private_key("expire"), db.get_chain_id());
       tx.sign(generate_private_key("bob_owner"), db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
       const auto& owner5 = db.get<account_authority_object, by_account>("bob").owner;
       BOOST_REQUIRE(owner5 == authority(1, generate_private_key("foo bar").get_public_key(), 1));

       BOOST_TEST_MESSAGE("Expiring owner authority history");

       acc_update.owner = authority(1, generate_private_key("new_key").get_public_key(), 1);

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(acc_update);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(generate_private_key("foo bar"), db.get_chain_id());
       db.push_transaction(tx, 0);

       generate_blocks(db.head_block_time()
                       + (DEIP_OWNER_AUTH_RECOVERY_PERIOD - DEIP_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD));
       generate_block();

       request.new_owner_authority = authority(1, generate_private_key("last key").get_public_key(), 1);

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(request);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       recover.new_owner_authority = request.new_owner_authority;
       recover.recent_owner_authority = authority(1, generate_private_key("bob_owner").get_public_key(), 1);

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(recover);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(generate_private_key("bob_owner"), db.get_chain_id());
       tx.sign(generate_private_key("last key"), db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
       const auto& owner6 = db.get<account_authority_object, by_account>("bob").owner;
       BOOST_REQUIRE(owner6 == authority(1, generate_private_key("new_key").get_public_key(), 1));

       recover.recent_owner_authority = authority(1, generate_private_key("foo bar").get_public_key(), 1);

       tx.operations.clear();
       tx.signatures.clear();

       tx.operations.push_back(recover);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(generate_private_key("foo bar"), db.get_chain_id());
       tx.sign(generate_private_key("last key"), db.get_chain_id());
       db.push_transaction(tx, 0);
       const auto& owner7 = db.get<account_authority_object, by_account>("bob").owner;
       BOOST_REQUIRE(owner7 == authority(1, generate_private_key("last key").get_public_key(), 1));
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_recovery_account)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing change_recovery_account_operation");

       ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(sam)(tyler))

       auto change_recovery_account
           = [&](const std::string& account_to_recover, const std::string& new_recovery_account) {
                 change_recovery_account_operation op;
                 op.account_to_recover = account_to_recover;
                 op.new_recovery_account = new_recovery_account;

                 signed_transaction tx;
                 tx.operations.push_back(op);
                 tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
                 tx.sign(alice_private_key, db.get_chain_id());
                 db.push_transaction(tx, 0);
             };

       auto recover_account = [&](const std::string& account_to_recover, const fc::ecc::private_key& new_owner_key,
                                  const fc::ecc::private_key& recent_owner_key) {
           recover_account_operation op;
           op.account_to_recover = account_to_recover;
           op.new_owner_authority = authority(1, public_key_type(new_owner_key.get_public_key()), 1);
           op.recent_owner_authority = authority(1, public_key_type(recent_owner_key.get_public_key()), 1);

           signed_transaction tx;
           tx.operations.push_back(op);
           tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
           tx.sign(recent_owner_key, db.get_chain_id());
           // only Alice -> throw
           DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
           tx.signatures.clear();
           tx.sign(new_owner_key, db.get_chain_id());
           // only Sam -> throw
           DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
           tx.sign(recent_owner_key, db.get_chain_id());
           // Alice+Sam -> OK
           db.push_transaction(tx, 0);
       };

       auto request_account_recovery
           = [&](const std::string& recovery_account, const fc::ecc::private_key& recovery_account_key,
                 const std::string& account_to_recover, const public_key_type& new_owner_key) {
                 request_account_recovery_operation op;
                 op.recovery_account = recovery_account;
                 op.account_to_recover = account_to_recover;
                 op.new_owner_authority = authority(1, new_owner_key, 1);

                 signed_transaction tx;
                 tx.operations.push_back(op);
                 tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
                 tx.sign(recovery_account_key, db.get_chain_id());
                 db.push_transaction(tx, 0);
             };

       auto change_owner = [&](const std::string& account, const fc::ecc::private_key& old_private_key,
                               const public_key_type& new_public_key) {
           account_update_operation op;
           op.account = account;
           op.owner = authority(1, new_public_key, 1);

           signed_transaction tx;
           tx.operations.push_back(op);
           tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
           tx.sign(old_private_key, db.get_chain_id());
           db.push_transaction(tx, 0);
       };

       // if either/both users do not exist, we shouldn't allow it
       DEIP_REQUIRE_THROW(change_recovery_account("alice", "nobody"), fc::exception);
       DEIP_REQUIRE_THROW(change_recovery_account("haxer", "sam"), fc::exception);
       DEIP_REQUIRE_THROW(change_recovery_account("haxer", "nobody"), fc::exception);
       change_recovery_account("alice", "sam");

       fc::ecc::private_key alice_priv1 = fc::ecc::private_key::regenerate(fc::sha256::hash("alice_k1"));
       fc::ecc::private_key alice_priv2 = fc::ecc::private_key::regenerate(fc::sha256::hash("alice_k2"));
       public_key_type alice_pub1 = public_key_type(alice_priv1.get_public_key());

       generate_blocks(db.head_block_time() + DEIP_OWNER_AUTH_RECOVERY_PERIOD - fc::seconds(DEIP_BLOCK_INTERVAL),
                       true);
       // cannot request account recovery until recovery account is approved
       DEIP_REQUIRE_THROW(request_account_recovery("sam", sam_private_key, "alice", alice_pub1), fc::exception);
       generate_blocks(1);
       // cannot finish account recovery until requested
       DEIP_REQUIRE_THROW(recover_account("alice", alice_priv1, alice_private_key), fc::exception);
       // do the request
       request_account_recovery("sam", sam_private_key, "alice", alice_pub1);
       // can't recover with the current owner key
       DEIP_REQUIRE_THROW(recover_account("alice", alice_priv1, alice_private_key), fc::exception);
       // unless we change it!
       change_owner("alice", alice_private_key, public_key_type(alice_priv2.get_public_key()));
       recover_account("alice", alice_priv1, alice_private_key);
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_bandwidth)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: account_bandwidth");
       ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
       generate_block();
       vest("alice", ASSET("10.000 TESTS"));
       fund("alice", ASSET("10.000 TESTS"));
       vest("bob", ASSET("10.000 TESTS"));

       generate_block();

       BOOST_TEST_MESSAGE("--- Test first tx in block");

       signed_transaction tx;
       transfer_operation op;

       op.from = "alice";
       op.to = "bob";
       op.amount = ASSET("1.000 TESTS");

       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());

       db.push_transaction(tx, 0);

       auto last_bandwidth_update = db.get<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
                                          boost::make_tuple("alice", witness::bandwidth_type::market))
                                        .last_bandwidth_update;
       auto average_bandwidth = db.get<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
                                      boost::make_tuple("alice", witness::bandwidth_type::market))
                                    .average_bandwidth;
       BOOST_REQUIRE(last_bandwidth_update == db.head_block_time());
       BOOST_REQUIRE(average_bandwidth == fc::raw::pack_size(tx) * 10 * DEIP_BANDWIDTH_PRECISION);
       auto total_bandwidth = average_bandwidth;

       BOOST_TEST_MESSAGE("--- Test second tx in block");

       op.amount = ASSET("0.100 TESTS");
       tx.clear();
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());

       db.push_transaction(tx, 0);

       last_bandwidth_update = db.get<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
                                     boost::make_tuple("alice", witness::bandwidth_type::market))
                                   .last_bandwidth_update;
       average_bandwidth = db.get<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
                                 boost::make_tuple("alice", witness::bandwidth_type::market))
                               .average_bandwidth;
       BOOST_REQUIRE(last_bandwidth_update == db.head_block_time());
       BOOST_REQUIRE(average_bandwidth == total_bandwidth + fc::raw::pack_size(tx) * 10 * DEIP_BANDWIDTH_PRECISION);
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_create_with_delegation_authorities)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: account_create_with_delegation_authorities");

       signed_transaction tx;
       ACTORS_WITH_EXPERT_TOKENS((alice));
       generate_blocks(1);
       fund("alice", ASSET("1000.000 TESTS"));
       vest("alice", ASSET("10000.000000 VESTS"));

       private_key_type priv_key = generate_private_key("temp_key");

       account_create_with_delegation_operation op;
       op.fee = ASSET("0.000 TESTS");
       op.delegation = asset(100, VESTS_SYMBOL);
       op.creator = "alice";
       op.new_account_name = "bob";
       op.owner = authority(1, priv_key.get_public_key(), 1);
       op.active = authority(2, priv_key.get_public_key(), 2);
       op.memo_key = priv_key.get_public_key();
       op.json_metadata = "{\"foo\":\"bar\"}";

       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.operations.push_back(op);

       BOOST_TEST_MESSAGE("--- Test failure when no signatures");
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

       BOOST_TEST_MESSAGE("--- Test success with witness signature");
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
       tx.operations.clear();
       tx.signatures.clear();
       op.new_account_name = "sam";
       tx.operations.push_back(op);
       tx.sign(alice_private_key, db.get_chain_id());
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

       BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
       tx.signatures.clear();
       tx.sign(init_account_priv_key, db.get_chain_id());
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

       BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the creator's authority");
       tx.signatures.clear();
       tx.sign(init_account_priv_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

// BOOST_AUTO_TEST_CASE(account_create_with_delegation_apply)
// {
//    try
//    {
//        BOOST_TEST_MESSAGE("Testing: account_create_with_delegation_apply");
//        signed_transaction tx;
//        ACTORS((alice));
//        // 150 * fee = ( 5 * DEIP ) + SP
//        generate_blocks(1);
//        fund("alice", ASSET("1510.000 TESTS"));
//        vest("alice", ASSET("1000.000 TESTS"));

//        private_key_type priv_key = generate_private_key("temp_key");

//        generate_block();

//        db_plugin->debug_update([=](database& db) {
//            db.modify(db.get_witness_schedule_object(),
//                      [&](witness_schedule_object& w) { w.median_props.account_creation_fee = ASSET("1.000 TESTS"); });
//        });

//        generate_block();

//        BOOST_TEST_MESSAGE("--- Test failure when VESTS are powering down.");
//        withdraw_vesting_operation withdraw;
//        withdraw.account = "alice";
//        withdraw.vesting_shares = db.get_account("alice").vesting_shares;
//        account_create_with_delegation_operation op;
//        op.fee = ASSET("10.000 TESTS");
//        op.delegation = ASSET("100000000.000000 VESTS");
//        op.creator = "alice";
//        op.new_account_name = "bob";
//        op.owner = authority(1, priv_key.get_public_key(), 1);
//        op.active = authority(2, priv_key.get_public_key(), 2);
//        op.memo_key = priv_key.get_public_key();
//        op.json_metadata = "{\"foo\":\"bar\"}";
//        tx.operations.push_back(withdraw);
//        tx.operations.push_back(op);
//        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
//        tx.sign(alice_private_key, db.get_chain_id());
//        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::assert_exception);

//        BOOST_TEST_MESSAGE("--- Test success under normal conditions. ");
//        tx.clear();
//        tx.operations.push_back(op);
//        tx.sign(alice_private_key, db.get_chain_id());
//        db.push_transaction(tx, 0);

//        const account_object& bob_acc = db.get_account("bob");
//        const account_object& alice_acc = db.get_account("alice");
//        BOOST_REQUIRE(alice_acc.delegated_vesting_shares == ASSET("100000000.000000 VESTS"));
//        BOOST_REQUIRE(bob_acc.received_vesting_shares == ASSET("100000000.000000 VESTS"));
//        BOOST_REQUIRE(bob_acc.effective_vesting_shares()
//                      == bob_acc.vesting_shares - bob_acc.delegated_vesting_shares + bob_acc.received_vesting_shares);

//        BOOST_TEST_MESSAGE("--- Test delegator object integrety. ");
//        auto delegation
//            = db.find<vesting_delegation_object, by_delegation>(boost::make_tuple(op.creator, op.new_account_name));

//        BOOST_REQUIRE(delegation != nullptr);
//        BOOST_REQUIRE(delegation->delegator == op.creator);
//        BOOST_REQUIRE(delegation->delegatee == op.new_account_name);
//        BOOST_REQUIRE(delegation->vesting_shares == ASSET("100000000.000000 VESTS"));
//        BOOST_REQUIRE(delegation->min_delegation_time == db.head_block_time() + DEIP_CREATE_ACCOUNT_DELEGATION_TIME);
//        auto del_amt = delegation->vesting_shares;
//        auto exp_time = delegation->min_delegation_time;

//        generate_block();

//        BOOST_TEST_MESSAGE("--- Test success using only DEIP to reach target delegation.");

//        tx.clear();
//        op.fee = asset(db.get_witness_schedule_object().median_props.account_creation_fee.amount
//                           * DEIP_CREATE_ACCOUNT_WITH_DEIP_MODIFIER * DEIP_CREATE_ACCOUNT_DELEGATION_RATIO,
//                       DEIP_SYMBOL);
//        op.delegation = asset(0, VESTS_SYMBOL);
//        op.new_account_name = "sam";
//        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
//        tx.operations.push_back(op);
//        tx.sign(alice_private_key, db.get_chain_id());
//        db.push_transaction(tx, 0);

//        BOOST_TEST_MESSAGE("--- Test failure when insufficient funds to process transaction.");
//        tx.clear();
//        op.fee = ASSET("10.000 TESTS");
//        op.delegation = ASSET("0.000000 VESTS");
//        op.new_account_name = "pam";
//        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
//        tx.operations.push_back(op);
//        tx.sign(alice_private_key, db.get_chain_id());

//        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

//        BOOST_TEST_MESSAGE("--- Test failure when insufficient fee fo reach target delegation.");
//        fund("alice", asset(db.get_witness_schedule_object().median_props.account_creation_fee.amount
//                                * DEIP_CREATE_ACCOUNT_WITH_DEIP_MODIFIER * DEIP_CREATE_ACCOUNT_DELEGATION_RATIO,
//                            DEIP_SYMBOL));
//        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

//        validate_database();

//        BOOST_TEST_MESSAGE("--- Test removing delegation from new account");
//        tx.clear();
//        delegate_vesting_shares_operation delegate;
//        delegate.delegator = "alice";
//        delegate.delegatee = "bob";
//        delegate.vesting_shares = ASSET("0.000000 VESTS");
//        tx.operations.push_back(delegate);
//        tx.sign(alice_private_key, db.get_chain_id());
//        db.push_transaction(tx, 0);

//        auto itr = db.get_index<vesting_delegation_expiration_index, by_id>().begin();
//        auto end = db.get_index<vesting_delegation_expiration_index, by_id>().end();

//        BOOST_REQUIRE(itr != end);
//        BOOST_REQUIRE(itr->delegator == "alice");
//        BOOST_REQUIRE(itr->vesting_shares == del_amt);
//        BOOST_REQUIRE(itr->expiration == exp_time);
//        validate_database();
//    }
//    FC_LOG_AND_RETHROW()
// }

BOOST_AUTO_TEST_CASE(create_research_group_apply)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: create_research_group_apply");

       ACTORS_WITH_EXPERT_TOKENS((alice));
       generate_block();

       private_key_type priv_key = generate_private_key("alice");

       create_research_group_operation op;

       op.name = "test";
       op.creator = "alice";
       op.permlink = "group";
       op.description = "group";
       op.quorum_percent = 10;
       op.tokens_amount = 100;

       BOOST_TEST_MESSAGE("--- Test");
       signed_transaction tx;
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.operations.push_back(op);
       tx.sign(alice_private_key, db.get_chain_id());
       tx.validate();
       db.push_transaction(tx, 0);

       auto& research_group_service = db.obtain_service<dbs_research_group>();
       auto& research_group = research_group_service.get_research_group(0);

       BOOST_CHECK(research_group.name == "test");
       BOOST_CHECK(research_group.description == "group");
       BOOST_CHECK(research_group.permlink == "group");
       BOOST_CHECK(research_group.quorum_percent == 10);
       BOOST_CHECK(research_group.total_tokens_amount == 100);

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_research_group_join_request_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: create_research_group_join_request_apply");

        ACTORS_WITH_EXPERT_TOKENS((alice));

        generate_block();

        auto& research_group = research_group_create(1, "name", "permlink", "description", 200, 50, 300);

        private_key_type priv_key = generate_private_key("alice");

        create_research_group_join_request_operation op;

        op.owner = "alice";
        op.research_group_id = 1;
        op.motivation_letter = "letter";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& research_group_join_request = db.get<research_group_join_request_object>(0);

        BOOST_CHECK(research_group_join_request.id == 0);
        BOOST_CHECK(research_group_join_request.research_group_id == 1);
        BOOST_CHECK(research_group_join_request.account_name == "alice");
        BOOST_CHECK(research_group_join_request.motivation_letter == "letter");
        BOOST_CHECK(research_group_join_request.expiration_time == db.head_block_time() + DAYS_TO_SECONDS(14));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reject_research_group_join_request_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: reject_research_group_join_request_apply");

        ACTORS_WITH_EXPERT_TOKENS((alice));

        generate_block();

        auto& research_group = research_group_create(1, "name", "permlink", "description", 200, 50, 300);
        auto& research_group_invite = research_group_join_request_create(1, "alice", 1, "letter");

        private_key_type priv_key = generate_private_key("alice");

        reject_research_group_join_request_operation op;

        op.research_group_join_request_id = 1;
        op.owner = "alice";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        BOOST_CHECK_THROW((db.get<research_group_join_request_object, by_id>(1)), boost::exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_research_tokens_to_research_group_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: transfer_research_tokens_to_research_group_apply");

        ACTOR_WITH_EXPERT_TOKENS(alice);
        ACTOR(bob);

        generate_block();

        private_key_type priv_key = generate_private_key("alice");

        add_expertise_tokens_operation op;

        std::vector<std::pair<int64_t, uint32_t>> disciplines_to_add;
        disciplines_to_add.push_back(std::make_pair(1, 1000));
        disciplines_to_add.push_back(std::make_pair(5, 2000));
        disciplines_to_add.push_back(std::make_pair(7, 2500));

        op.owner = "alice";
        op.account_name = "bob";
        op.disciplines_to_add = disciplines_to_add;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        BOOST_CHECK((db.get<expert_token_object, by_account_and_discipline>(std::make_tuple("bob", 1))).amount == 1000);
        BOOST_CHECK((db.get<expert_token_object, by_account_and_discipline>(std::make_tuple("bob", 5))).amount == 2000);
        BOOST_CHECK((db.get<expert_token_object, by_account_and_discipline>(std::make_tuple("bob", 7))).amount == 2500);

    }
    FC_LOG_AND_RETHROW()

}

BOOST_AUTO_TEST_CASE(contribute_to_token_sale_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: contribute_to_token_sale_apply");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob));
        fund("alice", 1000);
        fund("bob", 5000);

        generate_block();

        private_key_type alice_priv_key = generate_private_key("alice");

        auto& research_token_sale = research_token_sale_create(0, 1, db.head_block_time() - 60 * 60 * 5, db.head_block_time() + 60 * 60 * 5, 200, 1000, 100, 400);
        auto& research_token_sale_contribution = research_token_sale_contribution_create(0, 0, "bob", 200, db.head_block_time());

        contribute_to_token_sale_operation op;

        op.research_token_sale_id = 0;
        op.owner = "alice";
        op.amount = 600;

        BOOST_TEST_MESSAGE("--- Test");
        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(alice_priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& alice_account = db.get_account("alice");

        BOOST_CHECK(alice_account.balance.amount == 800);

        auto& alice_research_token = db.get<research_token_object, by_account_name_and_research_id>(std::make_tuple("alice", 1));
        auto& bob_research_token = db.get<research_token_object, by_account_name_and_research_id>(std::make_tuple("bob", 1));

        BOOST_CHECK(alice_research_token.amount == 500);
        BOOST_CHECK(bob_research_token.amount == 500);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(add_expertise_tokens_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: add_expertise_tokens_apply");

        ACTORS_WITH_EXPERT_TOKENS((alice));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
