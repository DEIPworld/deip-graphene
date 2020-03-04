#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_discipline_supply.hpp>

#include "database_fixture.hpp"

#include <limits>

using namespace deip;
using namespace deip::chain;
using namespace deip::protocol;
using fc::string;

//
// usage for all discipline_supply tests 'chain_test  -t discipline_supply_*'
//

class discipline_supply_service_check_fixture : public timed_blocks_database_fixture
{
public:
    discipline_supply_service_check_fixture()
        : discipline_supply_service(db.obtain_service<dbs_discipline_supply>())
        , account_service(db.obtain_service<dbs_account>())
        , account_balance_service(db.obtain_service<dbs_account_balance>())
        , public_key(database_fixture::generate_private_key("user private key").get_public_key())
        , fake(account_service.create_account_by_faucets("",
                                                         "initdelegate",
                                                         public_key,
                                                         "",
                                                         authority(),
                                                         authority(),
                                                         authority(),
                                                         asset(0, DEIP_SYMBOL)))
        , alice(account_service.create_account_by_faucets(
              "alice", "initdelegate", public_key, "", authority(), authority(), authority(), asset(0, DEIP_SYMBOL)))
        , bob(account_service.create_account_by_faucets(
              "bob", "initdelegate", public_key, "", authority(), authority(), authority(), asset(0, DEIP_SYMBOL)))
    {
        account_balance_service.adjust_balance(alice.name, asset(ALICE_ACCOUNT_DISCIPLINE_SUPPLY, DEIP_SYMBOL));
        account_balance_service.adjust_balance(bob.name, asset(BOB_ACCOUNT_DISCIPLINE_SUPPLY, DEIP_SYMBOL));
    }

    dbs_discipline_supply& discipline_supply_service;
    dbs_account& account_service;
    dbs_account_balance& account_balance_service;
    const public_key_type public_key;
    const account_object& fake;
    const account_object& alice;
    const account_object& bob;

    const int FAKE_ACCOUNT_DISCIPLINE_SUPPLY = 0;
    const int ALICE_ACCOUNT_DISCIPLINE_SUPPLY = 500;
    const int BOB_ACCOUNT_DISCIPLINE_SUPPLY = 1001;

    const int START_BLOCK = int(db.head_block_num());
    const int END_BLOCK = START_BLOCK + 20;

    const int DISCIPLINE_SUPPLY_BALANCE_DEFAULT = 200;
    const int DISCIPLINE_SUPPLY_PER_BLOCK_DEFAULT = DISCIPLINE_SUPPLY_BALANCE_DEFAULT / (END_BLOCK - START_BLOCK);

    const discipline_id_type TARGET_DISCIPLINE = 1;

    const asset DISCIPLINE_SUPPLY_BALANCE = asset(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);
    const asset FUND_DISCIPLINE_SUPPLY_INITIAL_SUPPLY
        = asset(TEST_REWARD_INITIAL_SUPPLY.amount
                * (DEIP_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS - DEIP_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS)
                / DEIP_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS);
};

//
// usage for all discipline_supply tests 'chain_test  -t discipline_supply_*'
//

BOOST_FIXTURE_TEST_SUITE(discipline_supply_service_check, discipline_supply_service_check_fixture)

DEIP_TEST_CASE(is_const_ref_to_same_memory)
{
    asset balance(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);

    const auto& discipline_supply = discipline_supply_service.create_discipline_supply(alice, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK,
                                                               TARGET_DISCIPLINE, false, "hash");

    db.modify(discipline_supply, [&](discipline_supply_object& b) { b.balance.amount -= 1; });

    BOOST_REQUIRE(discipline_supply.balance.amount == (DISCIPLINE_SUPPLY_BALANCE_DEFAULT - 1));
}

DEIP_TEST_CASE(owned_discipline_supply_creation)
{
    asset balance(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);

    auto reqired_alice_balance = account_balance_service.get_by_owner_and_asset(alice.name, DEIP_SYMBOL).amount;

    const auto& discipline_supply = discipline_supply_service.create_discipline_supply(alice, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK,
                                                               TARGET_DISCIPLINE, false, "hash");

    BOOST_CHECK(discipline_supply.balance.amount == DISCIPLINE_SUPPLY_BALANCE_DEFAULT);
    BOOST_CHECK(discipline_supply.per_block == DISCIPLINE_SUPPLY_PER_BLOCK_DEFAULT);

    reqired_alice_balance -= DISCIPLINE_SUPPLY_BALANCE_DEFAULT;

    const auto& actual_account = account_service.get_account("alice");

    BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(actual_account.name, DEIP_SYMBOL).amount == reqired_alice_balance);
}

DEIP_TEST_CASE(second_owned_discipline_supply_creation)
{
    asset balance(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);

    auto reqired_alice_balance = account_balance_service.get_by_owner_and_asset(alice.name, DEIP_SYMBOL).amount;

    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(alice, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE,
                                                   false, "hash"));

    reqired_alice_balance -= DISCIPLINE_SUPPLY_BALANCE_DEFAULT;

    const auto& discipline_supply = discipline_supply_service.create_discipline_supply(alice, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK,
                                                               TARGET_DISCIPLINE, false, "hash");

    BOOST_CHECK(discipline_supply.balance.amount == DISCIPLINE_SUPPLY_BALANCE_DEFAULT);
    BOOST_CHECK(discipline_supply.per_block == DISCIPLINE_SUPPLY_PER_BLOCK_DEFAULT);

    reqired_alice_balance -= DISCIPLINE_SUPPLY_BALANCE_DEFAULT;

    const auto& actual_account = account_service.get_account("alice");

    BOOST_REQUIRE(account_balance_service.get_by_owner_and_asset(actual_account.name, DEIP_SYMBOL).amount == reqired_alice_balance);
}

DEIP_TEST_CASE(owned_discipline_supply_creation_asserts)
{
    asset balance(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_CHECK_THROW(
            discipline_supply_service.create_discipline_supply(fake, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE,
                                                   false, "hash"), fc::assert_exception);

    // asset wrong_currency_balance(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, VESTS_SYMBOL);

    // BOOST_CHECK_THROW(discipline_supply_service.create_discipline_supplies(alice, wrong_currency_balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);

    asset wrong_amount_balance(0, DEIP_SYMBOL);

    BOOST_CHECK_THROW(discipline_supply_service.create_discipline_supply(alice, wrong_amount_balance, START_BLOCK, END_BLOCK,
                                                             TARGET_DISCIPLINE, false, "hash"), fc::assert_exception);

    wrong_amount_balance = asset(-100, DEIP_SYMBOL);

    BOOST_CHECK_THROW(discipline_supply_service.create_discipline_supply(alice, wrong_amount_balance, START_BLOCK, END_BLOCK,
                                                             TARGET_DISCIPLINE, false, "hash"), fc::assert_exception);

    asset too_large_balance(ALICE_ACCOUNT_DISCIPLINE_SUPPLY * 2, DEIP_SYMBOL);

    BOOST_CHECK_THROW(
            discipline_supply_service.create_discipline_supply(alice, too_large_balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE,
                                                   false, "hash"), fc::assert_exception);
}

DEIP_TEST_CASE(discipline_supply_creation_limit)
{
    share_type bp = DEIP_LIMIT_DISCIPLINE_SUPPLIES_PER_OWNER + 1;
    BOOST_REQUIRE(BOB_ACCOUNT_DISCIPLINE_SUPPLY >= bp);

    asset balance(BOB_ACCOUNT_DISCIPLINE_SUPPLY / bp, DEIP_SYMBOL);

    for (int ci = 0; ci < DEIP_LIMIT_DISCIPLINE_SUPPLIES_PER_OWNER; ++ci)
    {
        BOOST_REQUIRE_NO_THROW(
                discipline_supply_service.create_discipline_supply(bob, balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE, false,
                                                       "hash"));
    }

    BOOST_CHECK(account_balance_service.get_by_owner_and_asset(bob.name, DEIP_SYMBOL).amount == (BOB_ACCOUNT_DISCIPLINE_SUPPLY - DEIP_LIMIT_DISCIPLINE_SUPPLIES_PER_OWNER * balance.amount));

    BOOST_REQUIRE_THROW(
            discipline_supply_service.create_discipline_supply(bob, balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE, false,
                                                   "hash"), fc::assert_exception);
}

DEIP_TEST_CASE(get_all_discipline_supplies)
{
    asset balance(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(alice, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE,
                                                   false, "hash"));
    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(bob, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE, false,
                                                   "hash"));

    auto discipline_supplies = discipline_supply_service.get_discipline_supplies();
    BOOST_REQUIRE(discipline_supplies.size() == 2);
}

DEIP_TEST_CASE(get_all_discipline_supply_count)
{
    asset balance(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(alice, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE,
                                                   false, "hash"));
    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(bob, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE, false,
                                                   "hash"));

    BOOST_REQUIRE(discipline_supply_service.get_discipline_supplies().size() == 2);
}

DEIP_TEST_CASE(lookup_discipline_supply_owners)
{
    asset balance(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(alice, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE,
                                                   false, "hash"));
    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(bob, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE, false,
                                                   "hash"));
    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(bob, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE, false,
                                                   "hash"));

    BOOST_REQUIRE(discipline_supply_service.get_discipline_supplies().size() == 3);

    BOOST_CHECK_THROW(discipline_supply_service.lookup_discipline_supply_owners("alice", std::numeric_limits<uint32_t>::max()),
                      fc::assert_exception);

    {
        auto owners = discipline_supply_service.lookup_discipline_supply_owners("", DEIP_LIMIT_DISCIPLINE_SUPPLIES_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 2);
    }

    {
        auto owners = discipline_supply_service.lookup_discipline_supply_owners("alice", DEIP_LIMIT_DISCIPLINE_SUPPLIES_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 2);
    }

    {
        auto owners = discipline_supply_service.lookup_discipline_supply_owners("bob", DEIP_LIMIT_DISCIPLINE_SUPPLIES_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 1);
    }

    {
        auto owners = discipline_supply_service.lookup_discipline_supply_owners("", 2);
        BOOST_REQUIRE(owners.size() == 2);
    }
}

DEIP_TEST_CASE(check_get_discipline_supplies)
{
    asset balance(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_REQUIRE_EQUAL(discipline_supply_service.get_discipline_supplies_by_owner("alice").size(), 0u);

    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(alice, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE,
                                                   false, "hash"));

    BOOST_REQUIRE_EQUAL(discipline_supply_service.get_discipline_supplies_by_owner("alice").size(), 1u);

    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(alice, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE,
                                                   false, "hash"));

    BOOST_REQUIRE_EQUAL(discipline_supply_service.get_discipline_supplies_by_owner("alice").size(), 2u);

    for (const discipline_supply_object& discipline_supply : discipline_supply_service.get_discipline_supplies_by_owner("alice"))
    {
        BOOST_REQUIRE(discipline_supply.owner == account_name_type("alice"));
    }
}

DEIP_TEST_CASE(check_get_discipline_supply_count)
{
    asset balance(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_REQUIRE_EQUAL(discipline_supply_service.get_discipline_supplies_by_owner("alice").size(), 0u);

    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(alice, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE,
                                                   false, "hash"));
    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(alice, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE,
                                                   false, "hash"));
    BOOST_CHECK_NO_THROW(
            discipline_supply_service.create_discipline_supply(bob, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE, false,
                                                   "hash"));

    BOOST_REQUIRE_EQUAL(discipline_supply_service.get_discipline_supplies_by_owner("alice").size(), 2u);
    BOOST_REQUIRE_EQUAL(discipline_supply_service.get_discipline_supplies_by_owner("bob").size(), 1u);
}

DEIP_TEST_CASE(allocate_cash_per_block)
{
    const auto& discipline_supply = discipline_supply_service.create_discipline_supply(alice, DISCIPLINE_SUPPLY_BALANCE, START_BLOCK, END_BLOCK,
                                                               TARGET_DISCIPLINE, false, "hash");

    for (auto i = 0; i < END_BLOCK - START_BLOCK; i++) {
        auto cash = discipline_supply_service.allocate_funds(discipline_supply);
        BOOST_REQUIRE_EQUAL(cash.amount, discipline_supply.per_block);
    }
}

// TODO rework this test
// DEIP_TEST_CASE(auto_close_fund_discipline_supply_by_balance)
//{
//    BOOST_REQUIRE_NO_THROW(
//        create_fund_discipline_supply_in_block(asset(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL), time_point_sec::maximum()));
//
//    asset total_cash(0, DEIP_SYMBOL);
//
//    for (int ci = 0; true; ++ci)
//    {
//        BOOST_REQUIRE(ci <= DISCIPLINE_SUPPLY_BALANCE_DEFAULT);
//
//        generate_block();
//        generate_block();
//
//        total_cash += allocate_cash_from_fund_discipline_supply_in_block();
//
//        if (!discipline_supply_service.get_fund_discipline_supply_count())
//        {
//            break; // discipline_supply has closed, break and check result
//        }
//    }
//
//    BOOST_REQUIRE_THROW(discipline_supply_service.get_fund_discipline_supplies(), fc::assert_exception);
//
//    BOOST_REQUIRE(total_cash.amount == DISCIPLINE_SUPPLY_BALANCE_DEFAULT);
//}

BOOST_AUTO_TEST_SUITE_END()

#endif