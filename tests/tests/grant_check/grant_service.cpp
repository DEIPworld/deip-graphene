#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/dbs_account.hpp>
#include <deip/chain/dbs_grant.hpp>

#include "database_fixture.hpp"

#include <limits>

using namespace deip;
using namespace deip::chain;
using namespace deip::protocol;
using fc::string;

//
// usage for all grant tests 'chain_test  -t grant_*'
//

class grant_service_check_fixture : public timed_blocks_database_fixture
{
public:
    grant_service_check_fixture()
        : grant_service(db.obtain_service<dbs_grant>())
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
        account_service.increase_balance(alice, asset(ALICE_ACCOUNT_GRANT, DEIP_SYMBOL));
        account_service.increase_balance(bob, asset(BOB_ACCOUNT_GRANT, DEIP_SYMBOL));
    }

    dbs_grant& grant_service;
    dbs_account& account_service;
    const public_key_type public_key;
    const account_object& fake;
    const account_object& alice;
    const account_object& bob;

    const int FAKE_ACCOUNT_GRANT = 0;
    const int ALICE_ACCOUNT_GRANT = 500;
    const int BOB_ACCOUNT_GRANT = 1001;

    const int START_BLOCK = int(db.head_block_num());
    const int END_BLOCK = START_BLOCK + 20;

    const int GRANT_BALANCE_DEFAULT = 200;
    const int GRANT_PER_BLOCK_DEFAULT = GRANT_BALANCE_DEFAULT / (END_BLOCK - START_BLOCK);

    const discipline_id_type TARGET_DISCIPLINE = 1;

    const asset GRANT_BALANCE = asset(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);
    const asset FUND_GRANT_INITIAL_SUPPLY
        = asset(TEST_REWARD_INITIAL_SUPPLY.amount
                * (DEIP_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS - DEIP_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS)
                / DEIP_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS);
};

//
// usage for all grant tests 'chain_test  -t grant_*'
//

BOOST_FIXTURE_TEST_SUITE(grant_service_check, grant_service_check_fixture)

DEIP_TEST_CASE(is_const_ref_to_same_memory)
{
    asset balance(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);

    const auto& grant = grant_service.create_grant(alice, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE);

    db.modify(grant, [&](grant_object& b) { b.balance.amount -= 1; });

    BOOST_REQUIRE(grant.balance.amount == (GRANT_BALANCE_DEFAULT - 1));
}

DEIP_TEST_CASE(owned_grant_creation)
{
    asset balance(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);

    auto reqired_alice_balance = alice.balance.amount;

    const auto& grant = grant_service.create_grant(alice, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE);

    BOOST_CHECK(grant.balance.amount == GRANT_BALANCE_DEFAULT);
    BOOST_CHECK(grant.per_block == GRANT_PER_BLOCK_DEFAULT);

    reqired_alice_balance -= GRANT_BALANCE_DEFAULT;

    const auto& actual_account = account_service.get_account("alice");

    BOOST_REQUIRE(actual_account.balance.amount == reqired_alice_balance);
}

DEIP_TEST_CASE(second_owned_grant_creation)
{
    asset balance(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);

    auto reqired_alice_balance = alice.balance.amount;

    BOOST_CHECK_NO_THROW(grant_service.create_grant(alice, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    reqired_alice_balance -= GRANT_BALANCE_DEFAULT;

    const auto& grant = grant_service.create_grant(alice, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE);

    BOOST_CHECK(grant.balance.amount == GRANT_BALANCE_DEFAULT);
    BOOST_CHECK(grant.per_block == GRANT_PER_BLOCK_DEFAULT);

    reqired_alice_balance -= GRANT_BALANCE_DEFAULT;

    const auto& actual_account = account_service.get_account("alice");

    BOOST_REQUIRE(actual_account.balance.amount == reqired_alice_balance);
}

DEIP_TEST_CASE(owned_grant_creation_asserts)
{
    asset balance(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_CHECK_THROW(grant_service.create_grant(fake, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);

    asset wrong_currency_balance(GRANT_BALANCE_DEFAULT, VESTS_SYMBOL);

    BOOST_CHECK_THROW(grant_service.create_grant(alice, wrong_currency_balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);

    asset wrong_amount_balance(0, DEIP_SYMBOL);

    BOOST_CHECK_THROW(grant_service.create_grant(alice, wrong_amount_balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);

    wrong_amount_balance = asset(-100, DEIP_SYMBOL);

    BOOST_CHECK_THROW(grant_service.create_grant(alice, wrong_amount_balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);

    asset too_large_balance(ALICE_ACCOUNT_GRANT * 2, VESTS_SYMBOL);

    BOOST_CHECK_THROW(grant_service.create_grant(alice, too_large_balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);
}

DEIP_TEST_CASE(grant_creation_limit)
{
    share_type bp = DEIP_LIMIT_GRANTS_PER_OWNER + 1;
    BOOST_REQUIRE(BOB_ACCOUNT_GRANT >= bp);

    asset balance(BOB_ACCOUNT_GRANT / bp, DEIP_SYMBOL);

    for (int ci = 0; ci < DEIP_LIMIT_GRANTS_PER_OWNER; ++ci)
    {
        BOOST_REQUIRE_NO_THROW(grant_service.create_grant(bob, balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    }

    BOOST_CHECK(bob.balance.amount == (BOB_ACCOUNT_GRANT - DEIP_LIMIT_GRANTS_PER_OWNER * balance.amount));

    BOOST_REQUIRE_THROW(grant_service.create_grant(bob, balance, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE), fc::assert_exception);
}

DEIP_TEST_CASE(get_all_grants)
{
    asset balance(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_CHECK_NO_THROW(grant_service.create_grant(alice, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    BOOST_CHECK_NO_THROW(grant_service.create_grant(bob, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    auto grants = grant_service.get_grants();
    BOOST_REQUIRE(grants.size() == 2);
}

DEIP_TEST_CASE(get_all_grant_count)
{
    asset balance(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_CHECK_NO_THROW(grant_service.create_grant(alice, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    BOOST_CHECK_NO_THROW(grant_service.create_grant(bob, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    BOOST_REQUIRE(grant_service.get_grants().size() == 2);
}

DEIP_TEST_CASE(lookup_grant_owners)
{
    asset balance(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_CHECK_NO_THROW(grant_service.create_grant(alice, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    BOOST_CHECK_NO_THROW(grant_service.create_grant(bob, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    BOOST_CHECK_NO_THROW(grant_service.create_grant(bob, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    BOOST_REQUIRE(grant_service.get_grants().size() == 3);

    BOOST_CHECK_THROW(grant_service.lookup_grant_owners("alice", std::numeric_limits<uint32_t>::max()),
                      fc::assert_exception);

    {
        auto owners = grant_service.lookup_grant_owners(DEIP_ROOT_POST_PARENT, DEIP_LIMIT_GRANTS_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 2);
    }

    {
        auto owners = grant_service.lookup_grant_owners("alice", DEIP_LIMIT_GRANTS_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 2);
    }

    {
        auto owners = grant_service.lookup_grant_owners("bob", DEIP_LIMIT_GRANTS_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 1);
    }

    {
        auto owners = grant_service.lookup_grant_owners(DEIP_ROOT_POST_PARENT, 2);
        BOOST_REQUIRE(owners.size() == 2);
    }
}

DEIP_TEST_CASE(check_get_grants)
{
    asset balance(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_REQUIRE_EQUAL(grant_service.get_grants("alice").size(), 0u);

    BOOST_CHECK_NO_THROW(grant_service.create_grant(alice, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    BOOST_REQUIRE_EQUAL(grant_service.get_grants("alice").size(), 1u);

    BOOST_CHECK_NO_THROW(grant_service.create_grant(alice, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    BOOST_REQUIRE_EQUAL(grant_service.get_grants("alice").size(), 2u);

    for (const grant_object& grant : grant_service.get_grants("alice"))
    {
        BOOST_REQUIRE(grant.owner == account_name_type("alice"));
    }
}

DEIP_TEST_CASE(check_get_grant_count)
{
    asset balance(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_REQUIRE_EQUAL(grant_service.get_grants("alice").size(), 0u);

    BOOST_CHECK_NO_THROW(grant_service.create_grant(alice, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    BOOST_CHECK_NO_THROW(grant_service.create_grant(alice, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));
    BOOST_CHECK_NO_THROW(grant_service.create_grant(bob, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE));

    BOOST_REQUIRE_EQUAL(grant_service.get_grants("alice").size(), 2u);
    BOOST_REQUIRE_EQUAL(grant_service.get_grants("bob").size(), 1u);
}

DEIP_TEST_CASE(allocate_cash_per_block)
{
    asset balance(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);

    const auto& grant = grant_service.create_grant(alice, GRANT_BALANCE, START_BLOCK, END_BLOCK, TARGET_DISCIPLINE);

    auto cash = grant_service.allocate_funds(grant);

    BOOST_REQUIRE_EQUAL(cash.amount, 200); // wait next block
}

// TODO rework this test
// DEIP_TEST_CASE(auto_close_fund_grant_by_balance)
//{
//    BOOST_REQUIRE_NO_THROW(
//        create_fund_grant_in_block(asset(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL), time_point_sec::maximum()));
//
//    asset total_cash(0, DEIP_SYMBOL);
//
//    for (int ci = 0; true; ++ci)
//    {
//        BOOST_REQUIRE(ci <= GRANT_BALANCE_DEFAULT);
//
//        generate_block();
//        generate_block();
//
//        total_cash += allocate_cash_from_fund_grant_in_block();
//
//        if (!grant_service.get_fund_grant_count())
//        {
//            break; // grant has closed, break and check result
//        }
//    }
//
//    BOOST_REQUIRE_THROW(grant_service.get_fund_grants(), fc::assert_exception);
//
//    BOOST_REQUIRE(total_cash.amount == GRANT_BALANCE_DEFAULT);
//}

BOOST_AUTO_TEST_SUITE_END()

#endif