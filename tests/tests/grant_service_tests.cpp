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
            ga.target_discipline = 1;
            ga.max_researches_to_grant = 5;
            ga.min_number_of_positive_reviews = 5;
            ga.min_number_of_applications = 10;
            ga.amount = asset(1000, DEIP_SYMBOL);
            ga.start_time = db.head_block_time();
            ga.end_time = db.head_block_time() + DAYS_TO_SECONDS(30);
            ga.owner = "bob";
        });

        db.create<grant_object>([&](grant_object& ga) {
            ga.id = 1;
            ga.target_discipline = 1;
            ga.max_researches_to_grant = 6;
            ga.min_number_of_positive_reviews = 4;
            ga.min_number_of_applications = 10;
            ga.amount = asset(1000, DEIP_SYMBOL);
            ga.start_time = db.head_block_time();
            ga.end_time = db.head_block_time() + DAYS_TO_SECONDS(30);
            ga.owner = "jack";
        });

        db.create<grant_object>([&](grant_object& ga) {
            ga.id = 2;
            ga.target_discipline = 2;
            ga.max_researches_to_grant = 4;
            ga.min_number_of_positive_reviews = 10;
            ga.min_number_of_applications = 15;
            ga.amount = asset(1000, DEIP_SYMBOL);
            ga.start_time = db.head_block_time();
            ga.end_time = db.head_block_time() + DAYS_TO_SECONDS(30);
            ga.owner = "rick";
        });
    }

    dbs_grant& data_service;
};

BOOST_FIXTURE_TEST_SUITE(grant_service_tests, grant_service_fixture)

BOOST_AUTO_TEST_CASE(create_grant)
{
    try
    {
        auto& grant = data_service.create(1, asset(100, DEIP_SYMBOL), 5, 10, 10, db.head_block_time(), db.head_block_time() + DAYS_TO_SECONDS(10), "alice");

        BOOST_CHECK(grant.target_discipline == 1);
        BOOST_CHECK(grant.amount ==  asset(100, DEIP_SYMBOL));
        BOOST_CHECK(grant.min_number_of_positive_reviews == 5);
        BOOST_CHECK(grant.min_number_of_applications == 10);
        BOOST_CHECK(grant.max_researches_to_grant == 10);
        BOOST_CHECK(grant.created_at == db.head_block_time());
        BOOST_CHECK(grant.start_time == db.head_block_time());
        BOOST_CHECK(grant.end_time == db.head_block_time() + DAYS_TO_SECONDS(10));
        BOOST_CHECK(grant.owner == "alice");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_grant)
{
    try
    {
        create_grants();

        auto& grant = data_service.get(1);

        BOOST_CHECK(grant.id == 1);
        BOOST_CHECK(grant.target_discipline == 1);
        BOOST_CHECK(grant.amount ==  asset(1000, DEIP_SYMBOL));
        BOOST_CHECK(grant.min_number_of_positive_reviews == 4);
        BOOST_CHECK(grant.min_number_of_applications == 10);
        BOOST_CHECK(grant.max_researches_to_grant == 6);
        BOOST_CHECK(grant.start_time == db.head_block_time());
        BOOST_CHECK(grant.end_time == db.head_block_time() + DAYS_TO_SECONDS(30));
        BOOST_CHECK(grant.owner == "jack");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_grant_existence)
{
    try
    {
        create_grants();

        BOOST_CHECK_NO_THROW(data_service.check_grant_existence(1));
        BOOST_CHECK_THROW(data_service.check_grant_existence(10), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_by_target_discipline)
{
    try
    {
        create_grants();
        auto grants = data_service.get_by_target_discipline(1);

        BOOST_CHECK(grants.size() == 2);
        BOOST_CHECK(std::any_of(grants.begin(), grants.end(), [this](std::reference_wrapper<const grant_object> wrapper){
            const grant_object &grant = wrapper.get();

            return grant.id == 0 && grant.target_discipline == 1 &&
                    grant.max_researches_to_grant == 5 &&
                    grant.min_number_of_positive_reviews == 5 &&
                    grant.min_number_of_applications == 10 &&
                    grant.amount == asset(1000, DEIP_SYMBOL) &&
                    grant.start_time == db.head_block_time() &&
                    grant.end_time == db.head_block_time() + DAYS_TO_SECONDS(30) &&
                    grant.owner == "bob";
        }));

        BOOST_CHECK(std::any_of(grants.begin(), grants.end(), [this](std::reference_wrapper<const grant_object> wrapper){
            const grant_object &grant = wrapper.get();

            return grant.id == 1 && grant.target_discipline == 1 &&
                   grant.max_researches_to_grant == 6 &&
                   grant.min_number_of_positive_reviews == 4 &&
                   grant.min_number_of_applications == 10 &&
                   grant.amount == asset(1000, DEIP_SYMBOL) &&
                   grant.start_time == db.head_block_time() &&
                   grant.end_time == db.head_block_time() + DAYS_TO_SECONDS(30) &&
                   grant.owner == "jack";
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(delete_grant)
{
    try
    {
        ACTORS((jack))
        create_grants();

        auto& grant = db.get<grant_object>(1);
        BOOST_CHECK_NO_THROW(data_service.delete_grant(grant));
        BOOST_CHECK_THROW(db.get<grant_object>(1), std::out_of_range);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif