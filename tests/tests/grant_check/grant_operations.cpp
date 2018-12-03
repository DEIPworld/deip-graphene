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

class grant_operation_check_fixture : public timed_blocks_database_fixture
{
public:
    grant_operation_check_fixture()
    {
        create_grant_op.owner = "alice";        
        create_grant_op.balance = asset(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);
        create_grant_op.start_block = 0;
        create_grant_op.end_block = 1;
        create_grant_op.target_discipline = "1";
        create_grant_op.is_extendable = false;
        create_grant_op.content_hash = "hash";
    }

    create_grant_operation create_grant_op;

    const int GRANT_BALANCE_DEFAULT = 200;
    const char* GRANT_CONTENT_PERMLINK = "https://gitlab.com/DEIP/deip-blockchain/blob/master/README.md";
};

BOOST_FIXTURE_TEST_SUITE(grant_operation_check, grant_operation_check_fixture)

DEIP_TEST_CASE(create_grant_operation_check)
{
    BOOST_REQUIRE_NO_THROW(create_grant_op.validate());
}

DEIP_TEST_CASE(create_grant_operation_check_invalid_balance_amount)
{
    create_grant_op.balance = asset(0, DEIP_SYMBOL);

    BOOST_REQUIRE_THROW(create_grant_op.validate(), fc::assert_exception);

    create_grant_op.balance = asset(-GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);

    BOOST_REQUIRE_THROW(create_grant_op.validate(), fc::assert_exception);
}

// DEIP_TEST_CASE(create_grant_operation_check_invalid_balance_currency)
// {
//     create_grant_op.balance = asset(GRANT_BALANCE_DEFAULT, INCORRECT_SYMBOL);

//     BOOST_REQUIRE_THROW(create_grant_op.validate(), fc::assert_exception);
// }

DEIP_TEST_CASE(create_grant_operation_check_invalid_owner_name)
{
    create_grant_op.owner = "";

    BOOST_REQUIRE_THROW(create_grant_op.validate(), fc::assert_exception);

    create_grant_op.owner = "wrong;\n'j'";

    BOOST_REQUIRE_THROW(create_grant_op.validate(), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

class grant_transaction_check_fixture : public grant_operation_check_fixture
{
public:
    grant_transaction_check_fixture()
        : grant_service(db.obtain_service<dbs_discipline_supply>())
        , account_service(db.obtain_service<dbs_account>())
    {
    }

    dbs_discipline_supply& grant_service;
    dbs_account& account_service;

    private_key_type alice_create_grant(const asset& balance);
};

private_key_type grant_transaction_check_fixture::alice_create_grant(const asset& balance)
{
    BOOST_REQUIRE(BLOCK_LIMIT_DEFAULT > 0);

    ACTORS((alice))

    fund("alice", GRANT_BALANCE_DEFAULT);

    create_grant_operation op;
    op.owner = "alice";
    op.balance = balance;
    op.start_block = 0;
    op.end_block = 1;
    op.target_discipline = "1";    

    BOOST_REQUIRE_NO_THROW(op.validate());

    signed_transaction tx;

    tx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(op);

    BOOST_REQUIRE_NO_THROW(tx.sign(alice_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE(grant_service.get_grants("alice").size() == 1);

    return alice_private_key;
}

BOOST_FIXTURE_TEST_SUITE(grant_transaction_check, grant_transaction_check_fixture)

// DEIP_TEST_CASE(create_grant_check)
// {
//     asset balance(GRANT_BALANCE_DEFAULT, DEIP_SYMBOL);

//     BOOST_REQUIRE_NO_THROW(alice_create_grant(balance));

//     const grant_object& grant = (*grant_service.get_grants("alice").cbegin());

//     BOOST_REQUIRE(grant.owner == "alice");
//     // BOOST_REQUIRE(!grant.content_permlink.compare(GRANT_CONTENT_PERMLINK));
//     BOOST_REQUIRE(grant.balance == balance);

//     BOOST_REQUIRE_NO_THROW(validate_database());
// }

BOOST_AUTO_TEST_SUITE_END()

#endif
