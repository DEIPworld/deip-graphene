#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/protocol/exceptions.hpp>

#include <deip/chain/schema/block_summary_object.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/hardfork.hpp>
#include <deip/blockchain_history/account_history_object.hpp>
#include <deip/chain/schema/deip_objects.hpp>

#include <deip/chain/util/reward.hpp>

#include <deip/plugins/debug_node/debug_node_plugin.hpp>

#include <fc/crypto/digest.hpp>

#include "database_fixture.hpp"

#include <cmath>

using namespace deip;
using namespace deip::chain;
using namespace deip::protocol;

BOOST_FIXTURE_TEST_SUITE(operation_time_tests, clean_database_fixture)

BOOST_AUTO_TEST_SUITE_END()
#endif
