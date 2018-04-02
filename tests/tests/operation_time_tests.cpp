#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/protocol/exceptions.hpp>

#include <deip/chain/block_summary_object.hpp>
#include <deip/chain/database.hpp>
#include <deip/chain/hardfork.hpp>
#include <deip/chain/history_object.hpp>
#include <deip/chain/deip_objects.hpp>

#include <deip/chain/util/reward.hpp>

#include <deip/plugins/debug_node/debug_node_plugin.hpp>

#include <fc/crypto/digest.hpp>

#include "database_fixture.hpp"

#include <cmath>

using namespace deip;
using namespace deip::chain;
using namespace deip::protocol;

BOOST_FIXTURE_TEST_SUITE(operation_time_tests, clean_database_fixture)

// BOOST_AUTO_TEST_CASE(vesting_withdrawals)
// {
//     try
//     {
//         ACTORS_WITH_EXPERT_TOKENS((alice))
//         fund("alice", 100000);
//         vest("alice", 100000);

//         const auto& new_alice = db.get_account("alice");

//         BOOST_TEST_MESSAGE("Setting up withdrawal");

//         signed_transaction tx;
//         withdraw_vesting_operation op;
//         op.account = "alice";
//         op.vesting_shares = asset(new_alice.vesting_shares.amount / 2, VESTS_SYMBOL);
//         tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
//         tx.operations.push_back(op);
//         tx.sign(alice_private_key, db.get_chain_id());
//         db.push_transaction(tx, 0);

//         auto next_withdrawal = db.head_block_time() + DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS;
//         asset vesting_shares = new_alice.vesting_shares;
//         asset to_withdraw = op.vesting_shares;
//         asset original_vesting = vesting_shares;
//         asset withdraw_rate = new_alice.vesting_withdraw_rate;

//         BOOST_TEST_MESSAGE("Generating block up to first withdrawal");
//         generate_blocks(next_withdrawal - (DEIP_BLOCK_INTERVAL / 2), true);

//         BOOST_REQUIRE(db.get_account("alice").vesting_shares.amount.value == vesting_shares.amount.value);

//         BOOST_TEST_MESSAGE("Generating block to cause withdrawal");
//         generate_block();

//         auto fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();
//         auto gpo = db.get_dynamic_global_properties();

//         BOOST_REQUIRE(db.get_account("alice").vesting_shares.amount.value
//                       == (vesting_shares - withdraw_rate).amount.value);
//         BOOST_REQUIRE((withdraw_rate * gpo.get_vesting_share_price()).amount.value
//                           - db.get_account("alice").balance.amount.value
//                       <= 1); // Check a range due to differences in the share price
//         BOOST_REQUIRE(fill_op.from_account == "alice");
//         BOOST_REQUIRE(fill_op.to_account == "alice");
//         BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);
//         BOOST_REQUIRE(std::abs((fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price()).amount.value)
//                       <= 1);
//         validate_database();

//         BOOST_TEST_MESSAGE("Generating the rest of the blocks in the withdrawal");

//         vesting_shares = db.get_account("alice").vesting_shares;
//         auto balance = db.get_account("alice").balance;
//         auto old_next_vesting = db.get_account("alice").next_vesting_withdrawal;

//         for (int i = 1; i < DEIP_VESTING_WITHDRAW_INTERVALS - 1; i++)
//         {
//             generate_blocks(db.head_block_time() + DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS);

//             const auto& alice = db.get_account("alice");

//             gpo = db.get_dynamic_global_properties();
//             fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();

//             BOOST_REQUIRE(alice.vesting_shares.amount.value == (vesting_shares - withdraw_rate).amount.value);
//             BOOST_REQUIRE(balance.amount.value + (withdraw_rate * gpo.get_vesting_share_price()).amount.value
//                               - alice.balance.amount.value
//                           <= 1);
//             BOOST_REQUIRE(fill_op.from_account == "alice");
//             BOOST_REQUIRE(fill_op.to_account == "alice");
//             BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);
//             BOOST_REQUIRE(std::abs((fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price()).amount.value)
//                           <= 1);

//             if (i == DEIP_VESTING_WITHDRAW_INTERVALS - 1)
//                 BOOST_REQUIRE(alice.next_vesting_withdrawal == fc::time_point_sec::maximum());
//             else
//                 BOOST_REQUIRE(alice.next_vesting_withdrawal.sec_since_epoch()
//                               == (old_next_vesting + DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS).sec_since_epoch());

//             validate_database();

//             vesting_shares = alice.vesting_shares;
//             balance = alice.balance;
//             old_next_vesting = alice.next_vesting_withdrawal;
//         }

//         if (to_withdraw.amount.value % withdraw_rate.amount.value != 0)
//         {
//             BOOST_TEST_MESSAGE("Generating one more block to take care of remainder");
//             generate_blocks(db.head_block_time() + DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS, true);
//             fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();
//             gpo = db.get_dynamic_global_properties();

//             BOOST_REQUIRE(db.get_account("alice").next_vesting_withdrawal.sec_since_epoch()
//                           == (old_next_vesting + DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS).sec_since_epoch());
//             BOOST_REQUIRE(fill_op.from_account == "alice");
//             BOOST_REQUIRE(fill_op.to_account == "alice");
//             BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);
//             BOOST_REQUIRE(std::abs((fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price()).amount.value)
//                           <= 1);

//             generate_blocks(db.head_block_time() + DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS, true);
//             gpo = db.get_dynamic_global_properties();
//             fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();

//             BOOST_REQUIRE(db.get_account("alice").next_vesting_withdrawal.sec_since_epoch()
//                           == fc::time_point_sec::maximum().sec_since_epoch());
//             BOOST_REQUIRE(fill_op.to_account == "alice");
//             BOOST_REQUIRE(fill_op.from_account == "alice");
//             BOOST_REQUIRE(fill_op.withdrawn.amount.value == to_withdraw.amount.value % withdraw_rate.amount.value);
//             BOOST_REQUIRE(std::abs((fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price()).amount.value)
//                           <= 1);

//             validate_database();
//         }
//         else
//         {
//             generate_blocks(db.head_block_time() + DEIP_VESTING_WITHDRAW_INTERVAL_SECONDS, true);

//             BOOST_REQUIRE(db.get_account("alice").next_vesting_withdrawal.sec_since_epoch()
//                           == fc::time_point_sec::maximum().sec_since_epoch());

//             fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();
//             BOOST_REQUIRE(fill_op.from_account == "alice");
//             BOOST_REQUIRE(fill_op.to_account == "alice");
//             BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);
//             BOOST_REQUIRE(std::abs((fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price()).amount.value)
//                           <= 1);
//         }

//         BOOST_REQUIRE(db.get_account("alice").vesting_shares.amount.value
//                       == (original_vesting - op.vesting_shares).amount.value);
//     }
//     FC_LOG_AND_RETHROW()
// }

// BOOST_AUTO_TEST_CASE(vesting_withdraw_route)
// {
//     try
//     {
//         ACTORS_WITH_EXPERT_TOKENS((alice)(bob)(sam))

//         const auto& new_alice = db.get_account("alice");
//         const auto& new_bob = db.get_account("bob");
//         const auto& new_sam = db.get_account("sam");

//         auto original_vesting = new_alice.vesting_shares;

//         fund("alice", 1040000);
//         vest("alice", 1040000);

//         auto withdraw_amount = new_alice.vesting_shares - original_vesting;

//         BOOST_TEST_MESSAGE("Setup vesting withdraw");
//         withdraw_vesting_operation wv;
//         wv.account = "alice";
//         wv.vesting_shares = withdraw_amount;

//         signed_transaction tx;
//         tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
//         tx.operations.push_back(wv);
//         tx.sign(alice_private_key, db.get_chain_id());
//         db.push_transaction(tx, 0);

//         tx.operations.clear();
//         tx.signatures.clear();

//         BOOST_TEST_MESSAGE("Setting up bob destination");
//         set_withdraw_vesting_route_operation op;
//         op.from_account = "alice";
//         op.to_account = "bob";
//         op.percent = DEIP_1_PERCENT * 50;
//         op.auto_vest = true;
//         tx.operations.push_back(op);

//         BOOST_TEST_MESSAGE("Setting up sam destination");
//         op.to_account = "sam";
//         op.percent = DEIP_1_PERCENT * 30;
//         op.auto_vest = false;
//         tx.operations.push_back(op);
//         tx.sign(alice_private_key, db.get_chain_id());
//         db.push_transaction(tx, 0);

//         BOOST_TEST_MESSAGE("Setting up first withdraw");

//         auto vesting_withdraw_rate = new_alice.vesting_withdraw_rate;
//         auto old_alice_balance = new_alice.balance;
//         auto old_alice_vesting = new_alice.vesting_shares;
//         auto old_bob_balance = new_bob.balance;
//         auto old_bob_vesting = new_bob.vesting_shares;
//         auto old_sam_balance = new_sam.balance;
//         auto old_sam_vesting = new_sam.vesting_shares;
//         generate_blocks(new_alice.next_vesting_withdrawal, true);

//         {
//             const auto& alice = db.get_account("alice");
//             const auto& bob = db.get_account("bob");
//             const auto& sam = db.get_account("sam");

//             BOOST_REQUIRE(alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate);
//             BOOST_REQUIRE(
//                 alice.balance
//                 == old_alice_balance
//                     + asset((vesting_withdraw_rate.amount * DEIP_1_PERCENT * 20) / DEIP_100_PERCENT, VESTS_SYMBOL)
//                         * db.get_dynamic_global_properties().get_vesting_share_price());
//             BOOST_REQUIRE(
//                 bob.vesting_shares
//                 == old_bob_vesting
//                     + asset((vesting_withdraw_rate.amount * DEIP_1_PERCENT * 50) / DEIP_100_PERCENT, VESTS_SYMBOL));
//             BOOST_REQUIRE(bob.balance == old_bob_balance);
//             BOOST_REQUIRE(sam.vesting_shares == old_sam_vesting);
//             BOOST_REQUIRE(
//                 sam.balance
//                 == old_sam_balance
//                     + asset((vesting_withdraw_rate.amount * DEIP_1_PERCENT * 30) / DEIP_100_PERCENT, VESTS_SYMBOL)
//                         * db.get_dynamic_global_properties().get_vesting_share_price());

//             old_alice_balance = alice.balance;
//             old_alice_vesting = alice.vesting_shares;
//             old_bob_balance = bob.balance;
//             old_bob_vesting = bob.vesting_shares;
//             old_sam_balance = sam.balance;
//             old_sam_vesting = sam.vesting_shares;
//         }

//         BOOST_TEST_MESSAGE("Test failure with greater than 100% destination assignment");

//         tx.operations.clear();
//         tx.signatures.clear();

//         op.to_account = "sam";
//         op.percent = DEIP_1_PERCENT * 50 + 1;
//         tx.operations.push_back(op);
//         tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
//         tx.sign(alice_private_key, db.get_chain_id());
//         DEIP_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

//         BOOST_TEST_MESSAGE("Test from_account receiving no withdraw");

//         tx.operations.clear();
//         tx.signatures.clear();

//         op.to_account = "sam";
//         op.percent = DEIP_1_PERCENT * 50;
//         tx.operations.push_back(op);
//         tx.sign(alice_private_key, db.get_chain_id());
//         db.push_transaction(tx, 0);

//         generate_blocks(db.get_account("alice").next_vesting_withdrawal, true);
//         {
//             const auto& alice = db.get_account("alice");
//             const auto& bob = db.get_account("bob");
//             const auto& sam = db.get_account("sam");

//             BOOST_REQUIRE(alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate);
//             BOOST_REQUIRE(alice.balance == old_alice_balance);
//             BOOST_REQUIRE(
//                 bob.vesting_shares
//                 == old_bob_vesting
//                     + asset((vesting_withdraw_rate.amount * DEIP_1_PERCENT * 50) / DEIP_100_PERCENT, VESTS_SYMBOL));
//             BOOST_REQUIRE(bob.balance == old_bob_balance);
//             BOOST_REQUIRE(sam.vesting_shares == old_sam_vesting);
//             BOOST_REQUIRE(
//                 sam.balance
//                 == old_sam_balance
//                     + asset((vesting_withdraw_rate.amount * DEIP_1_PERCENT * 50) / DEIP_100_PERCENT, VESTS_SYMBOL)
//                         * db.get_dynamic_global_properties().get_vesting_share_price());
//         }
//     }
//     FC_LOG_AND_RETHROW()
// }


BOOST_AUTO_TEST_SUITE_END()
#endif
