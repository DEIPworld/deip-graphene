#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/research_token_sale_contribution_object.hpp>
#include <deip/chain/dbs_research_token_sale_contribution.hpp>


#include "database_fixture.hpp"

namespace deip {
namespace chain {

class research_token_sale_contribution_service_fixture : public clean_database_fixture
{
public:
    research_token_sale_contribution_service_fixture()
            : data_service(db.obtain_service<dbs_research_token_sale_contribution>())
    {
    }

    void create_research_token_sale_contributions()
    {
        db.create<research_token_sale_contribution_object>([&](research_token_sale_contribution_object& d) {
            d.id = 1;
            d.owner = "alice";
            d.amount = 100;
            d.research_token_sale_id = 1;
        });

        db.create<research_token_sale_contribution_object>([&](research_token_sale_contribution_object& d) {
            d.id = 2;
            d.owner = "bob";
            d.amount = 200;
            d.research_token_sale_id = 1;
        });

        db.create<research_token_sale_contribution_object>([&](research_token_sale_contribution_object& d) {
            d.id = 3;
            d.owner = "bob";
            d.amount = 300;
            d.research_token_sale_id = 2;
        });
    }

    dbs_research_token_sale_contribution& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_token_sale_contribution_service_tests, research_token_sale_contribution_service_fixture)

BOOST_AUTO_TEST_CASE(create_research_token_sale_contribution)
{
    try
    {
        auto& research_token_sale_contribution =
                data_service.create_research_token_sale_contributiont(1, "alice", fc::time_point_sec(0xffffffff), 100);

        BOOST_CHECK(research_token_sale_contribution.research_token_sale_id == 1);
        BOOST_CHECK(research_token_sale_contribution.owner == "alice");
        BOOST_CHECK(research_token_sale_contribution.contribution_time == fc::time_point_sec(0xffffffff));
        BOOST_CHECK(research_token_sale_contribution.amount == 100);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif