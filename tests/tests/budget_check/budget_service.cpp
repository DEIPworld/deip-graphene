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
        , fake(account_service.create_account_by_faucets(deip_ROOT_POST_PARENT,
                                                         "initdelegate",
                                                         public_key,
                                                         "",
                                                         authority(),
                                                         authority(),
                                                         authority(),
                                                         asset(0, deip_SYMBOL)))
        , alice(account_service.create_account_by_faucets(
              "alice", "initdelegate", public_key, "", authority(), authority(), authority(), asset(0, deip_SYMBOL)))
        , bob(account_service.create_account_by_faucets(
              "bob", "initdelegate", public_key, "", authority(), authority(), authority(), asset(0, deip_SYMBOL)))
    {
        account_service.increase_balance(alice, asset(ALICE_ACCOUNT_BUDGET, deip_SYMBOL));
        account_service.increase_balance(bob, asset(BOB_ACCOUNT_BUDGET, deip_SYMBOL));
    }

    asset allocate_all_cash_from_fund_budget_in_block();

    dbs_budget& budget_service;
    dbs_account& account_service;
    const public_key_type public_key;
    const account_object& fake;
    const account_object& alice;
    const account_object& bob;

    const int FAKE_ACCOUNT_BUDGET = 0;
    const int ALICE_ACCOUNT_BUDGET = 500;
    const int BOB_ACCOUNT_BUDGET = 1001;

    const int BUDGET_PER_BLOCK_DEFAULT = 40;
    const int BUDGET_BALANCE_DEFAULT = 200;

    const asset FUND_BUDGET_INITIAL_SUPPLY
        = asset(TEST_REWARD_INITIAL_SUPPLY.amount
                * (deip_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS - deip_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS)
                / deip_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS);
};

asset budget_service_check_fixture::allocate_all_cash_from_fund_budget_in_block()
{
    fc::time_point deadline = db.get_genesis_time() + fc::days(deip_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS);
    asset result;
    db_plugin->debug_update(
        [&](database&) {
            const budget_object& budget = budget_service.get_fund_budget();
            result = budget_service.allocate_cash(budget, deadline);
        },
        default_skip);
    return result;
}

//
// usage for all budget tests 'chain_test  -t budget_*'
//
BOOST_FIXTURE_TEST_SUITE(budget_service_check, budget_service_check_fixture)

deip_TEST_CASE(is_const_ref_to_same_memory)
{
    asset balance(BUDGET_BALANCE_DEFAULT, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    const auto& budget = budget_service.create_budget(alice, balance, deadline);

    db.modify(budget, [&](budget_object& b) { b.balance.amount -= 1; });

    BOOST_REQUIRE(budget.balance.amount == (BUDGET_BALANCE_DEFAULT - 1));
}

deip_TEST_CASE(owned_budget_creation)
{
    asset balance(BUDGET_BALANCE_DEFAULT, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    auto reqired_alice_balance = alice.balance.amount;

    const auto& budget = budget_service.create_budget(alice, balance, deadline);

    BOOST_CHECK(budget.balance.amount == BUDGET_BALANCE_DEFAULT);
    BOOST_CHECK(budget.per_block == BUDGET_PER_BLOCK_DEFAULT);

    reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

    const auto& actual_account = account_service.get_account("alice");

    BOOST_REQUIRE(actual_account.balance.amount == reqired_alice_balance);
}

deip_TEST_CASE(second_owned_budget_creation)
{
    asset balance(BUDGET_BALANCE_DEFAULT, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    auto reqired_alice_balance = alice.balance.amount;

    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));

    reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

    const auto& budget = budget_service.create_budget(alice, balance, deadline);

    BOOST_CHECK(budget.balance.amount == BUDGET_BALANCE_DEFAULT);
    BOOST_CHECK(budget.per_block == BUDGET_PER_BLOCK_DEFAULT);

    reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

    const auto& actual_account = account_service.get_account("alice");

    BOOST_REQUIRE(actual_account.balance.amount == reqired_alice_balance);
}

deip_TEST_CASE(fund_budget_creation)
{
    // fund budget is created within database(in database::init_genesis_rewards())
    // TODO: make MOC for database and test budget_service separately
    BOOST_REQUIRE_NO_THROW(budget_service.get_fund_budget());
}

deip_TEST_CASE(second_fund_budget_creation)
{
    asset balance(BUDGET_BALANCE_DEFAULT, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    BOOST_REQUIRE_THROW(budget_service.create_fund_budget(balance, deadline), fc::assert_exception);
}

deip_TEST_CASE(fund_budget_initial_supply)
{
    auto budget = budget_service.get_fund_budget();

    BOOST_REQUIRE(budget.balance == FUND_BUDGET_INITIAL_SUPPLY);
}

deip_TEST_CASE(fund_budget_initial_deadline)
{
    auto budget = budget_service.get_fund_budget();

    BOOST_REQUIRE(budget.deadline == db.get_genesis_time() + fc::days(deip_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS));
}

deip_TEST_CASE(owned_budget_creation_asserts)
{
    asset balance(BUDGET_BALANCE_DEFAULT, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    BOOST_CHECK_THROW(budget_service.create_budget(fake, balance, deadline), fc::assert_exception);

    asset wrong_currency_balance(BUDGET_BALANCE_DEFAULT, VESTS_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_budget(alice, wrong_currency_balance, deadline), fc::assert_exception);

    asset wrong_amount_balance(0, deip_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_budget(alice, wrong_amount_balance, deadline), fc::assert_exception);

    wrong_amount_balance = asset(-100, deip_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_budget(alice, wrong_amount_balance, deadline), fc::assert_exception);

    time_point_sec invalid_deadline = db.head_block_time();

    BOOST_CHECK_THROW(budget_service.create_budget(alice, balance, invalid_deadline), fc::assert_exception);

    invalid_deadline -= 1000;

    BOOST_CHECK_THROW(budget_service.create_budget(alice, balance, invalid_deadline), fc::assert_exception);

    asset too_large_balance(ALICE_ACCOUNT_BUDGET * 2, VESTS_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_budget(alice, too_large_balance, deadline), fc::assert_exception);
}

deip_TEST_CASE(budget_creation_limit)
{
    share_type bp = deip_LIMIT_BUDGETS_PER_OWNER + 1;
    BOOST_REQUIRE(BOB_ACCOUNT_BUDGET >= bp);

    asset balance(BOB_ACCOUNT_BUDGET / bp, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    for (int ci = 0; ci < deip_LIMIT_BUDGETS_PER_OWNER; ++ci)
    {
        BOOST_REQUIRE_NO_THROW(budget_service.create_budget(bob, balance, deadline));
    }

    BOOST_CHECK(bob.balance.amount == (BOB_ACCOUNT_BUDGET - deip_LIMIT_BUDGETS_PER_OWNER * balance.amount));

    BOOST_REQUIRE_THROW(budget_service.create_budget(bob, balance, deadline), fc::assert_exception);
}

deip_TEST_CASE(get_all_budgets)
{
    asset balance(BUDGET_BALANCE_DEFAULT, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    BOOST_CHECK_NO_THROW(budget_service.get_fund_budget());
    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
    BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));

    auto budgets = budget_service.get_budgets();
    BOOST_REQUIRE(budgets.size() == 3);
}

deip_TEST_CASE(get_all_budget_count)
{
    asset balance(BUDGET_BALANCE_DEFAULT, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    BOOST_CHECK_NO_THROW(budget_service.get_fund_budget());
    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
    BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));

    BOOST_REQUIRE(budget_service.get_budgets().size() == 3);
}

deip_TEST_CASE(lookup_budget_owners)
{
    asset balance(BUDGET_BALANCE_DEFAULT, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    BOOST_CHECK_NO_THROW(budget_service.get_fund_budget());
    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
    BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));
    BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));

    BOOST_REQUIRE(budget_service.get_budgets().size() == 4);

    BOOST_CHECK_THROW(budget_service.lookup_budget_owners("alice", std::numeric_limits<uint32_t>::max()),
                      fc::assert_exception);

    {
        auto owners = budget_service.lookup_budget_owners(deip_ROOT_POST_PARENT, deip_LIMIT_BUDGETS_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 3);
    }

    {
        auto owners = budget_service.lookup_budget_owners("alice", deip_LIMIT_BUDGETS_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 2);
    }

    {
        auto owners = budget_service.lookup_budget_owners("bob", deip_LIMIT_BUDGETS_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 1);
    }

    {
        auto owners = budget_service.lookup_budget_owners(deip_ROOT_POST_PARENT, 2);
        BOOST_REQUIRE(owners.size() == 2);
    }
}

deip_TEST_CASE(check_get_budgets)
{
    asset balance(BUDGET_BALANCE_DEFAULT, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 0u);

    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 1u);

    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 2u);

    for (const budget_object& budget : budget_service.get_budgets("alice"))
    {
        BOOST_REQUIRE(budget.owner == account_name_type("alice"));
    }
}

deip_TEST_CASE(check_get_budget_count)
{
    asset balance(BUDGET_BALANCE_DEFAULT, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 0u);

    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
    BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 2u);
    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("bob").size(), 1u);
}

deip_TEST_CASE(check_close_budget)
{
    asset balance(BUDGET_BALANCE_DEFAULT, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    auto reqired_alice_balance = alice.balance.amount;

    const auto& budget = budget_service.create_budget(alice, balance, deadline);

    reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

    auto budgets = budget_service.get_budgets("alice");
    BOOST_REQUIRE(!budgets.empty());

    BOOST_CHECK_NO_THROW(budget_service.close_budget(budget));

    reqired_alice_balance += BUDGET_BALANCE_DEFAULT;

    BOOST_REQUIRE(budget_service.get_budgets("alice").empty());

    const auto& actual_account = account_service.get_account("alice");

    BOOST_REQUIRE_EQUAL(reqired_alice_balance, actual_account.balance.amount);
}

deip_TEST_CASE(allocate_cash_per_block)
{
    asset balance(BUDGET_BALANCE_DEFAULT, deip_SYMBOL);
    time_point_sec deadline(default_deadline);

    const auto& budget = budget_service.create_budget(alice, balance, deadline);

    auto cash = budget_service.allocate_cash(budget);

    BOOST_REQUIRE_EQUAL(cash.amount, 0); // wait next block
}

deip_TEST_CASE(allocate_cash_from_fund_budget_per_block)
{
    const auto& budget = budget_service.get_fund_budget();

    auto cash = budget_service.allocate_cash(budget);

    BOOST_REQUIRE_GE(cash.amount,
                     FUND_BUDGET_INITIAL_SUPPLY.amount
                         / (deip_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS * deip_BLOCKS_PER_DAY));

    BOOST_REQUIRE_EQUAL(budget.balance.amount, (FUND_BUDGET_INITIAL_SUPPLY - cash).amount);
}

deip_TEST_CASE(auto_close_fund_budget_by_deadline)
{
    generate_block();

    asset total_cash = allocate_all_cash_from_fund_budget_in_block();

    BOOST_REQUIRE_THROW(budget_service.get_fund_budget(), fc::assert_exception);

    BOOST_REQUIRE_EQUAL(total_cash, FUND_BUDGET_INITIAL_SUPPLY);

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets().size(), (size_t)0);
}

// TODO rework this test
// deip_TEST_CASE(auto_close_fund_budget_by_balance)
//{
//    BOOST_REQUIRE_NO_THROW(
//        create_fund_budget_in_block(asset(BUDGET_BALANCE_DEFAULT, deip_SYMBOL), time_point_sec::maximum()));
//
//    asset total_cash(0, deip_SYMBOL);
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

deip_TEST_CASE(try_close_fund_budget)
{
    auto budget = budget_service.get_fund_budget();

    BOOST_REQUIRE_THROW(budget_service.close_budget(budget), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
