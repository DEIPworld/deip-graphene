#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/deip_objects.hpp>

#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_discipline_supply.hpp>

#include "database_fixture.hpp"

#include <limits>

using namespace deip;
using namespace deip::chain;
using namespace deip::protocol;
using fc::string;

//
// usage for all grant tests 'chain_test  -t grant_*'
//

class discipline_supply_operation_check_fixture : public timed_blocks_database_fixture
{
public:
    discipline_supply_operation_check_fixture()
    {
//        create_discipline_supply_op.owner = "alice";
//        create_discipline_supply_op.balance = asset(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);
//        create_discipline_supply_op.start_block = 0;
//        create_discipline_supply_op.end_block = 1;
//        create_discipline_supply_op.target_discipline = "1";
//        create_discipline_supply_op.is_extendable = false;
//        create_discipline_supply_op.content_hash = "hash";
    }

    create_grant_operation create_discipline_supply_op;

    const int DISCIPLINE_SUPPLY_BALANCE_DEFAULT = 200;
    const char* DISCIPLINE_SUPPLY_CONTENT_PERMLINK = "https://gitlab.com/DEIP/deip-blockchain/blob/master/README.md";
};

BOOST_FIXTURE_TEST_SUITE(grant_operation_check, discipline_supply_operation_check_fixture)

DEIP_TEST_CASE(create_grant_operation_check)
{
    BOOST_REQUIRE_NO_THROW(create_discipline_supply_op.validate());
}

DEIP_TEST_CASE(create_grant_operation_check_invalid_balance_amount)
{
//    create_discipline_supply_op.balance = asset(0, DEIP_SYMBOL);
//
//    BOOST_REQUIRE_THROW(create_discipline_supply_op.validate(), fc::assert_exception);
//
//    create_discipline_supply_op.balance = asset(-DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);
//
//    BOOST_REQUIRE_THROW(create_discipline_supply_op.validate(), fc::assert_exception);
}

// DEIP_TEST_CASE(create_grant_operation_check_invalid_balance_currency)
// {
//     create_grant_op.balance = asset(GRANT_BALANCE_DEFAULT, INCORRECT_SYMBOL);

//     BOOST_REQUIRE_THROW(create_grant_op.validate(), fc::assert_exception);
// }

DEIP_TEST_CASE(create_grant_operation_check_invalid_owner_name)
{
//    create_discipline_supply_op.owner = "";
//
//    BOOST_REQUIRE_THROW(create_discipline_supply_op.validate(), fc::assert_exception);
//
//    create_discipline_supply_op.owner = "wrong;\n'j'";
//
//    BOOST_REQUIRE_THROW(create_discipline_supply_op.validate(), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

class discipline_supply_transaction_check_fixture : public discipline_supply_operation_check_fixture
{
public:
    discipline_supply_transaction_check_fixture()
        : discipline_supply_service(db.obtain_service<dbs_discipline_supply>())
        , account_service(db.obtain_service<dbs_account>())
    {
    }

    dbs_discipline_supply& discipline_supply_service;
    dbs_account& account_service;

    private_key_type alice_create_discipline_supply(const asset& balance);
};

private_key_type discipline_supply_transaction_check_fixture::alice_create_discipline_supply(const asset& balance)
{
//    BOOST_REQUIRE(BLOCK_LIMIT_DEFAULT > 0);
//
   ACTORS((alice))
//
//    fund("alice", DISCIPLINE_SUPPLY_BALANCE_DEFAULT);
//
//    create_discipline_supply_operation op;
//    op.owner = "alice";
//    op.balance = balance;
//    op.start_block = 0;
//    op.end_block = 1;
//    op.target_discipline = "1";
//
//    BOOST_REQUIRE_NO_THROW(op.validate());
//
//    signed_transaction tx;
//
//    tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
//    tx.operations.push_back(op);
//
//    BOOST_REQUIRE_NO_THROW(tx.sign(alice_private_key, db.get_chain_id()));
//    BOOST_REQUIRE_NO_THROW(tx.validate());
//
//    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));
//
//    BOOST_REQUIRE(discipline_supply_service.get_discipline_supplies_by_owner("alice").size() == 1);
//
   return alice_private_key;
}

BOOST_FIXTURE_TEST_SUITE(grant_transaction_check, discipline_supply_transaction_check_fixture)

// DEIP_TEST_CASE(create_discipline_supply_check)
// {
//     asset balance(DISCIPLINE_SUPPLY_BALANCE_DEFAULT, DEIP_SYMBOL);

//     BOOST_REQUIRE_NO_THROW(alice_create_discipline_supply(balance));

//     const discipline_supply_object& discipline_supply = (*grant_service.get_discipline_supplies_by_owner("alice").cbegin());

//     BOOST_REQUIRE(discipline_supply.owner == "alice");
//     // BOOST_REQUIRE(!discipline_supply.content_permlink.compare(GRANT_CONTENT_PERMLINK));
//     BOOST_REQUIRE(discipline_supply.balance == balance);

//     BOOST_REQUIRE_NO_THROW(validate_database());
// }

BOOST_AUTO_TEST_SUITE_END()

#endif
