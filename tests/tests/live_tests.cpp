#include <boost/test/unit_test.hpp>

#include <deip/protocol/exceptions.hpp>

#include <deip/chain/database.hpp>
#include <deip/chain/hardfork.hpp>
#include <deip/chain/deip_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "database_fixture.hpp"

#include <iostream>

using namespace deip;
using namespace deip::chain;
using namespace deip::protocol;

#ifndef IS_TEST_NET

BOOST_FIXTURE_TEST_SUITE(live_tests, live_database_fixture)

/*BOOST_AUTO_TEST_CASE(retally_votes)
{
    try
    {
        flat_map<witness_id_type, share_type> expected_votes;

        const auto& by_account_witness_idx = db.get_index<witness_vote_index>().indices();

        for (auto vote : by_account_witness_idx)
        {
            if (expected_votes.find(vote.witness) == expected_votes.end())
                expected_votes[vote.witness] = db.get(vote.account).witness_vote_weight();
            else
                expected_votes[vote.witness] += db.get(vote.account).witness_vote_weight();
        }

        db.retally_witness_votes();

        const auto& witness_idx = db.get_index<witness_index>().indices();

        for (auto witness : witness_idx)
        {
            BOOST_REQUIRE_EQUAL(witness.votes.value, expected_votes[witness.id].value);
        }
    }
    FC_LOG_AND_RETHROW()
}*/

BOOST_AUTO_TEST_SUITE_END()

#endif
