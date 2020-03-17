#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>
#include <deip/chain/services/dbs_grant.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class grant_service_fixture : public clean_database_fixture
{
public:
    grant_service_fixture()
            : data_service(db.obtain_service<dbs_grant>())
    {
    }

    void create_grants()
    {
        db.create<grant_object>([&](grant_object& ga) {
            ga.id = 0;
            ga.max_number_of_research_to_grant = 5;
            ga.min_number_of_positive_reviews = 5;
            ga.min_number_of_applications = 10;
            ga.amount = asset(1000, DEIP_SYMBOL);
            ga.start_date = db.head_block_time();
            ga.end_date  = db.head_block_time() + DAYS_TO_SECONDS(30);
            ga.grantor = "bob";
        });

        db.create<grant_object>([&](grant_object& ga) {
            ga.id = 1;
            ga.max_number_of_research_to_grant = 6;
            ga.min_number_of_positive_reviews = 4;
            ga.min_number_of_applications = 10;
            ga.amount = asset(1000, DEIP_SYMBOL);
            ga.start_date = db.head_block_time();
            ga.end_date = db.head_block_time() + DAYS_TO_SECONDS(30);
            ga.grantor = "jack";
        });

        db.create<grant_object>([&](grant_object& ga) {
            ga.id = 2;
            ga.max_number_of_research_to_grant = 4;
            ga.min_number_of_positive_reviews = 10;
            ga.min_number_of_applications = 15;
            ga.amount = asset(1000, DEIP_SYMBOL);
            ga.start_date = db.head_block_time();
            ga.end_date = db.head_block_time() + DAYS_TO_SECONDS(30);
            ga.grantor = "rick";
        });
    }

    dbs_grant& data_service;
};

BOOST_FIXTURE_TEST_SUITE(grant_service_tests, grant_service_fixture)

BOOST_AUTO_TEST_CASE(create_grant)
{
    try
    {
        std::set<discipline_id_type> target_disciplines = {1};
        auto& grant = data_service.create_grant_with_announced_application_window(
          "alice", 
          asset(100, DEIP_SYMBOL), 
          target_disciplines,
          1,
          5, 
          10, 
          10,
          db.head_block_time(), 
          db.head_block_time() + DAYS_TO_SECONDS(10)
        );

        BOOST_CHECK(grant.amount ==  asset(100, DEIP_SYMBOL));
        BOOST_CHECK(grant.min_number_of_positive_reviews == 5);
        BOOST_CHECK(grant.min_number_of_applications == 10);
        BOOST_CHECK(grant.max_number_of_research_to_grant == 10);
        BOOST_CHECK(grant.created_at == db.head_block_time());
        BOOST_CHECK(grant.start_date == db.head_block_time());
        BOOST_CHECK(grant.end_date == db.head_block_time() + DAYS_TO_SECONDS(10));
        BOOST_CHECK(grant.grantor == "alice");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_grant)
{
    try
    {
        create_grants();

        auto& grant = data_service.get_grant_with_announced_application_window(1);

        BOOST_CHECK(grant.id == 1);
        BOOST_CHECK(grant.amount ==  asset(1000, DEIP_SYMBOL));
        BOOST_CHECK(grant.min_number_of_positive_reviews == 4);
        BOOST_CHECK(grant.min_number_of_applications == 10);
        BOOST_CHECK(grant.max_number_of_research_to_grant == 6);
        BOOST_CHECK(grant.start_date == db.head_block_time());
        BOOST_CHECK(grant.end_date == db.head_block_time() + DAYS_TO_SECONDS(30));
        BOOST_CHECK(grant.grantor == "jack");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(grant_with_announced_application_window_exists)
{
    try
    {
        create_grants();

        BOOST_CHECK(data_service.grant_with_announced_application_window_exists(1));
        BOOST_CHECK(!data_service.grant_with_announced_application_window_exists(10));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif