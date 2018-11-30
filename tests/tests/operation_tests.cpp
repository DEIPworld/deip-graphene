#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/protocol/exceptions.hpp>

#include <deip/chain/database/database.hpp>
#include <deip/chain/database/database_exceptions.hpp>
#include <deip/chain/hardfork.hpp>
#include <deip/chain/schema/deip_objects.hpp>

#include <deip/chain/util/reward.hpp>

#include <deip/witness/witness_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <deip/chain/schema/research_discipline_relation_object.hpp>
#include <deip/chain/schema/expert_token_object.hpp>
#include <deip/chain/schema/review_object.hpp>
#include <deip/chain/schema/vesting_balance_object.hpp>
#include <deip/chain/schema/grant_objects.hpp>
#include <deip/chain/schema/expertise_stats_object.hpp>
#include <deip/chain/schema/expertise_allocation_proposal_object.hpp>
#include <deip/chain/services/dbs_offer_research_tokens.hpp>

#include <deip/chain/services/dbs_research_token.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>


#define DROPOUT_COMPENSATION_IN_PERCENT 1500

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

BOOST_FIXTURE_TEST_SUITE(operation_tests, clean_database_fixture)

BOOST_AUTO_TEST_CASE(make_review_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: make_review_research_apply");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(john)(rachel));

        generate_block();

        BOOST_TEST_MESSAGE("--- Test normal review creation");
        auto& research = research_create(1, "test_research", "abstract", "permlink", 1, 10, 1500);
        db.create<research_content_object>([&](research_content_object& c) {
            c.id = 1;
            c.created_at = fc::time_point_sec(db.head_block_time() - 60 * 60 * 5);
            c.research_id = research.id;
            c.authors = { "alice", "bob" };
            c.content = "content";
            c.references = {};
            c.external_references = { "http://google.com" };
            c.type = research_content_type::milestone_data;
            c.activity_state = research_content_activity_state::active;
        });

        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& rdr) {
            rdr.discipline_id = 1;
            rdr.research_id = 1;
        });

        private_key_type priv_key = generate_private_key("john");

        make_review_operation op;

        std::vector<int64_t> references {1};
        op.author = "john";
        op.research_content_id = 1;
        op.content = "test";
        op.is_positive = true;
        op.weight =  DEIP_100_PERCENT;

        fc::uint128 total_expert_tokens_amount; // With Common Token
        auto it_pair = db.get_index<expert_token_index>().indicies().get<by_account_name>().equal_range("john");
        auto it = it_pair.first;
        const auto it_end = it_pair.second;
        while (it != it_end)
        {
            fc::uint128 amount(it->amount.value);
            total_expert_tokens_amount += amount;
            ++it;
        }

        auto& token = db.get<expert_token_object, by_account_and_discipline>(std::make_tuple("john", 1));
        auto old_voting_power = token.voting_power;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto new_voting_power = token.voting_power;
        const review_object& review = db.get<review_object, by_id>(0);

        std::vector<discipline_id_type> disciplines;
        for (auto discipline : review.disciplines)
            disciplines.push_back(discipline);

        BOOST_CHECK(review.research_content_id == 1);
        BOOST_CHECK(review.author == "john");
        BOOST_CHECK(review.is_positive == true);
        BOOST_CHECK(review.content == "test");
        BOOST_CHECK(review.expertise_amounts_used.at(1) == (old_voting_power * op.weight * token.amount) / (DEIP_100_PERCENT * DEIP_100_PERCENT));
        BOOST_CHECK(disciplines.size() == 1 && disciplines[0] == 1);
        BOOST_CHECK(old_voting_power - new_voting_power == (DEIP_REVIEW_REQUIRED_POWER_PERCENT * op.weight) / DEIP_100_PERCENT);

        validate_database();

        BOOST_TEST_MESSAGE("--- Test failing review creation");

        research_create(2, "test_research11", "abstract11", "permlink111", 1, 10, 1500);

        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& rdr) {
            rdr.discipline_id = 2;
            rdr.research_id = 2;
        });

        op.author = "john";
        op.research_content_id = 1;
        op.content = "test";
        op.is_positive = true;

        tx.operations.clear();
        tx.signatures.clear();

        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        BOOST_CHECK_THROW(db.push_transaction(tx, 0), fc::assert_exception);

    }
    FC_LOG_AND_RETHROW()
}

//BOOST_AUTO_TEST_CASE(vote_apply_failure)
//{
//
//    BOOST_TEST_MESSAGE("Testing: vote_apply failure cases");
//
//    ACTORS_WITH_EXPERT_TOKENS((alice)(bob));
//
//    generate_block();
//
//    auto& research = research_create(1, "test_research", "abstract", "permlink", 1, 10, 1500);
//    auto& discipline_service = db.obtain_service<dbs_discipline>();
//    auto& discipline = discipline_service.get_discipline(1);
//
//    db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
//        r.discipline_id = discipline.id;
//        r.research_id = research.id;
//        r.votes_count = 0;
//    });
//
//    auto& content = db.create<research_content_object>([&](research_content_object& c) {
//        c.id = 1;
//        c.created_at = fc::time_point_sec(db.head_block_time() - 60 * 60 * 5);
//        c.research_id = research.id;
//        c.authors = { "alice", "bob" };
//        c.content = "content";
//        c.references = {};
//        c.external_references = { "http://google.com" };
//        c.type = research_content_type::milestone;
//    });
//
//    private_key_type priv_key = generate_private_key("alice");
//
//    vote_operation op;
//
//    signed_transaction tx;
//    tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
//
//    BOOST_TEST_MESSAGE("--- Testing voting on a non-existent research");
//
//    tx.operations.clear();
//    tx.signatures.clear();
//
//    op.research_id = 100;
//    op.research_content_id = content.id._id;
//    op.discipline_id = discipline.id._id;
//    op.weight = 50 * DEIP_1_PERCENT;
//    op.voter = "alice";
//
//    tx.operations.push_back(op);
//    tx.sign(priv_key, db.get_chain_id());
//    tx.validate();
//
//    DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
//
//    validate_database();
//
//    BOOST_TEST_MESSAGE("--- Testing voting on a non-existent content");
//
//    tx.operations.clear();
//    tx.signatures.clear();
//
//    op.research_id = research.id._id;
//    op.research_content_id = 100;
//    op.discipline_id = discipline.id._id;
//    op.weight = 50 * DEIP_1_PERCENT;
//    op.voter = "alice";
//
//    tx.operations.push_back(op);
//    tx.sign(priv_key, db.get_chain_id());
//    tx.validate();
//
//    DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
//
//    validate_database();
//
//    BOOST_TEST_MESSAGE("--- Testing voting with non-existent discipline");
//
//    tx.operations.clear();
//    tx.signatures.clear();
//
//    op.research_id = research.id._id;
//    op.research_content_id = content.id._id;
//    op.discipline_id = 100;
//    op.weight = 50 * DEIP_1_PERCENT;
//    op.voter = "alice";
//
//    tx.operations.push_back(op);
//    tx.sign(priv_key, db.get_chain_id());
//    tx.validate();
//
//    DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
//
//    validate_database();
//
//    BOOST_TEST_MESSAGE("--- Testing voting with a weight of 0");
//
//    tx.operations.clear();
//    tx.signatures.clear();
//
//    op.research_id = research.id._id;
//    op.research_content_id = content.id._id;
//    op.discipline_id = discipline.id._id;
//    op.weight = 0;
//    op.voter = "alice";
//
//    tx.operations.push_back(op);
//    tx.sign(alice_private_key, db.get_chain_id());
//
//    DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
//
//    validate_database();
//}
//
//BOOST_AUTO_TEST_CASE(vote_apply_success)
//{
//    BOOST_TEST_MESSAGE("Testing: vote_apply success cases");
//
//    ACTORS_WITH_EXPERT_TOKENS((alice)(bob));
//
//    generate_block();
//
//    auto& research = research_create(1, "test_research", "abstract", "permlink", 1, 10, 1500);
//    auto& discipline_service = db.obtain_service<dbs_discipline>();
//    auto& discipline = discipline_service.get_discipline(1);
//
//    db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
//        r.discipline_id = discipline.id;
//        r.research_id = research.id;
//        r.votes_count = 0;
//    });
//
//    auto& expert_token_service = db.obtain_service<dbs_expert_token>();
//    auto& token = expert_token_service.get_expert_token_by_account_and_discipline("alice", 1);
//
//    auto& content = db.create<research_content_object>([&](research_content_object& c) {
//        c.id = 1;
//        c.created_at = fc::time_point_sec(db.head_block_time() - 60 * 60 * 5);
//        c.research_id = research.id;
//        c.authors = { "alice", "bob" };
//        c.content = "content";
//        c.references = {};
//        c.external_references = { "http://google.com" };
//        c.type = research_content_type::milestone;
//        c.activity_state = research_content_activity_state::active;
//    });
//
//    private_key_type priv_key = generate_private_key("alice");
//
//    vote_operation op;
//
//    signed_transaction tx;
//    tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
//
//    BOOST_TEST_MESSAGE("--- Testing success");
//
//    tx.operations.clear();
//    tx.signatures.clear();
//
//    auto old_voting_power = token.voting_power;
//
//    op.research_id = research.id._id;
//    op.research_content_id = content.id._id;
//    op.discipline_id = discipline.id._id;
//    op.weight = 50 * DEIP_1_PERCENT;
//    op.voter = "alice";
//
//    tx.operations.clear();
//    tx.signatures.clear();
//    tx.operations.push_back(op);
//    tx.sign(alice_private_key, db.get_chain_id());
//
//    db.push_transaction(tx, 0);
//
//    // Validate token
//    BOOST_REQUIRE(token.voting_power == old_voting_power - (old_voting_power * op.weight / DEIP_100_PERCENT / 10));
//    BOOST_REQUIRE(token.last_vote_time == db.head_block_time());
//
//    // Validate vote & total_votes objects
//    auto& vote_service = db.obtain_service<dbs_vote>();
//    auto& total_votes = vote_service.get_total_votes_by_content_and_discipline(op.research_content_id, op.discipline_id);
//
//    const auto& vote_idx = db._temporary_public_impl().get_index<vote_index>().indices().get<by_voter_discipline_and_content>();
//    auto itr = vote_idx.find(std::make_tuple(op.voter, op.discipline_id, op.research_content_id));
//    auto research_reward_curve = curve_id::linear;
//    auto curator_reward_curve = curve_id::linear;
//
//    // vote
//    BOOST_REQUIRE(itr != vote_idx.end());
//    auto& vote = *itr;
//    BOOST_REQUIRE(vote.voting_power == (old_voting_power * op.weight / DEIP_100_PERCENT));
//    int64_t expected_tokens_amount = (token.amount.value * old_voting_power * op.weight) / (DEIP_100_PERCENT * DEIP_100_PERCENT);
//    BOOST_REQUIRE(vote.tokens_amount.value == expected_tokens_amount);
//    BOOST_REQUIRE(vote.voting_time == db.head_block_time());
//    BOOST_REQUIRE(vote.voter == op.voter);
//    BOOST_REQUIRE(vote.discipline_id == op.discipline_id);
//    BOOST_REQUIRE(vote.research_id == op.research_id);
//    BOOST_REQUIRE(vote.research_content_id == op.research_content_id);
//
//    // Calculate vote weight
//    int64_t expected_curator_reward_weight = util::evaluate_reward_curve(expected_tokens_amount, curator_reward_curve).to_uint64();
//    /// discount weight by time
//    uint128_t w(expected_curator_reward_weight);
//    uint64_t delta_t = std::min(uint64_t((vote.voting_time - content.created_at).to_seconds()),
//                                uint64_t(DEIP_REVERSE_AUCTION_WINDOW_SECONDS));
//
//    w *= delta_t;
//    w /= DEIP_REVERSE_AUCTION_WINDOW_SECONDS;
//    expected_curator_reward_weight = w.to_uint64();
//    BOOST_REQUIRE(vote.weight == expected_curator_reward_weight);
//
//    // total_votes
//    BOOST_REQUIRE(total_votes.total_weight == expected_tokens_amount);
//
//    uint64_t expected_research_reward_weight = util::evaluate_reward_curve(expected_tokens_amount, research_reward_curve).to_uint64();
//    BOOST_REQUIRE(total_votes.total_weight == expected_research_reward_weight);
//    BOOST_REQUIRE(total_votes.total_weight == expected_curator_reward_weight);
//
//    // Validate discipline
//
//    BOOST_REQUIRE(discipline.total_active_weight == expected_tokens_amount);
//
//    // Validate global properties object
//    auto& dgpo = db.get_dynamic_global_properties();
//    BOOST_REQUIRE(dgpo.total_disciplines_weight == expected_tokens_amount);
//
//    validate_database();
//}

BOOST_AUTO_TEST_CASE(vote_for_review_apply_success)
{
    BOOST_TEST_MESSAGE("Testing: vote_for_review_apply success cases");

    ACTORS_WITH_EXPERT_TOKENS((alice)(bob));

    generate_block();

    auto& research = research_create(1, "test_research", "abstract", "permlink", 1, 10, 1500);
    auto& content = db.create<research_content_object>([&](research_content_object& rc) {
        rc.id = 1;
        rc.research_id = research.id;
        rc.content = "content";
        rc.type = research_content_type::milestone_data;
    });

    dbs_discipline& discipline_service = db.obtain_service<dbs_discipline>();
    auto& discipline = discipline_service.get_discipline(1);

    db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
        r.discipline_id = discipline.id;
        r.research_id = research.id;
        r.votes_count = 0;
    });

    auto& review = db.create<review_object>([&](review_object& r) {
        r.id = 1;
        r.content = "review";
        r.is_positive = true;
        r.author = "bob";
        r.research_content_id = content.id;
        r.created_at = db.head_block_time();
        r.expertise_amounts_used[discipline.id] = 1000;
        r.disciplines.insert(discipline.id);
    });

    db.create<total_votes_object>([&](total_votes_object& tv) {
        tv.total_weight = 0;
        tv.research_id = research.id;
        tv.research_content_id = content.id;
        tv.discipline_id = discipline.id;
    });

    private_key_type priv_key = generate_private_key("alice");

    vote_for_review_operation op;

    signed_transaction tx;
    tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);

    BOOST_TEST_MESSAGE("--- Testing success");

    tx.operations.clear();
    tx.signatures.clear();

    dbs_expert_token& expert_token_service = db.obtain_service<dbs_expert_token>();
    auto& token = expert_token_service.get_expert_token_by_account_and_discipline("alice", discipline.id);
    auto old_voting_power = token.voting_power;

    op.review_id = review.id._id;
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

    // Validate vote
    const auto& vote_idx = db._temporary_public_impl().get_index<review_vote_index>().indices().get<by_voter_discipline_and_review>();
    auto itr = vote_idx.find(std::make_tuple(op.voter, op.discipline_id, op.review_id));

    // vote
    BOOST_REQUIRE(itr != vote_idx.end());
    auto& vote = *itr;
    int64_t expected_tokens_amount = (token.amount.value * old_voting_power * abs(op.weight)) / (DEIP_100_PERCENT * DEIP_100_PERCENT);
    BOOST_REQUIRE(vote.voting_time == db.head_block_time());
    BOOST_REQUIRE(vote.voter == op.voter);
    BOOST_REQUIRE(vote.discipline_id == op.discipline_id);
    BOOST_REQUIRE(vote.review_id == op.review_id);

    // Calculate vote weight
    int64_t expected_weight = expected_tokens_amount;
    /// discount weight by time
    uint128_t w(expected_tokens_amount);
    uint64_t delta_t = std::min(uint64_t((vote.voting_time - review.created_at).to_seconds()),
                                uint64_t(DEIP_REVERSE_AUCTION_WINDOW_SECONDS));

    w *= delta_t;
    w /= DEIP_REVERSE_AUCTION_WINDOW_SECONDS;
    expected_weight = w.to_uint64();
    BOOST_REQUIRE(vote.weight == expected_weight);

    auto& updated_review = db.get<review_object>(review.id);

    BOOST_REQUIRE(updated_review.weights_per_discipline.at(vote.discipline_id) == expected_weight);

    // Validate discipline
    BOOST_REQUIRE(discipline.total_active_weight == 1000);

    // Validate review
    auto weight_modifier = db.calculate_review_weight_modifier(updated_review.id, discipline.id);
    BOOST_REQUIRE(updated_review.weight_modifiers.at(discipline.id) == weight_modifier);

    validate_database();
}

BOOST_AUTO_TEST_CASE(approve_research_group_invite_apply)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob));

        generate_block();

        vector<account_name_type> accounts = { "alice" };

        private_key_type bob_priv_key = generate_private_key("bob");
        private_key_type alice_priv_key = generate_private_key("alice");

           //////////////////////////////////////////////////
          /// Сreate two research groups and invite Bob  ///
         ///                                            ///
        //////////////////////////////////////////////////

        std::map<uint16_t, uint32_t> proposal_quorums;

        for (int i = 1; i <= 11; i++)
            proposal_quorums.insert(std::make_pair(i, 5000));

        research_group_create_by_operation("alice", "name rg1", "permlink rg1", "description rg1", DEIP_100_PERCENT, proposal_quorums, false);
        research_group_create_by_operation("alice", "name rg2", "permlink rg2", "description rg2", DEIP_100_PERCENT, proposal_quorums, false);

        research_group_invite_create(0, "bob", 0, 5000);
        research_group_invite_create(1, "bob", 1, 5000);

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

        BOOST_CHECK(_research_group_1_token.owner == "bob" && _research_group_2_token.owner == "bob");
        BOOST_CHECK(_research_group_1_token.research_group_id == 0 && _research_group_2_token.research_group_id == 1);
        BOOST_CHECK(_research_group_1_token.amount == 5000 && _research_group_2_token.amount == 5000);

        BOOST_CHECK_THROW((db.get<research_group_invite_object, by_id>(0)), boost::exception);
        BOOST_CHECK_THROW((db.get<research_group_invite_object, by_id>(1)), boost::exception);

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

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 50));

        research_group_create(31, "name", "permlink", "description", 200, proposal_quorums, false);
        research_group_invite_create(1, "bob", 31, 5000);

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

        std::map<uint16_t, uint32_t> proposal_quorums;

        for (int i = 1; i <= 11; i++)
            proposal_quorums.insert(std::make_pair(i, 5000));

           //////////////////////////////////////////////////
          /// Сreate two research groups and invite Bob  ///
         ///                                            ///
        //////////////////////////////////////////////////

        research_group_create_by_operation("alice", "name rg1", "permlink rg1", "description rg1", DEIP_100_PERCENT, proposal_quorums, false);
        research_group_create_by_operation("alice", "name rg2", "permlink rg2", "description rg2", DEIP_100_PERCENT, proposal_quorums, false);

        research_group_invite_create(0, "bob", 0, 10000);
        research_group_invite_create(1, "bob", 1, 10000);

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

       op.fee = asset(30000, DEIP_SYMBOL);
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
       BOOST_REQUIRE(acct.common_tokens_balance == op.fee.amount.value);
       BOOST_REQUIRE(acct.common_tokens_withdraw_rate == 0);
       BOOST_REQUIRE(acct.proxied_vsf_votes_total() == 0);
       BOOST_REQUIRE((init_starting_balance - ASSET("30.000 TESTS")).amount.value == init.balance.amount.value);
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
       BOOST_REQUIRE(acct.common_tokens_balance == op.fee.amount.value);
       BOOST_REQUIRE(acct.common_tokens_withdraw_rate == 0);
       BOOST_REQUIRE(acct.proxied_vsf_votes_total().value == 0);
       BOOST_REQUIRE((init_starting_balance - ASSET("30.000 TESTS")).amount.value == init.balance.amount.value);
       validate_database();

       BOOST_TEST_MESSAGE("--- Test failure when creator cannot cover fee");
       tx.signatures.clear();
       tx.operations.clear();
       op.fee = asset(0, DEIP_SYMBOL);
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
       op.fee = ASSET("0.000 TESTS");
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
BOOST_AUTO_TEST_CASE(transfer_to_common_tokens_validate)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: transfer_to_common_tokens_validate");

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_to_common_tokens_authorities)
{
   try
   {
       ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
       fund("alice", 10000);

       BOOST_TEST_MESSAGE("Testing: transfer_to_common_tokens_authorities");

       transfer_to_common_tokens_operation op;
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

BOOST_AUTO_TEST_CASE(transfer_to_common_tokens_apply)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: transfer_to_common_tokens_apply");

       ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
       fund("alice", 10000);

       const auto& new_alice = db.get_account("alice");
       const auto& new_bob = db.get_account("bob");

       const auto& gpo = db.get_dynamic_global_properties();

       BOOST_REQUIRE(new_alice.balance == ASSET("10.000 TESTS"));

       share_type shares = gpo.total_common_tokens_amount;
       share_type alice_common_tokens = new_alice.common_tokens_balance;
       share_type bob_common_tokens = new_bob.common_tokens_balance;

       transfer_to_common_tokens_operation op;
       op.from = "alice";
       op.to = "";
       op.amount = ASSET("7.500 TESTS");

       signed_transaction tx;
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       share_type new_vest = share_type(op.amount.amount);
       shares += new_vest;
       alice_common_tokens += new_vest;

       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("2.500 TESTS").amount.value);
       BOOST_REQUIRE(new_alice.common_tokens_balance == alice_common_tokens);
       BOOST_REQUIRE(gpo.total_common_tokens_amount == shares);
       validate_database();

       op.to = "bob";
       op.amount = asset(2000, DEIP_SYMBOL);
       tx.operations.clear();
       tx.signatures.clear();
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       new_vest = op.amount.amount;
       shares += new_vest;
       bob_common_tokens += new_vest;

       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("0.500 TESTS").amount.value);
       BOOST_REQUIRE(new_alice.common_tokens_balance == alice_common_tokens);
       BOOST_REQUIRE(new_bob.balance.amount.value == ASSET("0.000 TESTS").amount.value);
       BOOST_REQUIRE(new_bob.common_tokens_balance == bob_common_tokens);
       BOOST_REQUIRE(gpo.total_common_tokens_amount == shares);
       validate_database();

       DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);

       BOOST_REQUIRE(new_alice.balance.amount.value == ASSET("0.500 TESTS").amount.value);
       BOOST_REQUIRE(new_alice.common_tokens_balance == alice_common_tokens);
       BOOST_REQUIRE(new_bob.balance.amount.value == ASSET("0.000 TESTS").amount.value);
       BOOST_REQUIRE(new_bob.common_tokens_balance == bob_common_tokens);
       BOOST_REQUIRE(gpo.total_common_tokens_amount == shares);
       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(withdraw_common_tokens_validate)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: withdraw_common_tokens_validate");

       validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(withdraw_common_tokens_authorities)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: withdraw_common_tokens_authorities");

       ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
       fund("alice", 10000);

       withdraw_common_tokens_operation op;
       op.account = "alice";
       op.total_common_tokens_amount = 1000;

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

BOOST_AUTO_TEST_CASE(withdraw_common_tokens_apply)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: withdraw_common_tokens_apply");

       ACTORS_WITH_EXPERT_TOKENS((alice))
       generate_block();

       BOOST_TEST_MESSAGE("--- Test withdraw of existing VESTS");

       {
           const auto& alice = db.get_account("alice");

           withdraw_common_tokens_operation op;
           op.account = "alice";
           op.total_common_tokens_amount = alice.common_tokens_balance / 2;

           share_type old_common_tokens = alice.common_tokens_balance;

           signed_transaction tx;
           tx.operations.push_back(op);
           tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
           tx.sign(alice_private_key, db.get_chain_id());
           db.push_transaction(tx, 0);

           BOOST_REQUIRE(alice.common_tokens_balance == old_common_tokens);
           BOOST_REQUIRE(alice.common_tokens_withdraw_rate.value
                         == (old_common_tokens / (DEIP_COMMON_TOKENS_WITHDRAW_INTERVALS * 2)).value);
           BOOST_REQUIRE(alice.to_withdraw.value == op.total_common_tokens_amount.value);
           BOOST_REQUIRE(alice.next_common_tokens_withdrawal
                         == db.head_block_time() + DEIP_COMMON_TOKENS_WITHDRAW_INTERVAL_SECONDS);
           validate_database();

           BOOST_TEST_MESSAGE("--- Test changing vesting withdrawal");
           tx.operations.clear();
           tx.signatures.clear();

           op.total_common_tokens_amount = alice.common_tokens_balance / 3;
           tx.operations.push_back(op);
           tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
           tx.sign(alice_private_key, db.get_chain_id());
           db.push_transaction(tx, 0);

           BOOST_REQUIRE(alice.common_tokens_balance == old_common_tokens.value);
           BOOST_REQUIRE(alice.common_tokens_withdraw_rate.value
                         == (old_common_tokens / (DEIP_COMMON_TOKENS_WITHDRAW_INTERVALS * 3)).value);
           BOOST_REQUIRE(alice.to_withdraw.value == op.total_common_tokens_amount.value);
           BOOST_REQUIRE(alice.next_common_tokens_withdrawal
                         == db.head_block_time() + DEIP_COMMON_TOKENS_WITHDRAW_INTERVAL_SECONDS);
           validate_database();

           BOOST_TEST_MESSAGE("--- Test withdrawing more vests than available");

           tx.operations.clear();
           tx.signatures.clear();

           op.total_common_tokens_amount = alice.common_tokens_balance * 2;
           tx.operations.push_back(op);
           tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
           tx.sign(alice_private_key, db.get_chain_id());
           DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

           BOOST_REQUIRE(alice.common_tokens_balance.value == old_common_tokens.value);
           BOOST_REQUIRE(alice.common_tokens_withdraw_rate.value
                         == (old_common_tokens / (DEIP_COMMON_TOKENS_WITHDRAW_INTERVALS * 3)).value);
           BOOST_REQUIRE(alice.next_common_tokens_withdrawal
                         == db.head_block_time() + DEIP_COMMON_TOKENS_WITHDRAW_INTERVAL_SECONDS);
           validate_database();

           BOOST_TEST_MESSAGE("--- Test withdrawing 0 to reset vesting withdraw");
           tx.operations.clear();
           tx.signatures.clear();

           op.total_common_tokens_amount = 0;
           tx.operations.push_back(op);
           tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
           tx.sign(alice_private_key, db.get_chain_id());
           db.push_transaction(tx, 0);

           BOOST_REQUIRE(alice.common_tokens_balance == old_common_tokens.value);
           BOOST_REQUIRE(alice.common_tokens_withdraw_rate.value == 0);
           BOOST_REQUIRE(alice.to_withdraw.value == 0);
           BOOST_REQUIRE(alice.next_common_tokens_withdrawal == fc::time_point_sec::maximum());

           BOOST_TEST_MESSAGE("--- Test cancelling a withdraw when below the account creation fee");
           op.total_common_tokens_amount = alice.common_tokens_balance;
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
                           += wso.median_props.account_creation_fee - ASSET("0.001 TESTS") - gpo.common_tokens_fund;
                   gpo.common_tokens_fund = wso.median_props.account_creation_fee - ASSET("0.001 TESTS");

               });
           },
           database::skip_witness_signature);

       withdraw_common_tokens_operation op;
       signed_transaction tx;
       op.account = "alice";
       op.total_common_tokens_amount = 0;
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       BOOST_REQUIRE(db.get_account("alice").common_tokens_withdraw_rate == 0);
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

       account_create_operation acc_create;
       acc_create.fee = ASSET("30.000 TESTS");
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
       fund("alice", ASSET("10.000 TESTS"));

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
// BOOST_AUTO_TEST_CASE(account_create_with_delegation_authorities)
// {
//    try
//    {
//        BOOST_TEST_MESSAGE("Testing: account_create_with_delegation_authorities");

//        signed_transaction tx;
//        ACTORS_WITH_EXPERT_TOKENS((alice));
//        generate_blocks(1);
//        fund("alice", ASSET("1000.000 TESTS"));
//        vest("alice", ASSET("10000.000000 VESTS"));

//        private_key_type priv_key = generate_private_key("temp_key");

//        account_create_with_delegation_operation op;
//        op.fee = ASSET("0.000 TESTS");
//        op.delegation = asset(100, VESTS_SYMBOL);
//        op.creator = "alice";
//        op.new_account_name = "bob";
//        op.owner = authority(1, priv_key.get_public_key(), 1);
//        op.active = authority(2, priv_key.get_public_key(), 2);
//        op.memo_key = priv_key.get_public_key();
//        op.json_metadata = "{\"foo\":\"bar\"}";

//        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
//        tx.operations.push_back(op);

//        BOOST_TEST_MESSAGE("--- Test failure when no signatures");
//        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

//        BOOST_TEST_MESSAGE("--- Test success with witness signature");
//        tx.sign(alice_private_key, db.get_chain_id());
//        db.push_transaction(tx, 0);

//        BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
//        tx.operations.clear();
//        tx.signatures.clear();
//        op.new_account_name = "sam";
//        tx.operations.push_back(op);
//        tx.sign(alice_private_key, db.get_chain_id());
//        tx.sign(alice_private_key, db.get_chain_id());
//        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

//        BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
//        tx.signatures.clear();
//        tx.sign(init_account_priv_key, db.get_chain_id());
//        tx.sign(alice_private_key, db.get_chain_id());
//        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

//        BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the creator's authority");
//        tx.signatures.clear();
//        tx.sign(init_account_priv_key, db.get_chain_id());
//        DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

//        validate_database();
//    }
//    FC_LOG_AND_RETHROW()
// }

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
//        withdraw_common_tokens_operation withdraw;
//        withdraw.account = "alice";
//        withdraw.common_tokens_balance = db.get_account("alice").common_tokens_balance;
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

       std::map<uint16_t, uint32_t> proposal_quorums;

       for (int i = 1; i <= 11; i++)
           proposal_quorums.insert(std::make_pair(i, 1000));

       create_research_group_operation op;

       op.name = "test";
       op.creator = "alice";
       op.permlink = "group";
       op.description = "group";
       op.quorum_percent = DEIP_100_PERCENT;
       op.proposal_quorums = proposal_quorums;

       BOOST_TEST_MESSAGE("--- Test");
       signed_transaction tx;
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.operations.push_back(op);
       tx.sign(alice_private_key, db.get_chain_id());
       tx.validate();
       db.push_transaction(tx, 0);

       auto& research_group_service = db.obtain_service<dbs_research_group>();
       auto& research_group = research_group_service.get_research_group_by_permlink("group");

       BOOST_CHECK(research_group.name == "test");
       BOOST_CHECK(research_group.description == "group");
       BOOST_CHECK(research_group.permlink == "group");
       BOOST_CHECK(research_group.quorum_percent == DEIP_100_PERCENT);
       BOOST_CHECK(research_group.proposal_quorums.size() == 11);

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_research_group_with_invitees_apply)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: create_research_group_apply");

       ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(sam)(corp));
       generate_block();

       private_key_type priv_key = generate_private_key("alice");

       std::map<uint16_t, uint32_t> proposal_quorums;

       for (int i = 1; i <= 11; i++)
           proposal_quorums.insert(std::make_pair(i, 1000));

       create_research_group_operation op;

       op.name = "test";
       op.creator = "alice";
       op.permlink = "group";
       op.description = "group";
       op.quorum_percent = DEIP_100_PERCENT;
       op.proposal_quorums = proposal_quorums;
       op.invitees.push_back(invitee_type("bob", 1000, "good"));
       op.invitees.push_back(invitee_type("sam", 1000, "best"));
       op.invitees.push_back(invitee_type("corp", 6000, "bad"));

       BOOST_TEST_MESSAGE("--- Test");
       signed_transaction tx;
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.operations.push_back(op);
       tx.sign(alice_private_key, db.get_chain_id());
       tx.validate();
       db.push_transaction(tx, 0);

       auto& research_group_service = db.obtain_service<dbs_research_group>();
       auto& research_group = research_group_service.get_research_group_by_permlink("group");

       BOOST_CHECK(research_group.name == "test");
       BOOST_CHECK(research_group.description == "group");
       BOOST_CHECK(research_group.permlink == "group");
       BOOST_CHECK(research_group.proposal_quorums.size() == 11);

       auto& research_group_invite_service = db.obtain_service<dbs_research_group_invite>();
       auto& rg_invite_bob = research_group_invite_service.get_research_group_invite_by_account_name_and_research_group_id("bob", 25);
       auto& rg_invite_sam = research_group_invite_service.get_research_group_invite_by_account_name_and_research_group_id("sam", 25);
       auto& rg_invite_corp = research_group_invite_service.get_research_group_invite_by_account_name_and_research_group_id("corp", 25);

       BOOST_CHECK(rg_invite_bob.account_name == "bob");
       BOOST_CHECK(rg_invite_bob.research_group_id == 25);
       BOOST_CHECK(rg_invite_bob.research_group_token_amount == 1000);
       BOOST_CHECK(rg_invite_bob.cover_letter == "good");
       BOOST_CHECK(rg_invite_sam.account_name == "sam");
       BOOST_CHECK(rg_invite_sam.research_group_id == 25);
       BOOST_CHECK(rg_invite_sam.research_group_token_amount == 1000);
       BOOST_CHECK(rg_invite_sam.cover_letter == "best");
       BOOST_CHECK(rg_invite_corp.account_name == "corp");
       BOOST_CHECK(rg_invite_corp.research_group_id == 25);
       BOOST_CHECK(rg_invite_corp.research_group_token_amount == 6000);
       BOOST_CHECK(rg_invite_corp.cover_letter == "bad");
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

        research_token_create(1, "alice", 5000, 1);
        auto& research = research_create(1, "title", "abstract", "permlink", 1, 1, 1);

        private_key_type priv_key = generate_private_key("alice");

        db.modify(research, [&](research_object& r_o){
            r_o.owned_tokens = 50 * DEIP_1_PERCENT;
        });

        transfer_research_tokens_to_research_group_operation op;

        op.research_id = 1;
        op.amount = 50 * DEIP_1_PERCENT;
        op.owner = "alice";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        BOOST_CHECK(research.owned_tokens == DEIP_100_PERCENT);
        BOOST_CHECK_THROW(db.get<research_token_object>(1), std::out_of_range);
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

        research_token_sale_create(0, 1, db.head_block_time() - 60 * 60 * 5, db.head_block_time() + 60 * 60 * 5, 200, 1000, 100, 400);
        research_token_sale_contribution_create(0, 0, "bob", 200, db.head_block_time());

        contribute_to_token_sale_operation op;

        op.research_token_sale_id = 0;
        op.owner = "alice";
        op.amount = asset(600, DEIP_SYMBOL);

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

BOOST_AUTO_TEST_CASE(set_expertise_tokens_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: set_expertise_tokens_apply");

        ACTOR_WITH_EXPERT_TOKENS(alice);
        ACTOR(bob);

        generate_block();

        private_key_type priv_key = generate_private_key("alice");

        set_expertise_tokens_operation op;

        std::vector<expertise_amount_pair_type> disciplines_to_add;
        disciplines_to_add.push_back(expertise_amount_pair_type(1, 1000));
        disciplines_to_add.push_back(expertise_amount_pair_type(5, 2000));
        disciplines_to_add.push_back(expertise_amount_pair_type(7, 2500));

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

BOOST_AUTO_TEST_CASE(research_update_apply)
{
    try {
        BOOST_TEST_MESSAGE("Testing: set_expertise_tokens_apply");

        ACTOR_WITH_EXPERT_TOKENS(alice);

        generate_block();

        private_key_type priv_key = generate_private_key("alice");

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 100));

        auto& research = research_create(0, "title", "abstract", "permlink", 31, 10, 10);
        research_group_create(31, "name", "permlink", "description", 100, proposal_quorums, false);
        research_group_token_create(31, "alice", DEIP_100_PERCENT);

        research_update_operation op;

        op.research_id = 0;
        op.title = "new_title";
        op.abstract = "new_abstract";
        op.permlink = "new_permlink";
        op.owner = "alice";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        BOOST_CHECK(research.title == "new_title");
        BOOST_CHECK(research.abstract == "new_abstract");
        BOOST_CHECK(research.permlink == "new_permlink");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(deposit_to_vesting_balance_apply)
{
    try {
        BOOST_TEST_MESSAGE("Testing: deposit_to_vesting_balance_apply");

        ACTORS((alice)(bob));

        generate_block();

        fund("alice", asset(10000, DEIP_SYMBOL));

        private_key_type priv_key = generate_private_key("alice");

        create_vesting_balance_operation op;

        op.creator = "alice";
        op.owner = "bob";
        op.balance = asset(1000, DEIP_SYMBOL);
        op.vesting_duration_seconds = DAYS_TO_SECONDS(365);
        op.vesting_cliff_seconds = 0;
        op.period_duration_seconds = DAYS_TO_SECONDS(5);

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& vesting_balance = db.get<vesting_balance_object, by_id>(0);

        BOOST_CHECK(vesting_balance.owner == "bob");
        BOOST_CHECK(vesting_balance.balance.amount == 1000);
        BOOST_CHECK(vesting_balance.vesting_duration_seconds == DAYS_TO_SECONDS(365));
        BOOST_CHECK(vesting_balance.start_timestamp == db.head_block_time());
        BOOST_CHECK(vesting_balance.period_duration_seconds == DAYS_TO_SECONDS(5));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(withdraw_from_vesting_balance_apply)
{
    try {
        BOOST_TEST_MESSAGE("Testing: withdraw_from_vesting_balance_apply");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob));

        generate_block();

        private_key_type priv_key = generate_private_key("bob");

        auto& contract = db.create<vesting_balance_object>([&](vesting_balance_object& v) {
            v.id = 1;
            v.owner = "bob";
            v.balance = asset(1000, DEIP_SYMBOL);
            v.start_timestamp = fc::time_point_sec(db.head_block_time() - DAYS_TO_SECONDS(155));
            v.vesting_duration_seconds = DAYS_TO_SECONDS(300);
            v.period_duration_seconds = DAYS_TO_SECONDS(10);
            v.vesting_cliff_seconds = 0;
        });

        withdraw_vesting_balance_operation op;

        op.vesting_balance_id = 1;
        op.owner = "bob";
        op.amount = asset(500, DEIP_SYMBOL);

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        BOOST_CHECK(contract.owner == "bob");
        BOOST_CHECK(contract.balance.amount == 500);
        BOOST_CHECK(contract.withdrawn.amount == 500);

        auto bob_acc = db.get_account("bob");

        BOOST_CHECK(bob_acc.balance.amount == 500);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(invite_member_execute_test)
{
    try {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
        std::vector<std::pair<account_name_type, share_type>> accounts = {std::make_pair("alice", 10000)};
        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 0));

        setup_research_group(31, "name", "research_group", "research group", 0, proposal_quorums, false, accounts);
        const std::string json_str = "{\"name\":\"bob\",\"research_group_id\":31,\"research_group_token_amount_in_percent\":5000}";
        create_proposal(1, dbs_proposal::action_t::invite_member, json_str, "alice", 31, fc::time_point_sec(0xffffffff),
                        1);


        auto &research_group_invite_service = db.obtain_service<dbs_research_group_invite>();

        vote_proposal_operation op;
        op.research_group_id = 31;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto &research_group_invite = research_group_invite_service.get_research_group_invite_by_account_name_and_research_group_id(
                "bob", 31);

        BOOST_CHECK(research_group_invite.account_name == "bob");
        BOOST_CHECK(research_group_invite.research_group_id == 31);
        BOOST_CHECK(research_group_invite.research_group_token_amount == 50 * DEIP_1_PERCENT);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(exclude_member_test)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob));

        auto& research_group_service = db.obtain_service<dbs_research_group>();
        vector<std::pair<account_name_type, share_type>> accounts = { std::make_pair("alice", 9000), std::make_pair("bob", 1000) };

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 40));

        setup_research_group(31, "name", "research_group", "research group", 0, proposal_quorums, false, accounts);

        const std::string exclude_member_json = "{\"name\":\"bob\",\"research_group_id\": 31}";
        create_proposal(1, dbs_proposal::action_t::dropout_member, exclude_member_json, "alice", 31, time_point_sec(0xffffffff), 1);

        vote_proposal_operation op;

        op.research_group_id = 31;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        BOOST_CHECK_THROW(research_group_service.get_token_by_account_and_research_group("bob", 31), fc::exception);
        BOOST_CHECK(
                research_group_service.get_token_by_account_and_research_group("alice", 31).amount == DEIP_100_PERCENT);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_research_review_share_test)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice));

        vector<std::pair<account_name_type, share_type>> accounts = { std::make_pair("alice", 9000), std::make_pair("bob", 1000) };

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 40));

        setup_research_group(31, "name", "research_group", "research group", 0, proposal_quorums, false, accounts);

        const auto& new_research = db.create<research_object>([&](research_object& r) {
            r.id = 20;
            fc::from_string(r.title, "title");
            fc::from_string(r.abstract, "abstract");
            fc::from_string(r.permlink, "permlink");
            r.research_group_id = 31;
            r.review_share_in_percent = 1500;
            r.dropout_compensation_in_percent = 1500;
            r.is_finished = false;
            r.owned_tokens = DEIP_100_PERCENT;
            r.created_at = db.head_block_time();
            r.last_update_time = db.head_block_time();
            r.review_share_in_percent_last_update = fc::time_point_sec(db.head_block_time().sec_since_epoch() - DAYS_TO_SECONDS(100));
        });

        const std::string change_review_share_proposal_json = "{\"review_share_in_percent\": 4500,\"research_id\": 20}";
        create_proposal(1, dbs_proposal::action_t::change_research_review_share_percent, change_review_share_proposal_json, "alice", 31, time_point_sec(0xffffffff), 4000);

        vote_proposal_operation op;

        op.research_group_id = 31;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        BOOST_CHECK(new_research.review_share_in_percent == 4500);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(exclude_member_with_research_token_compensation_test)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob));

        auto& research_group_service = db.obtain_service<dbs_research_group>();
        vector<std::pair<account_name_type, share_type>> accounts = { std::make_pair("alice", 8000), std::make_pair("bob", 2000) };

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 1));

        setup_research_group(31, "name", "research_group", "research group", 0, proposal_quorums, false, accounts);
        auto& research = research_create(0, "name","abstract", "permlink", 31, 10, DROPOUT_COMPENSATION_IN_PERCENT);

        const std::string exclude_member_json = "{\"name\":\"bob\",\"research_group_id\": 31}";
        create_proposal(1, dbs_proposal::action_t::dropout_member, exclude_member_json, "alice", 31, time_point_sec(0xffffffff), 1);

        vote_proposal_operation op;

        op.research_group_id = 31;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& research_token_service = db.obtain_service<dbs_research_token>();
        auto& research_token = research_token_service.get_by_owner_and_research("bob", research.id);

        BOOST_CHECK_THROW(research_group_service.get_token_by_account_and_research_group("bob", 1), fc::exception);
        BOOST_CHECK(research_token.account_name == "bob");
        BOOST_CHECK(research_token.amount == 300);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_quorum_test)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob));

        auto& research_group_service = db.obtain_service<dbs_research_group>();
        vector<std::pair<account_name_type,share_type>> accounts = { std::make_pair("alice", 5000), std::make_pair("bob", 5000) };

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 50));

        setup_research_group(31, "name", "research_group", "research group", 0, proposal_quorums, false, accounts);

        const std::string change_quorum_json = "{\"quorum_percent\": 8000, \"proposal_type\": 2, \"research_group_id\": 31}";
        create_proposal(1, dbs_proposal::action_t::change_quorum, change_quorum_json, "alice", 31, time_point_sec(0xffffffff), 1);

        vote_proposal_operation op;

        op.research_group_id = 31;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& research_group = research_group_service.get_research_group(31);

        BOOST_CHECK(research_group.proposal_quorums.at(invite_member) == 8000);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(start_research_execute_test)
{
    try {
        ACTORS_WITH_EXPERT_TOKENS((alice))

        std::vector<std::pair<account_name_type, share_type>> accounts = {std::make_pair("alice", 10000)};

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 1));

        setup_research_group(31, "name", "research_group", "research group", 0, proposal_quorums, false, accounts);
        const std::string json_str = "{\"title\":\"test\","
                "\"research_group_id\":31,"
                "\"abstract\":\"abstract\","
                "\"permlink\":\"permlink\","
                "\"review_share_in_percent\": 10,"
                "\"dropout_compensation_in_percent\": 1500,"
                "\"disciplines\": [1, 2, 3]}";

        create_proposal(1, dbs_proposal::action_t::start_research, json_str, "alice", 31,
                        fc::time_point_sec(0xffffffff), 1);

        vote_proposal_operation op;
        op.research_group_id = 31;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto &research_service = db.obtain_service<dbs_research>();
        auto &research = research_service.get_research(0);

        BOOST_CHECK(research.title == "test");
        BOOST_CHECK(research.abstract == "abstract");
        BOOST_CHECK(research.permlink == "permlink");
        BOOST_CHECK(research.research_group_id == 31);
        BOOST_CHECK(research.review_share_in_percent == 10);
        BOOST_CHECK(research.dropout_compensation_in_percent == DROPOUT_COMPENSATION_IN_PERCENT);

        auto &research_discipline_relation_service = db.obtain_service<dbs_research_discipline_relation>();
        auto relations = research_discipline_relation_service.get_research_discipline_relations_by_research(0);

        BOOST_CHECK(relations.size() == 3);

        BOOST_CHECK(std::any_of(relations.begin(), relations.end(),
                                [](std::reference_wrapper<const research_discipline_relation_object> wrapper) {
                                    const research_discipline_relation_object &research_discipline_relation = wrapper.get();
                                    return research_discipline_relation.id == 0
                                           && research_discipline_relation.research_id == 0
                                           && research_discipline_relation.discipline_id == 1;
                                }));

        BOOST_CHECK(std::any_of(relations.begin(), relations.end(),
                                [](std::reference_wrapper<const research_discipline_relation_object> wrapper) {
                                    const research_discipline_relation_object &research_discipline_relation = wrapper.get();
                                    return research_discipline_relation.id == 1
                                           && research_discipline_relation.research_id == 0
                                           && research_discipline_relation.discipline_id == 2;
                                }));

        BOOST_CHECK(std::any_of(relations.begin(), relations.end(),
                                [](std::reference_wrapper<const research_discipline_relation_object> wrapper) {
                                    const research_discipline_relation_object &research_discipline_relation = wrapper.get();
                                    return research_discipline_relation.id == 2
                                           && research_discipline_relation.research_id == 0
                                           && research_discipline_relation.discipline_id == 3;
                                }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(send_funds_execute_test)
{
    try {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
        fund("bob", 1000);
        std::vector<std::pair<account_name_type, share_type>> accounts = {std::make_pair("alice", 10000)};

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 1));

        setup_research_group(31, "name", "research_group", "research group", 750, proposal_quorums, false, accounts);
        const std::string json_str = "{\"research_group_id\":31,"
                "\"recipient\":\"bob\","
                "\"funds\": 250}";

        create_proposal(1, dbs_proposal::action_t::send_funds, json_str, "alice", 31, fc::time_point_sec(0xffffffff),
                        1);

        vote_proposal_operation op;
        op.research_group_id = 31;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto &account_service = db.obtain_service<dbs_account>();
        auto &bobs_account = account_service.get_account("bob");

        auto &research_group_service = db.obtain_service<dbs_research_group>();
        auto &research_group = research_group_service.get_research_group(31);

        BOOST_CHECK(research_group.balance.amount == 500);
        BOOST_CHECK(bobs_account.balance.amount == 1250);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_grant_execute_test)
{
    try {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
        fund("alice", 1000);

        create_grant_operation op;
        op.owner = "alice";
        op.balance = asset(100, DEIP_SYMBOL);
        op.target_discipline = "Mathematics";
        op.start_block = 1000;
        op.end_block = 1010;
        op.is_extendable = false;
        op.content_hash = "hash";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& grant = db.get<grant_object, by_owner_name>("alice");
        BOOST_CHECK(grant.target_discipline == 1);
        BOOST_CHECK(grant.start_block == 1000);
        BOOST_CHECK(grant.end_block == 1010);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(rebalance_research_group_tokens_execute_test)
{
    try {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
        std::vector<std::pair<account_name_type, share_type>> accounts = {std::make_pair("alice", 5000),
                                                                          std::make_pair("bob", 5000)};

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 1));

        setup_research_group(31, "name", "research_group", "research group", 750, proposal_quorums, false, accounts);
        const std::string json_str = "{\"research_group_id\":31,"
                "\"accounts\":[{"
                "\"account_name\":\"alice\","
                "\"new_amount_in_percent\": 7500 },"
                "{"
                "\"account_name\":\"bob\","
                "\"new_amount_in_percent\": 2500 }]}";
        create_proposal(1, dbs_proposal::action_t::rebalance_research_group_tokens, json_str, "alice", 31,
                        fc::time_point_sec(0xffffffff), 1);

        vote_proposal_operation op;
        op.research_group_id = 31;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto &research_group_service = db.obtain_service<dbs_research_group>();
        auto &alice_token = research_group_service.get_token_by_account_and_research_group("alice",
                                                                                           31);
        auto &bobs_token = research_group_service.get_token_by_account_and_research_group("bob", 31);

        BOOST_CHECK(alice_token.amount == 75 * DEIP_1_PERCENT);
        BOOST_CHECK(bobs_token.amount == 25 * DEIP_1_PERCENT);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(research_token_sale_execute_test)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice))
        std::vector<std::pair<account_name_type, share_type>> accounts = { std::make_pair("alice", 100)};

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 1));

        setup_research_group(31, "name", "research_group", "research group", 0, proposal_quorums, false, accounts);
        const std::string json_str = "{\"research_id\":0,\"amount_for_sale\":5000,\"start_time\":\"2020-02-08T16:00:54\",\"end_time\":\"2020-03-08T15:02:31\",\"soft_cap\":\"0.060 TESTS\",\"hard_cap\":\"0.090 TESTS\"}";

        create_proposal(1, dbs_proposal::action_t::start_research_token_sale, json_str, "alice", 31, fc::time_point_sec(0xffffffff), 1);

        auto& research = research_create(0, "name","abstract", "permlink", 31, 10, DROPOUT_COMPENSATION_IN_PERCENT);
        auto& research_token_sale_service = db.obtain_service<dbs_research_token_sale>();

        vote_proposal_operation op;
        op.research_group_id = 31;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& research_token_sale = research_token_sale_service.get_by_id(0);

        BOOST_CHECK(research_token_sale.research_id == 0);
        BOOST_CHECK(research_token_sale.start_time == fc::time_point_sec(1581177654));
        BOOST_CHECK(research_token_sale.end_time == fc::time_point_sec(1583679751));
        BOOST_CHECK(research_token_sale.total_amount == asset(0, DEIP_SYMBOL));
        BOOST_CHECK(research_token_sale.balance_tokens == 5000);
        BOOST_CHECK(research_token_sale.soft_cap == asset(60, DEIP_SYMBOL));
        BOOST_CHECK(research_token_sale.hard_cap == asset(90, DEIP_SYMBOL));
        BOOST_CHECK(research.owned_tokens == 5000);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(invite_member_data_validate_test)
{
    try {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
        const std::string json_str = "{\"name\":\"\",\"research_group_id\":1,\"research_group_token_amount\":1000}";
        create_proposal(1, dbs_proposal::action_t::invite_member, json_str, "alice", 1, fc::time_point_sec(0xffffffff),
                        1);

        vote_proposal_operation op;
        op.research_group_id = 1;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();

        BOOST_CHECK_THROW(db.push_transaction(tx, 0), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(exclude_member_data_validate_test)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
        const std::string exclude_member_json = "{\"name\":\"\",\"research_group_id\": 1}";
        create_proposal(1, dbs_proposal::action_t::dropout_member, exclude_member_json, "alice", 1, time_point_sec(0xffffffff), 1);

        vote_proposal_operation op;

        op.research_group_id = 1;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();

        BOOST_CHECK_THROW(db.push_transaction(tx, 0), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_research_review_share_data_validate_test)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice));

        const std::string create_research_proposal_json = "{\"title\":\"testresearch\","
                                                          "\"research_group_id\":31,"
                                                          "\"abstract\":\"abstract\","
                                                          "\"permlink\":\"permlink\","
                                                          "\"review_share_in_percent\": 1000,"
                                                          "\"dropout_compensation_in_percent\": 1500,"
                                                          "\"disciplines\": [1, 2, 3]}";

        const std::string change_review_share_proposal_json = "{\"review_share_in_percent\": 5100,\"research_id\": 0}";

        std::vector<std::pair<account_name_type, share_type>> accounts = { std::make_pair("alice", 100)};

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 1));

        setup_research_group(31, "name", "research_group", "research group", 0, proposal_quorums, false, accounts);

        create_proposal(1, dbs_proposal::action_t::start_research, create_research_proposal_json, "alice", 31, fc::time_point_sec(0xffffffff),
                        1);

        vote_proposal_operation start_research_vote_op;
        start_research_vote_op.research_group_id = 31;
        start_research_vote_op.proposal_id = 1;
        start_research_vote_op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(start_research_vote_op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        create_proposal(2, dbs_proposal::action_t::change_research_review_share_percent, change_review_share_proposal_json, "alice", 31, time_point_sec(0xfffff1ff), 1);

        vote_proposal_operation change_review_share_vote_op;

        change_review_share_vote_op.research_group_id = 31;
        change_review_share_vote_op.proposal_id = 2;
        change_review_share_vote_op.voter = "alice";

        signed_transaction tx2;
        tx2.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx2.operations.push_back(change_review_share_vote_op);
        tx2.sign(priv_key, db.get_chain_id());
        tx2.validate();

        BOOST_CHECK_THROW(db.push_transaction(tx2, 0), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_quorum_data_validate_test)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
        const std::string change_quorum_json = "{\"quorum_percent\": 1000, \"proposal_type\": 1, \"research_group_id\": 1}";
        create_proposal(1, dbs_proposal::action_t::change_quorum, change_quorum_json, "alice", 1, time_point_sec(0xffffffff), 1);

        vote_proposal_operation op;

        op.research_group_id = 1;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();

        BOOST_CHECK_THROW(db.push_transaction(tx, 0), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(start_research_validate_test)
{
    try {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
        const std::string json_str =
                "{\"title\":\"\","
                        "\"research_group_id\":1,"
                        "\"abstract\":\"\","
                        "\"permlink\":\"\","
                        "\"review_share_in_percent\": 500,"
                        "\"dropout_compensation_in_percent\": 1500}";
        create_proposal(1, dbs_proposal::action_t::start_research, json_str, "alice", 1, fc::time_point_sec(0xffffffff),
                        1);

        vote_proposal_operation op;
        op.research_group_id = 1;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();

        BOOST_CHECK_THROW(db.push_transaction(tx, 0), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(research_token_sale_data_validate_test)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice))
        std::vector<std::pair<account_name_type, share_type>> accounts = { std::make_pair("alice", 100)};

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 1));

        setup_research_group(31, "name", "research_group", "research group", 0, proposal_quorums, false, accounts);

        // TODO: Add check for every value
        const std::string json_str = "{\"research_id\":0,\"amount_for_sale\":9999999999,\"start_time\":\"2020-02-08T15:02:31\",\"end_time\":\"2020-01-08T15:02:31\",\"soft_cap\":9999999999,\"hard_cap\":9999994444}";

        create_proposal(1, dbs_proposal::action_t::start_research_token_sale, json_str, "alice", 31, fc::time_point_sec(0xffffffff), 1);
        research_create(0, "name","abstract", "permlink", 31, 10, DROPOUT_COMPENSATION_IN_PERCENT);

        vote_proposal_operation op;
        op.research_group_id = 31;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();

        BOOST_CHECK_THROW(db.push_transaction(tx, 0), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_research_material)
{
    ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(john)(greg))
    std::vector<std::pair<account_name_type, share_type>> accounts = { std::make_pair("alice", 100)};
    std::map<uint16_t, share_type> proposal_quorums;

    for (int i = First_proposal; i <= Last_proposal; i++)
        proposal_quorums.insert(std::make_pair(i, 1));

    setup_research_group(31, "name", "research_group", "research group", 0, proposal_quorums, false, accounts);

    auto& research = db.create<research_object>([&](research_object& r) {
        r.id = 1;
        r.title = "Research #1";
        r.permlink = "Research #1 permlink";
        r.research_group_id = 31;
        r.review_share_in_percent = 1000;
        r.dropout_compensation_in_percent = DROPOUT_COMPENSATION_IN_PERCENT;
        r.is_finished = false;
        r.created_at = db.head_block_time();
        r.abstract = "abstract for Research #1";
        r.owned_tokens = DEIP_100_PERCENT;
    });

    db.create<research_discipline_relation_object>([&](research_discipline_relation_object& rdr) {
        rdr.id = 0;
        rdr.discipline_id = 1;
        rdr.research_id = 1;
    });

    db.create<research_discipline_relation_object>([&](research_discipline_relation_object& rdr) {
        rdr.id = 1;
        rdr.discipline_id = 2;
        rdr.research_id = 1;
    });


    const std::string json_str = "{\"research_id\": 1,"
            "\"type\": 9,"
            "\"title\":\"milestone for Research #2\","
            "\"content\":\"milestone for Research #2\","
            "\"permlink\":\"milestone-research-two\","
            "\"authors\":[\"alice\"],"
            "\"references\": [3] }";

    create_proposal(2, dbs_proposal::action_t::create_research_material, json_str, "alice", 31, fc::time_point_sec(0xffffffff), 1);

    vote_proposal_operation op;
    op.research_group_id = 31;
    op.proposal_id = 2;
    op.voter = "alice";

    private_key_type priv_key = generate_private_key("alice");
    private_key_type john_priv_key = generate_private_key("john");

    signed_transaction tx;
    tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(op);
    tx.sign(priv_key, db.get_chain_id());
    tx.validate();
    db.push_transaction(tx, 0);

    auto& proposal_service = db.obtain_service<dbs_proposal>();
    auto& proposal = proposal_service.get_proposal(2);
    BOOST_CHECK(proposal.is_completed == true);

    auto& research_content_service = db.obtain_service<dbs_research_content>();
    auto contents = research_content_service.get_by_research_id(1);

    BOOST_CHECK(contents.size() == 1);
    BOOST_CHECK(std::any_of(
        contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper) {
            const research_content_object& content = wrapper.get();
            return content.id == 0 && content.research_id == 1 && content.type == research_content_type::milestone_data
                && content.content == "milestone for Research #2" && content.permlink == "milestone-research-two"
                && content.authors.size() == 1 && content.authors.find("alice") != content.authors.end()
                && content.references.size() == 1;
        }));


    make_review_operation op3;

    op3.author = "john";
    op3.research_content_id = 0;
    op3.content = "test";
    op3.is_positive = true;
    op3.weight =  DEIP_100_PERCENT;

    signed_transaction tx3;
    tx3.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
    tx3.operations.push_back(op3);
    tx3.sign(john_priv_key, db.get_chain_id());
    tx3.validate();
    db.push_transaction(tx3, 0);

    BOOST_CHECK(research.is_finished == false);

    const std::string json_str2 = "{\"research_id\": 1,"
                                  "\"type\": 2,"
                                  "\"title\":\"final result for Research #2\","
                                  "\"content\":\"final result for Research #2\","
                                  "\"permlink\":\"final-result-research-two\","
                                  "\"authors\":[\"alice\"],"
                                  "\"references\": [3] }";

    create_proposal(3, dbs_proposal::action_t::create_research_material, json_str2, "alice", 31, fc::time_point_sec(0xffff1fff), 1);

    vote_proposal_operation op2;
    op2.research_group_id = 31;
    op2.proposal_id = 3;
    op2.voter = "alice";

    signed_transaction tx2;
    tx2.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
    tx2.operations.push_back(op2);
    tx2.sign(priv_key, db.get_chain_id());
    tx2.validate();
    db.push_transaction(tx2, 0);

    auto& total_vote = db.get<total_votes_object, by_content_and_discipline>(std::make_tuple(0, 1));
    auto& total_vote2 = db.get<total_votes_object, by_content_and_discipline>(std::make_tuple(0, 2));

    BOOST_CHECK(total_vote.total_weight == 10000);
    BOOST_CHECK(total_vote2.total_weight == 10000);

    BOOST_CHECK(research.is_finished == true);
}

BOOST_AUTO_TEST_CASE(check_dgpo_used_power)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: make_review expertise");

        ACTORS_WITH_EXPERT_TOKENS((john)(alice)(jack));

        generate_block();

        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 7000));

        auto& research = research_create(1, "test_research", "test_abstract", "test_permlink", 30, 10, 1500);
        research_group_create(30, "group3", "test3", "test3", 100, proposal_quorums, false);
        research_group_token_create(30, "john", DEIP_1_PERCENT * 60);
        research_group_token_create(30, "alice", DEIP_1_PERCENT * 40);

        db.create<research_content_object>([&](research_content_object& c) {
            c.id = 1;
            c.created_at = fc::time_point_sec(db.head_block_time() - 60 * 60 * 5);
            c.research_id = research.id;
            c.authors = {"alice"};
            c.content = "content";
            c.references = {};
            c.external_references = { "http://google.com" };
            c.type = research_content_type::milestone_data;
            c.activity_state = research_content_activity_state::active;
        });

        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& rdr) {
            rdr.discipline_id = 1;
            rdr.research_id = 1;
        });

        private_key_type priv_key = generate_private_key("john");
        private_key_type alice_key = generate_private_key("alice");
        private_key_type jack_key = generate_private_key("jack");

        generate_block();

        make_review_operation op;

        std::vector<int64_t> references {1};
        op.author = "jack";
        op.research_content_id = 1;
        op.content = "test";
        op.is_positive = true;
        op.weight = DEIP_100_PERCENT;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(jack_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& stats = db.get_expertise_stats();

        BOOST_CHECK(stats.used_expertise_per_block.value == 10000);

        generate_block();

        BOOST_CHECK(stats.get_expertise_used_last_week().value == 10000);
        BOOST_CHECK(stats.total_used_expertise.value == 10000);
        BOOST_CHECK(stats.used_expertise_per_block.value == 0);

        BOOST_TEST_MESSAGE("Testing: vote for review expertise");

        vote_for_review_operation op2;

        tx.operations.clear();
        tx.signatures.clear();

        dbs_expert_token& expert_token_service = db.obtain_service<dbs_expert_token>();
        expert_token_service.get_expert_token_by_account_and_discipline("john", 1);

        op2.review_id = 0;
        op2.discipline_id = 1;
        op2.weight = 50 * DEIP_1_PERCENT;
        op2.voter = "john";

        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back(op2);
        tx.sign(priv_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        BOOST_CHECK(stats.used_expertise_per_block == 5000);

        generate_block();

        BOOST_CHECK(stats.get_expertise_used_last_week().value == 15000);
        BOOST_CHECK(stats.total_used_expertise == 15000);
        BOOST_CHECK(stats.used_expertise_per_block == 0);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(vote_for_negative_review)
{
    try {
        BOOST_TEST_MESSAGE("Testing: make_review_research_apply");

        ACTORS_WITH_EXPERT_TOKENS((alice));

        generate_block();

        BOOST_TEST_MESSAGE("--- Test normal review creation");
        auto &research = research_create(1, "test_research", "abstract", "permlink", 1, 10, 1500);
        db.create<research_content_object>([&](research_content_object &c) {
            c.id = 1;
            c.created_at = fc::time_point_sec(db.head_block_time() - 60 * 60 * 5);
            c.research_id = research.id;
            c.authors = {"alice", "bob"};
            c.content = "content";
            c.references = {};
            c.external_references = {"http://google.com"};
            c.type = research_content_type::milestone_data;
            c.activity_state = research_content_activity_state::active;
        });

        db.create<research_discipline_relation_object>([&](research_discipline_relation_object &rdr) {
            rdr.discipline_id = 1;
            rdr.research_id = 1;
        });

        private_key_type priv_key = generate_private_key("alice");

        make_review_operation op;

        std::vector<int64_t> references{1};
        op.author = "alice";
        op.research_content_id = 1;
        op.content = "test";
        op.is_positive = false;
        op.weight = DEIP_100_PERCENT;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        const review_object &review = db.get<review_object, by_id>(0);

        vote_for_review_operation op2;

        signed_transaction tx2;
        tx2.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);

        BOOST_TEST_MESSAGE("--- Testing success");

        dbs_discipline &discipline_service = db.obtain_service<dbs_discipline>();
        auto &discipline = discipline_service.get_discipline(1);

        op2.review_id = review.id._id;
        op2.discipline_id = discipline.id._id;
        op2.weight = 50 * DEIP_1_PERCENT;
        op2.voter = "alice";

        tx2.operations.clear();
        tx2.signatures.clear();
        tx2.operations.push_back(op2);
        tx2.sign(alice_private_key, db.get_chain_id());

        db.push_transaction(tx2, 0);

        // TODO: Complete test
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_research_tokens_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: transfer_research_tokens_apply");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob));

        generate_block();

        auto& alice_token = research_token_create(0, "alice", 5000, 1);
        auto& research = research_create(1, "title", "abstract", "permlink", 1, 1, 1);

        private_key_type priv_key = generate_private_key("alice");

        db.modify(research, [&](research_object& r_o){
            r_o.owned_tokens = 50 * DEIP_1_PERCENT;
        });

        transfer_research_tokens_operation op;

        op.research_id = 1;
        op.amount = 40 * DEIP_1_PERCENT;
        op.sender = "alice";
        op.receiver = "bob";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);
        
        auto& bob_token = db.get<research_token_object>(1);

        BOOST_CHECK(alice_token.amount == 10 * DEIP_1_PERCENT);
        BOOST_CHECK(bob_token.amount == 40 * DEIP_1_PERCENT);

        transfer_research_tokens_operation op2;

        op2.research_id = 1;
        op2.amount = 10 * DEIP_1_PERCENT;
        op2.sender = "alice";
        op2.receiver = "bob";

        signed_transaction tx2;
        tx2.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx2.operations.push_back(op2);
        tx2.sign(priv_key, db.get_chain_id());
        tx2.validate();
        db.push_transaction(tx2, 0);
        
        BOOST_CHECK_THROW(db.get<research_token_object>(0), std::out_of_range);
        BOOST_CHECK(bob_token.amount == 50 * DEIP_1_PERCENT);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(unique_proposal_hash_test)
{
    try {
        ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
        std::vector<std::pair<account_name_type, share_type>> accounts = {std::make_pair("alice", 10000)};
        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 1));

        setup_research_group(31, "name", "research_group", "research group", 0, proposal_quorums, false, accounts);

        const std::string json_str = "{\"name\":\"bob\",\"research_group_id\":31,\"research_group_token_amount_in_percent\":5000}";

        create_proposal_operation op;
        op.creator = "alice";
        op.research_group_id = 31;
        op.data = json_str;
        op.action = dbs_proposal::action_t::invite_member;
        op.expiration_time = db.head_block_time() + DAYS_TO_SECONDS(7);

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        dbs_proposal& proposal_service = db.obtain_service<dbs_proposal>();
        auto &invite_proposal = proposal_service.get_proposal(0);

        BOOST_CHECK(invite_proposal.id == 0);
        BOOST_CHECK(invite_proposal.research_group_id == 31);
        BOOST_CHECK(invite_proposal.creator == "alice");
        BOOST_CHECK(invite_proposal.action == dbs_proposal::action_t::invite_member);

        const std::string json_str_with_spaces = "{\"name\":\"bob\", \"research_group_id\":31        ,\"research_group_token_amount_in_percent\":             5000}";

        create_proposal_operation op2;
        op2.creator = "alice";
        op2.research_group_id = 31;
        op2.data = json_str_with_spaces;
        op2.action = dbs_proposal::action_t::invite_member;
        op2.expiration_time = db.head_block_time() + DAYS_TO_SECONDS(5);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(op2);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();

        BOOST_CHECK_THROW(db.push_transaction(tx, 0), fc::assert_exception);

        const std::string exclude_member_json = "{\"name\":\"bob\",\"research_group_id\": 31}";

        create_proposal_operation op3;
        op3.creator = "alice";
        op3.research_group_id = 31;
        op3.data = exclude_member_json;
        op3.action = dbs_proposal::action_t::dropout_member;
        op3.expiration_time = db.head_block_time() + DAYS_TO_SECONDS(7);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(op3);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto &dropout_proposal = proposal_service.get_proposal(1);

        BOOST_CHECK(dropout_proposal.id == 1);
        BOOST_CHECK(dropout_proposal.research_group_id == 31);
        BOOST_CHECK(dropout_proposal.creator == "alice");
        BOOST_CHECK(dropout_proposal.action == dbs_proposal::action_t::dropout_member);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(delegate_expertise_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: delegate_expertise_apply");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(jack));

        generate_block();

        private_key_type bob_priv_key = generate_private_key("bob");
        private_key_type jack_priv_key = generate_private_key("jack");

        delegate_expertise_operation op;

        op.sender = "bob";
        op.receiver = "alice";
        op.discipline_id = 1;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(bob_priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        BOOST_CHECK((db.get<expert_token_object, by_account_and_discipline>(std::make_tuple("alice", 1)).proxied_expertise[0]) == 10000);

        BOOST_CHECK_THROW(db.push_transaction(tx, 0), fc::assert_exception);

        delegate_expertise_operation op2;

        op2.sender = "jack";
        op2.receiver = "alice";
        op2.discipline_id = 1;

        signed_transaction tx2;
        tx2.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx2.operations.push_back(op2);
        tx2.sign(jack_priv_key, db.get_chain_id());
        tx2.validate();
        db.push_transaction(tx2, 0);

        BOOST_CHECK((db.get<expert_token_object, by_account_and_discipline>(std::make_tuple("alice", 1)).proxied_expertise[0]) == 20000);
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(withdraw_expertise_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: delegate expertise and withdraw then");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(jack)(mike));

        generate_block();

        private_key_type bob_priv_key = generate_private_key("bob");
        private_key_type jack_priv_key = generate_private_key("jack");
        private_key_type mike_priv_key = generate_private_key("mike");

        delegate_expertise_operation op;

        op.sender = "bob";
        op.receiver = "alice";
        op.discipline_id = 1;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(bob_priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        BOOST_CHECK_THROW(db.push_transaction(tx, 0), fc::assert_exception);

        delegate_expertise_operation op2;

        op2.sender = "jack";
        op2.receiver = "alice";
        op2.discipline_id = 1;

        signed_transaction tx2;
        tx2.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx2.operations.push_back(op2);
        tx2.sign(jack_priv_key, db.get_chain_id());
        tx2.validate();
        db.push_transaction(tx2, 0);

        revoke_expertise_delegation_operation op3;

        op3.sender = "jack";
        op3.discipline_id = 1;

        signed_transaction tx3;
        tx3.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx3.operations.push_back(op3);
        tx3.sign(jack_priv_key, db.get_chain_id());
        tx3.validate();
        db.push_transaction(tx3, 0);

        auto test = db.get<expert_token_object, by_account_and_discipline>(std::make_tuple("alice", 1)).proxied_expertise;
        BOOST_CHECK(test[0] == 10000);

        revoke_expertise_delegation_operation op4;

        op4.sender = "mike";
        op4.discipline_id = 1;

        signed_transaction tx4;
        tx4.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx4.operations.push_back(op4);
        tx4.sign(mike_priv_key, db.get_chain_id());
        tx4.validate();
        BOOST_CHECK_THROW(db.push_transaction(tx4, 0), fc::assert_exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(expertise_allocation_proposal_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: create expertise allocation proposal");

        ACTORS_WITH_EXPERT_TOKENS((alice));
        ACTORS((bob))

        generate_block();

        private_key_type bob_priv_key = generate_private_key("bob");

        create_expertise_allocation_proposal_operation op;

        op.claimer = "bob";
        op.discipline_id = 1;
        op.description = "test";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(bob_priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& expertise_allocation_proposal = db.get<expertise_allocation_proposal_object, by_id>(0);

        BOOST_CHECK(expertise_allocation_proposal.claimer == "bob");
        BOOST_CHECK(expertise_allocation_proposal.discipline_id == 1);

        create_expertise_allocation_proposal_operation op2;

        op2.claimer = "bob";
        op2.discipline_id = 1;
        op2.description = "test";

        signed_transaction tx2;
        tx2.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx2.operations.push_back(op2);
        tx2.sign(bob_priv_key, db.get_chain_id());
        tx2.validate();
        BOOST_CHECK_THROW(db.push_transaction(tx2, 0), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(vote_for_expertise_allocation_proposal_apply)
{
    try
    {
        ACTORS_WITH_EXPERT_TOKENS((alice)(john)(jack)(mike)(peter)(miles)(kate));
        ACTOR(bob);

        generate_block();

        private_key_type bob_priv_key = generate_private_key("bob");
        private_key_type jack_priv_key = generate_private_key("jack");
        private_key_type john_priv_key = generate_private_key("john");
        private_key_type kate_priv_key = generate_private_key("kate");

        db.modify(db.get<discipline_object>(1), [&](discipline_object& d) {
            d.total_expertise_amount = 70000;
        });

        create_expertise_allocation_proposal_operation op;

        op.claimer = "bob";
        op.discipline_id = 1;
        op.description = "test";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(bob_priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& expertise_allocation_proposal = db.get<expertise_allocation_proposal_object, by_id>(0);

        BOOST_CHECK(expertise_allocation_proposal.claimer == "bob");
        BOOST_CHECK(expertise_allocation_proposal.discipline_id == 1);

        vote_for_expertise_allocation_proposal_operation op2;

        op2.proposal_id = 0;
        op2.voter = "jack";
        op2.voting_power = DEIP_100_PERCENT;

        signed_transaction tx2;
        tx2.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx2.operations.push_back(op2);
        tx2.sign(jack_priv_key, db.get_chain_id());
        tx2.validate();
        db.push_transaction(tx2, 0);

        BOOST_CHECK(expertise_allocation_proposal.total_voted_expertise == 10000);

        vote_for_expertise_allocation_proposal_operation op2_1;

        op2_1.proposal_id = 0;
        op2_1.voter = "jack";
        op2_1.voting_power = -DEIP_100_PERCENT;

        signed_transaction tx2_1;
        tx2_1.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx2_1.operations.push_back(op2_1);
        tx2_1.sign(jack_priv_key, db.get_chain_id());
        tx2_1.validate();
        db.push_transaction(tx2_1, 0);

        auto& expertise_allocation_proposal_2 = db.get<expertise_allocation_proposal_object, by_id>(0);

        BOOST_CHECK(expertise_allocation_proposal_2.total_voted_expertise == -10000);

    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(offer_research_tokens_proposal)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: offer_research_tokens_proposal");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(jack)(mike));

        generate_block();

        db.create<research_object>([&](research_object& r) {
            r.id = 100;
            r.research_group_id = 1;
            r.title = "title";
            r.abstract = "abstract";
            r.permlink = "permlink";
            r.owned_tokens = 50 * DEIP_1_PERCENT;
        });

        std::vector<std::pair<account_name_type, share_type>> accounts = {std::make_pair("alice", 10000)};
        std::map<uint16_t, share_type> proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            proposal_quorums.insert(std::make_pair(i, 0));

        setup_research_group(31, "name", "research_group", "research group", 0, proposal_quorums, false, accounts);
        const std::string json_str = "{\"sender\":\"alice\",\"receiver\":\"mike\",\"research_id\": 100,\"amount\":1000,\"price\":\"1.000 TESTS\"}";

        create_proposal(1, dbs_proposal::action_t::offer_research_tokens, json_str, "alice", 31, fc::time_point_sec(0xffffffff),
                        1);


        auto &offer_service = db.obtain_service<dbs_offer_research_tokens>();

        vote_proposal_operation op;
        op.research_group_id = 31;
        op.proposal_id = 1;
        op.voter = "alice";

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& offer = offer_service.get(0);

        BOOST_CHECK(offer.sender == "alice");
        BOOST_CHECK(offer.receiver == "mike");
        BOOST_CHECK(offer.research_id == 100);
        BOOST_CHECK(offer.amount == 1000);
        BOOST_CHECK(offer.price.amount == 1000);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(accept_offer_research_tokens_proposal)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: accept_offer_research_tokens_proposal");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(jack)(mike));

        generate_block();

        fund("bob", 500000);

        db.create<research_object>([&](research_object& r) {
            r.id = 100;
            r.research_group_id = 1;
            r.title = "title";
            r.abstract = "abstract";
            r.permlink = "permlink";
            r.owned_tokens = 50 * DEIP_1_PERCENT;
        });

        dbs_offer_research_tokens& offer_service = db.obtain_service<dbs_offer_research_tokens>();
        dbs_research& research_service = db.obtain_service<dbs_research>();
        dbs_research_token& research_token_service = db.obtain_service<dbs_research_token>();

        offer_service.create("alice", "bob", 100, 10 * DEIP_1_PERCENT, asset(1, DEIP_SYMBOL));

        accept_research_token_offer_operation op;
        op.offer_research_tokens_id = 0;
        op.buyer = "bob";

        private_key_type priv_key = generate_private_key("bob");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        BOOST_CHECK_THROW(offer_service.check_offer_existence(op.offer_research_tokens_id), fc::assert_exception);

        auto& buyer = db.get_account(op.buyer);
        auto& research = research_service.get_research(100);

        BOOST_CHECK(buyer.name == "bob");
        BOOST_CHECK(buyer.balance.amount == 499999);

        BOOST_CHECK(research.owned_tokens == 40 * DEIP_1_PERCENT);

        auto& token = research_token_service.get_by_owner_and_research("bob", 100);

        BOOST_CHECK(token.account_name == "bob");
        BOOST_CHECK(token.research_id == 100);
        BOOST_CHECK(token.amount == 10 * DEIP_1_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reject_offer_research_tokens_proposal)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: reject_offer_research_tokens_proposal");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(jack)(mike));

        generate_block();

        fund("bob", 500000);

        db.create<research_object>([&](research_object& r) {
            r.id = 100;
            r.research_group_id = 1;
            r.title = "title";
            r.abstract = "abstract";
            r.permlink = "permlink";
            r.owned_tokens = 50 * DEIP_1_PERCENT;
        });

        dbs_offer_research_tokens& offer_service = db.obtain_service<dbs_offer_research_tokens>();
        dbs_research& research_service = db.obtain_service<dbs_research>();
        dbs_research_token& research_token_service = db.obtain_service<dbs_research_token>();

        offer_service.create("alice", "bob", 100, 10 * DEIP_1_PERCENT, asset(1, DEIP_SYMBOL));

        reject_research_token_offer_operation op;
        op.offer_research_tokens_id = 0;
        op.buyer = "bob";

        private_key_type priv_key = generate_private_key("bob");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        BOOST_CHECK_THROW(offer_service.check_offer_existence(op.offer_research_tokens_id), fc::assert_exception);

        auto& buyer = db.get_account(op.buyer);
        auto& research = research_service.get_research(100);

        BOOST_CHECK(buyer.name == "bob");
        BOOST_CHECK(buyer.balance.amount == 500000);

        BOOST_CHECK(research.owned_tokens == 50 * DEIP_1_PERCENT);

        BOOST_CHECK_THROW(research_token_service.check_existence_by_owner_and_research("bob", 100), fc::assert_exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(calculate_eci_test_case)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: calculate_eci_test_case");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(john)(rachel));

        generate_block();

        BOOST_TEST_MESSAGE("--- Test normal review creation");
        auto& research = research_create(1, "test_research", "abstract", "permlink", 1, 10, 1500);
        db.create<research_content_object>([&](research_content_object& c) {
            c.id = 1;
            c.created_at = fc::time_point_sec(db.head_block_time() - 60 * 60 * 5);
            c.research_id = research.id;
            c.authors = { "alice" };
            c.content = "content";
            c.references = {};
            c.external_references = { "http://google.com" };
            c.type = research_content_type::milestone_data;
            c.activity_state = research_content_activity_state::active;
            c.permlink = "12";
        });

        auto& content = db.create<research_content_object>([&](research_content_object& c) {
            c.id = 2;
            c.created_at = fc::time_point_sec(db.head_block_time() - 60 * 60 * 5);
            c.research_id = research.id;
            c.authors = { "alice" };
            c.content = "content2";
            c.references = {};
            c.external_references = { "http://google.com" };
            c.type = research_content_type::milestone_data;
            c.activity_state = research_content_activity_state::active;
            c.permlink = "123";
        });

        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& rdr) {
            rdr.discipline_id = 1;
            rdr.research_id = 1;
        });

        private_key_type alice_priv_key = generate_private_key("alice");
        private_key_type bob_priv_key = generate_private_key("bob");
        private_key_type rachel_priv_key = generate_private_key("rachel");
        private_key_type john_priv_key = generate_private_key("john");

        make_review_operation op;

        std::vector<int64_t> references {1};
        op.author = "bob";
        op.research_content_id = 1;
        op.content = "test";
        op.is_positive = true;
        op.weight =  DEIP_100_PERCENT;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(bob_priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        validate_database();

        auto& review = db.get<review_object, by_author_and_research_content>(std::make_tuple("bob", 1));

        db.modify(review, [&](review_object& r_o){
            r_o.created_at = fc::time_point_sec(db.head_block_time() - 60 * 60 * 5);
        });
        vote_for_review_operation op2;

        op2.review_id = review.id._id;
        op2.discipline_id = 1;
        op2.weight = DEIP_100_PERCENT;
        op2.voter = "alice";

        signed_transaction tx2;
        tx2.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx2.operations.push_back(op2);
        tx2.sign(alice_priv_key, db.get_chain_id());
        tx2.validate();
        db.push_transaction(tx2, 0);

        auto eci1 = db.get<research_object>(1).eci_per_discipline.at(1);

        vote_for_review_operation op3;

        op3.review_id = review.id._id;
        op3.discipline_id = 1;
        op3.weight = DEIP_100_PERCENT;
        op3.voter = "john";

        signed_transaction tx3;
        tx3.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx3.operations.push_back(op3);
        tx3.sign(john_priv_key, db.get_chain_id());
        tx3.validate();
        db.push_transaction(tx3, 0);

        auto eci2 = db.get<research_object>(1).eci_per_discipline.at(1);

        make_review_operation op4;

        op4.author = "bob";
        op4.research_content_id = 2;
        op4.content = "test3";
        op4.is_positive = false;
        op4.weight =  10000;

        signed_transaction tx4;
        tx4.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx4.operations.push_back(op4);
        tx4.sign(bob_priv_key, db.get_chain_id());
        tx4.validate();
        db.push_transaction(tx4, 0);

        auto eci3 = db.get<research_object>(1).eci_per_discipline.at(1);

        auto& review2 = db.get<review_object, by_author_and_research_content>(std::make_tuple("bob", 2));

        db.modify(review2, [&](review_object& r_o){
            r_o.created_at = fc::time_point_sec(db.head_block_time() - 60 * 60 * 5);
        });

        vote_for_review_operation op5;

        op5.review_id = review2.id._id;
        op5.discipline_id = 1;
        op5.weight = DEIP_100_PERCENT;
        op5.voter = "rachel";

        signed_transaction tx5;
        tx5.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx5.operations.push_back(op5);
        tx5.sign(rachel_priv_key, db.get_chain_id());
        tx5.validate();
        db.push_transaction(tx5, 0);

        auto eci4 = db.get<research_object>(1).eci_per_discipline.at(1);

        BOOST_CHECK(eci4 == 2000);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
