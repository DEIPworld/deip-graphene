#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/protocol/exceptions.hpp>

#include <deip/chain/database/database.hpp>
#include <deip/chain/database/database_exceptions.hpp>
#include <deip/chain/hardfork.hpp>

#include <deip/chain/util/reward.hpp>

#include <deip/witness/witness_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/schema/expert_token_object.hpp>
#include <deip/chain/schema/expertise_allocation_proposal_object.hpp>
#include <deip/chain/schema/research_discipline_relation_object.hpp>
#include <deip/chain/schema/review_object.hpp>
#include <deip/chain/schema/vesting_balance_object.hpp>
#include <deip/chain/schema/grant_application_object.hpp>
#include <deip/chain/schema/grant_application_review_object.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_asset.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/services/dbs_research_token.hpp>



#define COMPENSATION_IN_PERCENT 1500

using namespace deip;
using namespace deip::chain;
using namespace deip::protocol;
using fc::string;

BOOST_AUTO_TEST_SUITE(test_create_account_operation_get_authorities)

BOOST_AUTO_TEST_CASE(there_is_no_owner_authority)
{
    try
    {
        create_account_operation op;
        op.creator = "alice";
        op.new_account_name = "bob";

        flat_set<account_name_type> authorities;

        op.get_required_owner_authorities(authorities);

        BOOST_CHECK(authorities.empty() == true);
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(creator_have_active_authority)
{
    try
    {
        create_account_operation op;
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



BOOST_AUTO_TEST_CASE(account_create_apply)
{
   try
   {
       BOOST_TEST_MESSAGE("Testing: account_create_apply");

       generate_blocks(DEIP_BLOCKS_PER_HOUR);

       private_key_type priv_key = generate_private_key("alice");

       auto& account_balance_service = db.obtain_service<dbs_account_balance>();
       asset init_starting_balance = asset(account_balance_service.get_by_owner_and_asset(TEST_INIT_DELEGATE_NAME, DEIP_SYMBOL).amount, DEIP_SYMBOL);

       create_account_operation op;

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
       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset("alice", DEIP_SYMBOL).amount == 0);
       BOOST_REQUIRE(acct.id._id == acct_auth.id._id);

       /// because init_witness has created vesting shares and blocks have been produced, 100 DEIP is worth less than
       /// 100 vesting shares due to rounding
       BOOST_REQUIRE(acct.common_tokens_balance == op.fee.amount);
       BOOST_REQUIRE(acct.common_tokens_withdraw_rate == 0);
       BOOST_REQUIRE(acct.proxied_vsf_votes_total() == 0);
       BOOST_REQUIRE((init_starting_balance - asset(30000, DEIP_SYMBOL)).amount.value == asset(account_balance_service.get_by_owner_and_asset(TEST_INIT_DELEGATE_NAME, DEIP_SYMBOL).amount, DEIP_SYMBOL).amount.value);
       validate_database();

       BOOST_TEST_MESSAGE("--- Test failure of duplicate account creation");
       BOOST_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);

       BOOST_REQUIRE(acct.name == "alice");
       BOOST_REQUIRE(acct_auth.owner == authority(1, priv_key.get_public_key(), 1));
       BOOST_REQUIRE(acct_auth.active == authority(2, priv_key.get_public_key(), 2));
       BOOST_REQUIRE(acct.memo_key == priv_key.get_public_key());
       BOOST_REQUIRE(acct.proxy == "");
       BOOST_REQUIRE(acct.created == db.head_block_time());
       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset("alice", DEIP_SYMBOL).amount == 0);
       BOOST_REQUIRE(acct.common_tokens_balance == op.fee.amount);
       BOOST_REQUIRE(acct.common_tokens_withdraw_rate == 0);
       BOOST_REQUIRE(acct.proxied_vsf_votes_total().value == 0);
       BOOST_REQUIRE((init_starting_balance - asset(30000, DEIP_SYMBOL)).amount.value == asset(account_balance_service.get_by_owner_and_asset(TEST_INIT_DELEGATE_NAME, DEIP_SYMBOL).amount, DEIP_SYMBOL).amount.value);
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
               wso.median_props.account_creation_fee = asset(10000, DEIP_SYMBOL);
           });
       });
       generate_block();

       tx.clear();
       op.fee = asset(0, DEIP_SYMBOL);
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

       update_account_operation op;
       op.account = "alice";
       op.active = authority();
       op.active->weight_threshold = 1;
       op.active->add_authorities("abcdefghijklmnopq", 1);

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

       update_account_operation op;
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

       update_account_operation op;
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
       op = update_account_operation();
       op.account = "alice";
       op.active = authority();
       op.active->weight_threshold = 1;
       op.active->add_authorities("dave", 1);
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
       op.amount = asset(2500, DEIP_SYMBOL);

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

       update_account_operation update_op;
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
       transfer_op.amount = asset(1000, DEIP_SYMBOL);

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

       auto& account_balance_service = db.obtain_service<dbs_account_balance>();
       const auto& new_alice = db.get_account("alice");
       const auto& new_bob = db.get_account("bob");

       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_alice.name, DEIP_SYMBOL).amount == 10000);
       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_bob.name, DEIP_SYMBOL).amount == 0);

       signed_transaction tx;
       transfer_operation op;

       op.from = "alice";
       op.to = "bob";
       op.amount = asset(5000, DEIP_SYMBOL);

       BOOST_TEST_MESSAGE("--- Test normal transaction");
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_alice.name, DEIP_SYMBOL).amount == 5000);
       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_bob.name, DEIP_SYMBOL).amount == 5000);
       validate_database();

       BOOST_TEST_MESSAGE("--- Generating a block");
       generate_block();

       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_alice.name, DEIP_SYMBOL).amount == 5000);
       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_bob.name, DEIP_SYMBOL).amount == 5000);
       validate_database();

       BOOST_TEST_MESSAGE("--- Test emptying an account");
       tx.signatures.clear();
       tx.operations.clear();
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, database::skip_transaction_dupe_check);

       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_alice.name, DEIP_SYMBOL).amount == 0);
       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_bob.name, DEIP_SYMBOL).amount == 10000);
       validate_database();

       BOOST_TEST_MESSAGE("--- Test transferring non-existent funds");
       tx.signatures.clear();
       tx.operations.clear();
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);

       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_alice.name, DEIP_SYMBOL).amount == 0);
       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_bob.name, DEIP_SYMBOL).amount == 10000);
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
       op.amount = asset(250, DEIP_SYMBOL);

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
       auto& account_balance_service = db.obtain_service<dbs_account_balance>();

       const auto& new_alice = db.get_account("alice");
       const auto& new_bob = db.get_account("bob");

       const auto& gpo = db.get_dynamic_global_properties();

       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_alice.name, DEIP_SYMBOL).amount == 10000);

       share_type shares = gpo.total_common_tokens_amount;
       share_type alice_common_tokens = new_alice.common_tokens_balance;
       share_type bob_common_tokens = new_bob.common_tokens_balance;

       transfer_to_common_tokens_operation op;
       op.from = "alice";
       op.to = "";
       op.amount = asset(7500, DEIP_SYMBOL);

       signed_transaction tx;
       tx.operations.push_back(op);
       tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
       tx.sign(alice_private_key, db.get_chain_id());
       db.push_transaction(tx, 0);

       share_type new_vest = op.amount.amount;
       shares += new_vest;
       alice_common_tokens += new_vest;

       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_alice.name, DEIP_SYMBOL).amount == 2500);
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

       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_alice.name, DEIP_SYMBOL).amount == 500);
       BOOST_REQUIRE(new_alice.common_tokens_balance == alice_common_tokens);
       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_bob.name, DEIP_SYMBOL).amount == 0);
       BOOST_REQUIRE(new_bob.common_tokens_balance == bob_common_tokens);
       BOOST_REQUIRE(gpo.total_common_tokens_amount == shares);
       validate_database();

       DEIP_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);

       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_alice.name, DEIP_SYMBOL).amount == 500);
       BOOST_REQUIRE(new_alice.common_tokens_balance == alice_common_tokens);
       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_bob.name, DEIP_SYMBOL).amount == 0);
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
                   w.median_props.account_creation_fee = asset(10000, DEIP_SYMBOL);
               });

               db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& gpo) {
                   gpo.current_supply
                           += wso.median_props.account_creation_fee - asset(1, DEIP_SYMBOL) - gpo.common_tokens_fund;
                   gpo.common_tokens_fund = wso.median_props.account_creation_fee - asset(1, DEIP_SYMBOL);

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
       op.fee = asset(1000, DEIP_SYMBOL);
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
       auto& account_balance_service = db.obtain_service<dbs_account_balance>();

       fund("alice", 10000);

       private_key_type signing_key = generate_private_key("new_key");

       BOOST_TEST_MESSAGE("--- Test upgrading an account to a witness");

       witness_update_operation op;
       op.owner = "alice";
       op.url = "foo.bar";
       op.fee = asset(1000, DEIP_SYMBOL);
       op.block_signing_key = signing_key.get_public_key();
       op.props.account_creation_fee = DEIP_MIN_ACCOUNT_CREATION_FEE;
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
       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_alice.name, DEIP_SYMBOL).amount == 10000); // No fee
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
       BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(new_alice.name, DEIP_SYMBOL).amount == 10000);
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

       create_account_operation acc_create;
       acc_create.fee = asset(30000, DEIP_SYMBOL);
       acc_create.creator = "alice";
       acc_create.new_account_name = "bob";
       acc_create.owner = authority(1, generate_private_key("bob_owner").get_public_key(), 1);
       acc_create.active = authority(1, generate_private_key("bob_active").get_public_key(), 1);
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

       update_account_operation acc_update;
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
           update_account_operation op;
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
       fund("alice", asset(10000, DEIP_SYMBOL));

       generate_block();

       BOOST_TEST_MESSAGE("--- Test first tx in block");

       signed_transaction tx;
       transfer_operation op;

       op.from = "alice";
       op.to = "bob";
       op.amount = asset(1000, DEIP_SYMBOL);

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

       op.amount = asset(100, DEIP_SYMBOL);
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

BOOST_AUTO_TEST_CASE(contribute_to_token_sale_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: contribute_to_token_sale_apply");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob));
        fund("alice", 1000);
        fund("bob", 5000);

        auto& account_balance_service = db.obtain_service<dbs_account_balance>();

        generate_block();

        private_key_type alice_priv_key = generate_private_key("alice");

        research_group_create(31, "group", "test", "test", 100, true, false);

        db.create<research_object>([&](research_object& r) {
            r.id = 1;
            fc::from_string(r.title, "title");
            fc::from_string(r.abstract, "abstract");
            fc::from_string(r.permlink, "permlink");
            r.research_group_id = 31;
            r.is_finished = false;
            r.created_at = db.head_block_time();
            r.last_update_time = db.head_block_time();
            r.review_share_last_update = fc::time_point_sec(db.head_block_time().sec_since_epoch() - DAYS_TO_SECONDS(100));
        });

        research_token_sale_create(0, 1, db.head_block_time() - 60 * 60 * 5, db.head_block_time() + 60 * 60 * 5, 200, 1000, 100);
        research_token_sale_contribution_create(0, 0, "bob", 200, db.head_block_time());

        contribute_to_token_sale_operation op;

        op.token_sale_external_id = "";
        op.contributor = "alice";
        op.amount = asset(600, DEIP_SYMBOL);

        BOOST_TEST_MESSAGE("--- Test");
        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(alice_priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& alice_account = db.get_account("alice");

        BOOST_CHECK(account_balance_service.get_by_owner_and_asset(alice_account.name, DEIP_SYMBOL).amount == 800);

        auto& alice_research_token = db.get<research_token_object, by_account_name_and_research_id>(std::make_tuple("alice", 1));
        auto& bob_research_token = db.get<research_token_object, by_account_name_and_research_id>(std::make_tuple("bob", 1));

        BOOST_CHECK(alice_research_token.amount == 500);
        BOOST_CHECK(bob_research_token.amount == 500);
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
        auto& account_balance_service = db.obtain_service<dbs_account_balance>();

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

        BOOST_CHECK(account_balance_service.get_by_owner_and_asset(bob_acc.name, DEIP_SYMBOL).amount == 500);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_discipline_supply_execute_test)
{
//    try {
//        ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
//        fund("alice", 1000);
//
//        create_discipline_supply_operation op;
//        op.owner = "alice";
//        op.balance = asset(100, DEIP_SYMBOL);
//        op.target_discipline = "Mathematics";
//        op.start_block = 1000;
//        op.end_block = 1010;
//        op.is_extendable = false;
//        op.content_hash = "hash";
//
//        private_key_type priv_key = generate_private_key("alice");
//
//        signed_transaction tx;
//        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
//        tx.operations.push_back(op);
//        tx.sign(priv_key, db.get_chain_id());
//        tx.validate();
//        db.push_transaction(tx, 0);
//
//        auto& discipline_supply = db.get<discipline_supply_object, by_owner_name>("alice");
//        BOOST_CHECK(discipline_supply.target_discipline == 1);
//        BOOST_CHECK(discipline_supply.start_block == 1000);
//        BOOST_CHECK(discipline_supply.end_block == 1010);
//    }
//    FC_LOG_AND_RETHROW()
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


BOOST_AUTO_TEST_CASE(create_asset_test)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: create_asset_test");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(jack));
        generate_block();

        create_asset_operation op;

        op.issuer = "alice";
        op.symbol = "TEST";
        op.name = "TEST";
        op.description = "TEST";
        op.precision = 3;

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        dbs_asset& asset_service = db.obtain_service<dbs_asset>();
        auto& asset_obj = asset_service.get_by_string_symbol("TEST");

        BOOST_CHECK(asset_obj.string_symbol == "TEST");
        BOOST_CHECK(asset_obj.issuer == "alice");
        BOOST_CHECK(asset_obj.name == "TEST");
        BOOST_CHECK(asset_obj.description == "TEST");
        BOOST_CHECK(asset_obj.current_supply == 0);

        create_asset_operation op2;

        op2.issuer = "alice";
        op2.symbol = "TEST";
        op2.name = "TEST";
        op2.description = "TEST";
        op2.precision = 3;

        private_key_type jack_priv_key = generate_private_key("jack");

        signed_transaction tx2;
        tx2.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx2.operations.push_back(op2);
        tx2.sign(priv_key, db.get_chain_id());
        tx2.validate();
        BOOST_CHECK_THROW(db.push_transaction(tx2, 0), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(issue_asset_test)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: issue_asset_test");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(jack));
        generate_block();

        auto& new_asset1 = db.create<asset_object>([&](asset_object &a_o) {
            a_o.id = 1;
            a_o.precision = 2;
            fc::from_string(a_o.string_symbol, "USD");
            a_o.symbol =  (uint64_t(2) | (uint64_t('U') << 8) | (uint64_t('S') << 16) | (uint64_t('D') << 24));
        });

        auto& new_asset2 = db.create<asset_object>([&](asset_object &a_o) {
            a_o.id = 2;
            a_o.precision = 2;
            fc::from_string(a_o.string_symbol, "EUR");
            a_o.symbol =  (uint64_t(2) | (uint64_t('E') << 8) | (uint64_t('U') << 16) | (uint64_t('R') << 24));
        });

        issue_asset_operation op;

        op.issuer = "alice";
        op.amount = asset(1000, new_asset1.symbol);

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        auto& account_balance_1 = db.get<account_balance_object, by_owner_and_asset_symbol>(boost::make_tuple(op.issuer, op.amount.symbol));

        BOOST_CHECK(account_balance_1.owner == "alice");
        BOOST_CHECK(account_balance_1.asset_id == 1);
        BOOST_CHECK(account_balance_1.amount == 1000);

        auto& asset_obj = db.get<asset_object, by_symbol>(op.amount.symbol);

        BOOST_CHECK(asset_obj.current_supply == 1000);

        issue_asset_operation op2;

        op2.issuer = "alice";
        op2.amount = asset(10000, new_asset1.symbol);

        signed_transaction tx2;
        tx2.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx2.operations.push_back(op2);
        tx2.sign(priv_key, db.get_chain_id());
        tx2.validate();
        db.push_transaction(tx2, 0);

        BOOST_CHECK(account_balance_1.owner == "alice");
        BOOST_CHECK(account_balance_1.asset_id == 1);
        BOOST_CHECK(account_balance_1.amount == 11000);
        BOOST_CHECK(asset_obj.current_supply == 11000);

        issue_asset_operation op4;

        op4.issuer = "bob";
        op4.amount = asset(10000, new_asset2.symbol);

        private_key_type bob_priv_key = generate_private_key("bob");

        signed_transaction tx4;
        tx4.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx4.operations.push_back(op4);
        tx4.sign(bob_priv_key, db.get_chain_id());
        tx4.validate();
        db.push_transaction(tx4, 0);

        auto& account_balance_2 = db.get<account_balance_object, by_owner_and_asset_symbol>(boost::make_tuple(op4.issuer, op4.amount.symbol));

        BOOST_CHECK(account_balance_2.owner == "bob");
        BOOST_CHECK(account_balance_2.asset_id == 2);
        BOOST_CHECK(account_balance_2.amount == 10000);

        auto& asset_obj_2 = db.get<asset_object, by_symbol>(op4.amount.symbol);

        BOOST_CHECK(asset_obj_2.current_supply == 10000);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reserve_asset_test)
{
    try {
        BOOST_TEST_MESSAGE("Testing: reserve_asset_test");

        ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(jack));
        generate_block();

        auto& new_asset1 = db.create<asset_object>([&](asset_object &a_o) {
            a_o.id = 1;
            a_o.precision = 2;
            fc::from_string(a_o.string_symbol, "USD");
            a_o.symbol =  (uint64_t(2) | (uint64_t('U') << 8) | (uint64_t('S') << 16) | (uint64_t('D') << 24));
        });

        auto& alice_balance = db.create<account_balance_object>([&](account_balance_object &ab_o) {
            ab_o.asset_id = 1;
            ab_o.owner = "alice";
            ab_o.amount = 10000;
            ab_o.symbol = new_asset1.symbol;
        });

        db.modify(new_asset1, [&](asset_object &a_o) {
            a_o.current_supply += 10000;
        });

        auto asset_1_current_supply = new_asset1.current_supply;

        reserve_asset_operation op;

        op.owner = "alice";
        op.amount = asset(1000, new_asset1.symbol);

        private_key_type priv_key = generate_private_key("alice");

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        BOOST_CHECK(alice_balance.amount == 9000);
        BOOST_CHECK(new_asset1.current_supply == asset_1_current_supply - 1000);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
