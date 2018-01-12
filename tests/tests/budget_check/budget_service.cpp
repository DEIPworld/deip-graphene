#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/dbs_account.hpp>
#include <deip/chain/dbs_budget.hpp>

#include "database_fixture.hpp"

#include <limits>

using namespace deip;
using namespace deip::chain;
using namespace deip::protocol;
using fc::string;

//
// usage for all budget tests 'chain_test  -t budget_*'
//

class budget_service_check_fixture : public timed_blocks_database_fixture
{
public:
    budget_service_check_fixture()
        : budget_service(db.obtain_service<dbs_budget>())
        , account_service(db.obtain_service<dbs_account>())
        , public_key(database_fixture::generate_private_key("user private key").get_public_key())
        , fake(account_service.create_account_by_faucets(DEIP_ROOT_POST_PARENT,
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
        account_service.increase_balance(alice, asset(ALICE_ACCOUNT_BUDGET, DEIP_SYMBOL));
        account_service.increase_balance(bob, asset(BOB_ACCOUNT_BUDGET, DEIP_SYMBOL));
    }

    dbs_budget& budget_service;
    dbs_account& account_service;
    const public_key_type public_key;
    const account_object& fake;
    const account_object& alice;
    const account_object& bob;

    const int FAKE_ACCOUNT_BUDGET = 0;
    const int ALICE_ACCOUNT_BUDGET = 500;
    const int BOB_ACCOUNT_BUDGET = 1001;

    const int START_BLOCK = 0;
    const int END_BLOCK = 10;

    const int BUDGET_PER_BLOCK_DEFAULT = 25;
    const int BUDGET_BALANCE_DEFAULT = 200;

    const discipline_id_type TARGET_DISCIPLINE = 1;

    const asset BUDGET_BALANCE = asset(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);
    const asset FUND_BUDGET_INITIAL_SUPPLY
        = asset(TEST_REWARD_INITIAL_SUPPLY.amount
                * (DEIP_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS - DEIP_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS)
                / DEIP_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS);
};

//
// usage for all budget tests 'chain_test  -t budget_*'
//

BOOST_FIXTURE_TEST_SUITE(budget_service_check, budget_service_check_fixture)

DEIP_TEST_CASE(is_const_ref_to_same_memory)
{
    asset balance(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);

    const auto& budget = budget_service.create_grant(alice, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE);

    db.modify(budget, [&](budget_object& b) { b.balance.amount -= 1; });

    BOOST_REQUIRE(budget.balance.amount == (BUDGET_BALANCE_DEFAULT - 1));
}

DEIP_TEST_CASE(owned_budget_creation)
{
    asset balance(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);

    auto reqired_alice_balance = alice.balance.amount;

    const auto& budget = budget_service.create_grant(alice, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE);

    BOOST_CHECK(budget.balance.amount == BUDGET_BALANCE_DEFAULT);
    BOOST_CHECK(budget.per_block == BUDGET_PER_BLOCK_DEFAULT);

    reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

    const auto& actual_account = account_service.get_account("alice");

    BOOST_REQUIRE(actual_account.balance.amount == reqired_alice_balance);
}

DEIP_TEST_CASE(second_owned_budget_creation)
{
    asset balance(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);

    auto reqired_alice_balance = alice.balance.amount;

    BOOST_CHECK_NO_THROW(budget_service.create_grant(alice, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

    const auto& budget = budget_service.create_grant(alice, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE);

    BOOST_CHECK(budget.balance.amount == BUDGET_BALANCE_DEFAULT);
    BOOST_CHECK(budget.per_block == BUDGET_PER_BLOCK_DEFAULT);

    reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

    const auto& actual_account = account_service.get_account("alice");

    BOOST_REQUIRE(actual_account.balance.amount == reqired_alice_balance);
}

DEIP_TEST_CASE(owned_budget_creation_asserts)
{
    asset balance(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_grant(fake, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);

    asset wrong_currency_balance(BUDGET_BALANCE_DEFAULT, VESTS_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_grant(alice, wrong_currency_balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);

    asset wrong_amount_balance(0, DEIP_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_grant(alice, wrong_amount_balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);

    wrong_amount_balance = asset(-100, DEIP_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_grant(alice, wrong_amount_balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);

    asset too_large_balance(ALICE_ACCOUNT_BUDGET * 2, VESTS_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_grant(alice, too_large_balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);
}

DEIP_TEST_CASE(budget_creation_limit)
{
    share_type bp = DEIP_LIMIT_BUDGETS_PER_OWNER + 1;
    BOOST_REQUIRE(BOB_ACCOUNT_BUDGET >= bp);

    asset balance(BOB_ACCOUNT_BUDGET / bp, DEIP_SYMBOL);

    for (int ci = 0; ci < DEIP_LIMIT_BUDGETS_PER_OWNER; ++ci)
    {
        BOOST_REQUIRE_NO_THROW(budget_service.create_grant(bob, balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    }

    BOOST_CHECK(bob.balance.amount == (BOB_ACCOUNT_BUDGET - DEIP_LIMIT_BUDGETS_PER_OWNER * balance.amount));

    BOOST_REQUIRE_THROW(budget_service.create_grant(bob, balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);
}

DEIP_TEST_CASE(get_all_budgets)
{
    asset balance(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_CHECK_NO_THROW(budget_service.create_grant(alice, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    BOOST_CHECK_NO_THROW(budget_service.create_grant(bob, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    auto budgets = budget_service.get_budgets();
    BOOST_REQUIRE(budgets.size() == 2);
}

DEIP_TEST_CASE(get_all_budget_count)
{
    asset balance(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_CHECK_NO_THROW(budget_service.create_grant(alice, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    BOOST_CHECK_NO_THROW(budget_service.create_grant(bob, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    BOOST_REQUIRE(budget_service.get_budgets().size() == 2);
}

DEIP_TEST_CASE(lookup_budget_owners)
{
    asset balance(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_CHECK_NO_THROW(budget_service.create_grant(alice, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    BOOST_CHECK_NO_THROW(budget_service.create_grant(bob, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    BOOST_CHECK_NO_THROW(budget_service.create_grant(bob, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    BOOST_REQUIRE(budget_service.get_budgets().size() == 3);

    BOOST_CHECK_THROW(budget_service.lookup_budget_owners("alice", std::numeric_limits<uint32_t>::max()),
                      fc::assert_exception);

    {
        auto owners = budget_service.lookup_budget_owners(DEIP_ROOT_POST_PARENT, DEIP_LIMIT_BUDGETS_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 2);
    }

    {
        auto owners = budget_service.lookup_budget_owners("alice", DEIP_LIMIT_BUDGETS_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 2);
    }

    {
        auto owners = budget_service.lookup_budget_owners("bob", DEIP_LIMIT_BUDGETS_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 1);
    }

    {
        auto owners = budget_service.lookup_budget_owners(DEIP_ROOT_POST_PARENT, 2);
        BOOST_REQUIRE(owners.size() == 2);
    }
}

DEIP_TEST_CASE(check_get_budgets)
{
    asset balance(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 0u);

    BOOST_CHECK_NO_THROW(budget_service.create_grant(alice, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 1u);

    BOOST_CHECK_NO_THROW(budget_service.create_grant(alice, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 2u);

    for (const budget_object& budget : budget_service.get_budgets("alice"))
    {
        BOOST_REQUIRE(budget.owner == account_name_type("alice"));
    }
}

DEIP_TEST_CASE(check_get_budget_count)
{
    asset balance(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 0u);

    BOOST_CHECK_NO_THROW(budget_service.create_grant(alice, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    BOOST_CHECK_NO_THROW(budget_service.create_grant(alice, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    BOOST_CHECK_NO_THROW(budget_service.create_grant(bob, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 2u);
    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("bob").size(), 1u);
}

DEIP_TEST_CASE(allocate_cash_per_block)
{
    asset balance(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL);

    const auto& budget = budget_service.create_grant(alice, BUDGET_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE);

    auto cash = budget_service.allocate_funds(budget);

    BOOST_REQUIRE_EQUAL(cash.amount, 200); // wait next block
}

// TODO rework this test
// DEIP_TEST_CASE(auto_close_fund_budget_by_balance)
//{
//    BOOST_REQUIRE_NO_THROW(
//        create_fund_budget_in_block(asset(BUDGET_BALANCE_DEFAULT, DEIP_SYMBOL), time_point_sec::maximum()));
//
//    asset total_cash(0, DEIP_SYMBOL);
//
//    for (int ci = 0; true; ++ci)
//    {
//        BOOST_REQUIRE(ci <= BUDGET_BALANCE_DEFAULT);
//
//        generate_block();
//        generate_block();
//
//        total_cash += allocate_cash_from_fund_budget_in_block();
//
//        if (!budget_service.get_fund_budget_count())
//        {
//            break; // budget has closed, break and check result
//        }
//    }
//
//    BOOST_REQUIRE_THROW(budget_service.get_fund_budgets(), fc::assert_exception);
//
//    BOOST_REQUIRE(total_cash.amount == BUDGET_BALANCE_DEFAULT);
//}

BOOST_AUTO_TEST_SUITE_END()

#endif