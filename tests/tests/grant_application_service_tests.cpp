#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>
#include <deip/chain/util/asset.hpp>
#include <deip/chain/schema/grant_object.hpp>
#include <deip/chain/schema/grant_application_object.hpp>
#include <deip/chain/schema/grant_application_review_object.hpp>
#include <deip/chain/services/dbs_grant_application.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class grant_application_service_fixture : public clean_database_fixture
{
public:
    grant_application_service_fixture()
            : data_service(db.obtain_service<dbs_grant_application>())
    {
    }

    void create_grant_applications()
    {
        db.create<grant_application_object>([&](grant_application_object& ga_o) {
            ga_o.id = 0;
            ga_o.grant_id = 1;
            ga_o.research_id = 1;
            ga_o.creator = "alice";
            ga_o.application_hash = "test1";
        });

        db.create<grant_application_object>([&](grant_application_object& ga_o) {
            ga_o.id = 1;
            ga_o.grant_id = 1;
            ga_o.research_id = 2;
            ga_o.creator = "bob";
            ga_o.application_hash = "test2";
        });

        db.create<grant_application_object>([&](grant_application_object& ga_o) {
            ga_o.id = 2;
            ga_o.grant_id = 2;
            ga_o.research_id = 2;
            ga_o.creator = "john";
            ga_o.application_hash = "test3";
        });
    }

    dbs_grant_application& data_service;
};

BOOST_FIXTURE_TEST_SUITE(grant_application_service_tests, grant_application_service_fixture)

BOOST_AUTO_TEST_CASE(create_grant_application)
{
    try
    {
        auto& grant_application = data_service.create_grant_application(1, 1, "test", "alice");
        BOOST_CHECK(grant_application.grant_id == 1);
        BOOST_CHECK(grant_application.research_id == 1);
        BOOST_CHECK(grant_application.creator == "alice");
        BOOST_CHECK(grant_application.application_hash == "test");
        BOOST_CHECK(grant_application.created_at == db.head_block_time());
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_grant_application)
{
    try
    {
        create_grant_applications();

        auto& grant_application = data_service.get_grant_application(1);

        BOOST_CHECK(grant_application.grant_id == 1);
        BOOST_CHECK(grant_application.research_id == 2);
        BOOST_CHECK(grant_application.creator == "bob");
        BOOST_CHECK(grant_application.application_hash == "test2");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_grant_applications_by_grant)
{
    try
    {
        create_grant_applications();
        auto applications = data_service.get_grant_applications_by_grant(1);

        BOOST_CHECK(applications.size() == 2);
        BOOST_CHECK(std::any_of(applications.begin(), applications.end(), [](std::reference_wrapper<const grant_application_object> wrapper){
            const grant_application_object &application = wrapper.get();

            return application.id == 0 && application.research_id == 1 &&
                    application.grant_id == 1 &&
                    application.creator == "alice" &&
                    application.application_hash == "test1";
        }));

        BOOST_CHECK(std::any_of(applications.begin(), applications.end(), [](std::reference_wrapper<const grant_application_object> wrapper){
            const grant_application_object &application = wrapper.get();

            return application.id == 1 && application.research_id == 2 &&
                   application.grant_id == 1 &&
                   application.creator == "bob" &&
                   application.application_hash == "test2";
        }));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_grant_applications_by_research_id)
{
    try
    {
        create_grant_applications();
        auto applications = data_service.get_grant_applications_by_research_id(2);

        BOOST_CHECK(applications.size() == 2);

        BOOST_CHECK(std::any_of(applications.begin(), applications.end(), [](std::reference_wrapper<const grant_application_object> wrapper){
            const grant_application_object &application = wrapper.get();

            return application.id == 1 && application.research_id == 2 &&
                   application.grant_id == 1 &&
                   application.creator == "bob" &&
                   application.application_hash == "test2";
        }));

        BOOST_CHECK(std::any_of(applications.begin(), applications.end(), [](std::reference_wrapper<const grant_application_object> wrapper){
            const grant_application_object &application = wrapper.get();

            return application.id == 2 && application.research_id == 2 &&
                   application.grant_id == 2 &&
                   application.creator == "john" &&
                   application.application_hash == "test3";
        }));

    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(delete_grant_appication_by_id)
{
    try
    {
        create_grant_applications();
        BOOST_CHECK_NO_THROW(data_service.delete_grant_appication_by_id(1));
        BOOST_CHECK_THROW(db.get<grant_application_object>(1), std::out_of_range);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(update_grant_application_status)
{
    try
    {
        db.create<grant_object>([&](grant_object& ga) {
            ga.id = 1;
            ga.target_discipline = 1;
            ga.max_number_of_researches_to_grant = 5;
            ga.min_number_of_positive_reviews = 5;
            ga.min_number_of_applications = 10;
            ga.amount = asset(1000, DEIP_SYMBOL);
            ga.start_time = db.head_block_time();
            ga.end_time = db.head_block_time() + DAYS_TO_SECONDS(30);
            ga.owner = "bob";
            std::set<account_name_type> officers;
            officers.insert("alice");
            ga.officers.insert(officers.begin(), officers.end());
        });

        create_grant_applications();
        auto& ga = data_service.get_grant_application(1);
        data_service.update_grant_application_status(ga, grant_application_status::application_approved);

        BOOST_CHECK(ga.status == grant_application_status::application_approved);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif