#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/research_group_join_request_object.hpp>
#include <deip/chain/dbs_research_group_join_request.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class research_group_join_request_service_fixture : public clean_database_fixture
{
public:
    research_group_join_request_service_fixture()
            : data_service(db.obtain_service<dbs_research_group_join_request>())
    {
    }

    void create_research_join_request_objects() {
        db.create<research_group_join_request_object>([&](research_group_join_request_object& rgir_o) {
            rgir_o.id = 1;
            rgir_o.account_name = "alice";
            rgir_o.research_group_id = 1;
            rgir_o.motivation_letter = "test";
            rgir_o.expiration_time = fc::time_point_sec(0xffffffff);
        });

        db.create<research_group_join_request_object>([&](research_group_join_request_object& rgir_o) {
            rgir_o.id = 2;
            rgir_o.account_name = "bob";
            rgir_o.research_group_id = 1;
            rgir_o.motivation_letter = "test";
            rgir_o.expiration_time = fc::time_point_sec(123);
        });

        db.create<research_group_join_request_object>([&](research_group_join_request_object& rgir_o) {
            rgir_o.id = 3;
            rgir_o.account_name = "alice";
            rgir_o.research_group_id = 2;
            rgir_o.motivation_letter = "test";
            rgir_o.expiration_time = fc::time_point_sec(0xffffff1f);
        });
    }

    dbs_research_group_join_request& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_group_join_request_service, research_group_join_request_service_fixture)

BOOST_AUTO_TEST_CASE(create_research_join_request_object)
{
    try
    {
        auto& research_group_join_request = data_service.create("bob", 1, "test");

        BOOST_CHECK(research_group_join_request.account_name == "bob");
        BOOST_CHECK(research_group_join_request.research_group_id == 1);
        BOOST_CHECK(research_group_join_request.motivation_letter == "test");
        BOOST_CHECK(research_group_join_request.expiration_time == db.head_block_time() + DAYS_TO_SECONDS(14));

        BOOST_CHECK_THROW(data_service.create("bob", 1, "test2"), boost::exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_join_request_object)
{
    try
    {
        create_research_join_request_objects();

        auto& research_group_join_request = data_service.get(3);

        BOOST_CHECK(research_group_join_request.account_name == "alice");
        BOOST_CHECK(research_group_join_request.research_group_id == 2);
        BOOST_CHECK(research_group_join_request.motivation_letter == "test");
        BOOST_CHECK(research_group_join_request.expiration_time == fc::time_point_sec(0xffffff1f));

        BOOST_CHECK_THROW(data_service.get(4), boost::exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_join_request_by_account_name_and_research_group_id)
{
    try
    {
        create_research_join_request_objects();

        auto& research_group_join_request = data_service.get_research_group_join_request_by_account_name_and_research_group_id("alice", 1);

        BOOST_CHECK(research_group_join_request.account_name == "alice");
        BOOST_CHECK(research_group_join_request.research_group_id == 1);
        BOOST_CHECK(research_group_join_request.motivation_letter == "test");
        BOOST_CHECK(research_group_join_request.expiration_time == fc::time_point_sec(0xffffffff));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_research_group_join_request_existence)
{
    try
    {
        create_research_join_request_objects();

        BOOST_CHECK_NO_THROW(data_service.check_research_group_join_request_existence(1));
        BOOST_CHECK_THROW(data_service.check_research_group_join_request_existence(4), fc::assert_exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_join_requests_by_account_name)
{
    try
    {
        create_research_join_request_objects();

        auto research_group_join_requests = data_service.get_research_group_join_requests_by_account_name("alice");

        BOOST_CHECK(std::any_of(research_group_join_requests.begin(), research_group_join_requests.end(),
                                [](std::reference_wrapper<const research_group_join_request_object> wrapper) {
                                    const research_group_join_request_object& research_group_join_request = wrapper.get();
                                    return research_group_join_request.id == 1
                                           && research_group_join_request.research_group_id == 1
                                           && research_group_join_request.account_name == "alice"
                                           && research_group_join_request.motivation_letter == "test"
                                           && research_group_join_request.expiration_time == fc::time_point_sec(0xffffffff);
                                }));

        BOOST_CHECK(std::any_of(research_group_join_requests.begin(), research_group_join_requests.end(),
                                [](std::reference_wrapper<const research_group_join_request_object> wrapper) {
                                    const research_group_join_request_object& research_group_join_request = wrapper.get();
                                    return research_group_join_request.id == 3
                                           && research_group_join_request.research_group_id == 2
                                           && research_group_join_request.account_name == "alice"
                                           && research_group_join_request.motivation_letter == "test"
                                           && research_group_join_request.expiration_time == fc::time_point_sec(0xffffff1f);
                                }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_join_requests_by_research_group_id)
{
    try
    {
        create_research_join_request_objects();

        auto research_group_join_requests = data_service.get_research_group_join_requests_by_research_group_id(1);

        BOOST_CHECK(std::any_of(research_group_join_requests.begin(), research_group_join_requests.end(),
                                [](std::reference_wrapper<const research_group_join_request_object> wrapper) {
                                    const research_group_join_request_object& research_group_join_request = wrapper.get();
                                    return research_group_join_request.id == 1
                                           && research_group_join_request.research_group_id == 1
                                           && research_group_join_request.account_name == "alice"
                                           && research_group_join_request.motivation_letter == "test"
                                           && research_group_join_request.expiration_time == fc::time_point_sec(0xffffffff);
                                }));

        BOOST_CHECK(std::any_of(research_group_join_requests.begin(), research_group_join_requests.end(),
                                [](std::reference_wrapper<const research_group_join_request_object> wrapper) {
                                    const research_group_join_request_object& research_group_join_request = wrapper.get();
                                    return research_group_join_request.id == 2
                                           && research_group_join_request.research_group_id == 1
                                           && research_group_join_request.account_name == "bob"
                                           && research_group_join_request.motivation_letter == "test"
                                           && research_group_join_request.expiration_time == fc::time_point_sec(123);
                                }));
    }
    FC_LOG_AND_RETHROW()
}
    
BOOST_AUTO_TEST_CASE(clear_expired_research_group_join_requests)
{
    try
    {
        create_research_join_request_objects();

        BOOST_CHECK_NO_THROW(data_service.clear_expired_research_group_join_requests());
        BOOST_CHECK_THROW((db.get<research_group_join_request_object, by_id>(2)), boost::exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(is_expired)
{
    try
    {
        create_research_join_request_objects();

        BOOST_CHECK(data_service.is_expired(db.get<research_group_join_request_object>(2)) == true);
    }
    FC_LOG_AND_RETHROW()
}
    

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif

