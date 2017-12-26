#include <boost/test/unit_test.hpp>

#include <fc/io/json.hpp>
#include <deip/chain/genesis_state.hpp>

namespace sc = deip::chain;
namespace sp = deip::protocol;

BOOST_AUTO_TEST_SUITE(deserialize_genesis_state)

BOOST_AUTO_TEST_CASE(check_accounts_fields)
{
    const std::string genesis_str = "{\"accounts\":[{"
                                    "\"name\":\"user\","
                                    "\"recovery_account\":\"admin\","
                                    "\"public_key\":\"DEIP1111111111111111111111111111111114T1Anm\","
                                    "\"deip_amount\":1000,"
                                    "\"sp_amount\":1000000"
                                    "}]}";

    const sc::genesis_state_type genesis_state = fc::json::from_string(genesis_str).as<sc::genesis_state_type>();

    BOOST_REQUIRE(genesis_state.accounts.size() == 1);

    sc::genesis_state_type::account_type account = genesis_state.accounts.front();

    BOOST_CHECK(account.name == "user");
    BOOST_CHECK(account.public_key == sp::public_key_type("DEIP1111111111111111111111111111111114T1Anm"));
    BOOST_CHECK(account.deip_amount == 1000);
    BOOST_CHECK(account.sp_amount == 1000000);
    BOOST_CHECK(account.recovery_account == "admin");
}

BOOST_AUTO_TEST_CASE(check_witness_fields)
{
    const std::string genesis_str = "{\"witness_candidates\":[{"
                                    "\"owner_name\":\"user\","
                                    "\"block_signing_key\":\"DEIP1111111111111111111111111111111114T1Anm\""
                                    "}]}";

    const sc::genesis_state_type genesis_state = fc::json::from_string(genesis_str).as<sc::genesis_state_type>();

    BOOST_REQUIRE(genesis_state.witness_candidates.size() == 1);

    sc::genesis_state_type::witness_type w = genesis_state.witness_candidates.front();

    BOOST_CHECK(w.owner_name == "user");
    BOOST_CHECK(w.block_signing_key == sp::public_key_type("DEIP1111111111111111111111111111111114T1Anm"));
}

BOOST_AUTO_TEST_CASE(check_initial_timestamp)
{
    const std::string genesis_str = "{\"initial_timestamp\": \"2017-11-28T14:48:10\"}";

    const sc::genesis_state_type genesis_state = fc::json::from_string(genesis_str).as<sc::genesis_state_type>();

    BOOST_CHECK(genesis_state.initial_timestamp == fc::time_point_sec(1511880490));
}

BOOST_AUTO_TEST_CASE(check_initial_supply)
{
    std::string genesis_str = "{\"init_supply\": 1000000}";

    sc::genesis_state_type genesis_state = fc::json::from_string(genesis_str).as<sc::genesis_state_type>();

    BOOST_CHECK(genesis_state.init_supply == 1000000);
}

BOOST_AUTO_TEST_CASE(check_discipline_fields)
{
    const std::string genesis_str = "{\"disciplines\":[{"
                                    "\"id\":1,
                                    "\"name\":\"physics\","
                                    "\"parent_id\":NULL,"
                                    "\"votes_in_last_ten_weeks\":100"
                                    "}]}";

    const sc::genesis_state_type genesis_state = fc::json::from_string(genesis_str).as<sc::genesis_state_type>();

    BOOST_REQUIRE(genesis_state.disciplines.size() == 1);

    sc::genesis_state_type::discipline_type discipline = genesis_state.disciplines.front();

    BOOST_CHECK(discipline.name == "physics");
    BOOST_CHECK(discipline.parent_id == NULL);
    BOOST_CHECK(discipline.votes_in_last_ten_weeks == 100);
}

BOOST_AUTO_TEST_SUITE_END()
