#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/database.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class database_unit_service_fixture : public clean_database_fixture
{
public:
    database_unit_service_fixture()
            : account_service(db.obtain_service<dbs_account>()),
              vote_service(db.obtain_service<dbs_vote>())
    {
    }

    void create_votes()
    {
        db.create<vote_object>([&](vote_object& d) {
            d.id = 1;
            d.discipline_id = 1;
            d.voter = "alice";
            d.research_id = 1;
            d.research_content_id = 1;
            d.weight = 10;
        });

        db.create<vote_object>([&](vote_object& d) {
            d.id = 2;
            d.discipline_id = 1;
            d.voter = "bob";
            d.research_id = 1;
            d.research_content_id = 1;
            d.weight = 20;
        });

        db.create<vote_object>([&](vote_object& d) {
            d.id = 3;
            d.discipline_id = 1;
            d.voter = "john";
            d.research_id = 1;
            d.research_content_id = 1;
            d.weight = 30;
        });
    }

    void create_total_votes()
    {
        db.create<total_votes_object>([&](total_votes_object& d) {
            d.id = 1;
            d.discipline_id = 1;
            d.research_id = 1;
            d.research_content_id = 1;
            d.total_curators_reward_weight = 60;
        });
    }

    dbs_account& account_service;
    dbs_vote& vote_service;
};

BOOST_FIXTURE_TEST_SUITE(database_unit_service, database_unit_service_fixture)

BOOST_AUTO_TEST_CASE(reward_voters)
{
    try
    {
        ACTORS((alice)(bob)(john));
        fund("alice", 100);
        fund("bob", 100);
        fund("john", 100);

        create_votes();
        create_total_votes();

        BOOST_CHECK_NO_THROW(db.reward_voters(1, 1, 60));

        auto& alice_account = db.get_account("alice");
        auto& bob_account = db.get_account("bob");
        auto& john_account = db.get_account("john");

        BOOST_CHECK(alice_account.balance.amount == 110);
        BOOST_CHECK(bob_account.balance.amount == 120);
        BOOST_CHECK(john_account.balance.amount == 130);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
