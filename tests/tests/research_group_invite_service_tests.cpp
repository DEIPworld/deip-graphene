#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/research_group_invite_object.hpp>
#include <deip/chain/services/dbs_research_group_invite.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class research_group_invite_service_fixture : public clean_database_fixture
{
public:
    research_group_invite_service_fixture()
            : data_service(db.obtain_service<dbs_research_group_invite>())
    {
    }

    void create_research_invite_objects() {
        db.create<research_group_invite_object>([&](research_group_invite_object& rgi_o) {
            rgi_o.id = 1;
            rgi_o.account_name = "alice";
            rgi_o.research_group_id = 1;
            rgi_o.research_group_token_amount = 100;
            rgi_o.expiration_time = fc::time_point_sec(0xffffffff);
        });

        db.create<research_group_invite_object>([&](research_group_invite_object& rgi_o) {
            rgi_o.id = 2;
            rgi_o.account_name = "alice";
            rgi_o.research_group_id = 2;
            rgi_o.research_group_token_amount = 100;
            rgi_o.expiration_time = fc::time_point_sec(123);
        });

        db.create<research_group_invite_object>([&](research_group_invite_object& rgi_o) {
            rgi_o.id = 3;
            rgi_o.account_name = "bob";
            rgi_o.research_group_id = 1;
            rgi_o.research_group_token_amount = 100;
            rgi_o.expiration_time = fc::time_point_sec(0xffffff1f);
        });
    }

    dbs_research_group_invite& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_group_invite_service, research_group_invite_service_fixture)

BOOST_AUTO_TEST_CASE(create)
{
    try
    {
        auto& research_group_invite = data_service.create("bob", 2, 100, "test", "alice", false);

        BOOST_CHECK(research_group_invite.account_name == "bob");
        BOOST_CHECK(research_group_invite.research_group_id == 2);
        BOOST_CHECK(research_group_invite.research_group_token_amount == 100);
        BOOST_CHECK(research_group_invite.cover_letter == "test");
        BOOST_CHECK(research_group_invite.token_source == "alice");

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(throw_on_create_research_invite_object)
{
    try
    {
        create_research_invite_objects();

        BOOST_CHECK_THROW(data_service.create("alice", 1, 200, "test", "alice", false), boost::exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get)
{
    try
    {
        create_research_invite_objects();

        auto& research_group_invite = data_service.get_research_group_invite(1);

        BOOST_CHECK(research_group_invite.account_name == "alice");
        BOOST_CHECK(research_group_invite.research_group_id == 1);
        BOOST_CHECK(research_group_invite.research_group_token_amount == 100);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_invite)
{
    try
    {
        create_research_invite_objects();

        auto& research_group_invite = data_service.get_research_group_invite("alice", 1);

        BOOST_CHECK(research_group_invite.account_name == "alice");
        BOOST_CHECK(research_group_invite.research_group_id == 1);
        BOOST_CHECK(research_group_invite.research_group_token_amount == 100);
        BOOST_CHECK(research_group_invite.expiration_time == fc::time_point_sec(0xffffffff));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(research_group_invite_exists)
{
    try
    {
        create_research_invite_objects();

        BOOST_CHECK(data_service.research_group_invite_exists(1));
        BOOST_CHECK(data_service.research_group_invite_exists(2));
        BOOST_CHECK(!data_service.research_group_invite_exists(25));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_invites_by_account_name)
{
    try
    {
        create_research_invite_objects();

        auto research_group_invites = data_service.get_research_group_invites_by_account_name("alice");

        BOOST_CHECK(std::any_of(research_group_invites.begin(), research_group_invites.end(),
                                [](std::reference_wrapper<const research_group_invite_object> wrapper) {
                                    const research_group_invite_object& research_group_invite = wrapper.get();
                                    return research_group_invite.id == 1
                                           && research_group_invite.research_group_id == 1
                                           && research_group_invite.account_name == "alice"
                                           && research_group_invite.research_group_token_amount == 100
                                           && research_group_invite.expiration_time == fc::time_point_sec(0xffffffff);

                                }));

        BOOST_CHECK(std::any_of(research_group_invites.begin(), research_group_invites.end(),
                                [](std::reference_wrapper<const research_group_invite_object> wrapper) {
                                    const research_group_invite_object& research_group_invite = wrapper.get();
                                    return research_group_invite.id == 2
                                           && research_group_invite.research_group_id == 2
                                           && research_group_invite.account_name == "alice"
                                           && research_group_invite.research_group_token_amount == 100
                                           && research_group_invite.expiration_time == fc::time_point_sec(123);
                                }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_group_invites_by_research_group_id)
{
    try
    {
        create_research_invite_objects();

        auto research_group_invites = data_service.get_research_group_invites_by_research_group_id(1);

        BOOST_CHECK(std::any_of(research_group_invites.begin(), research_group_invites.end(),
                                [](std::reference_wrapper<const research_group_invite_object> wrapper) {
                                    const research_group_invite_object& research_group_invite = wrapper.get();
                                    return research_group_invite.id == 1
                                           && research_group_invite.research_group_id == 1
                                           && research_group_invite.account_name == "alice"
                                           && research_group_invite.research_group_token_amount == 100;
                                }));

        BOOST_CHECK(std::any_of(research_group_invites.begin(), research_group_invites.end(),
                                [](std::reference_wrapper<const research_group_invite_object> wrapper) {
                                    const research_group_invite_object& research_group_invite = wrapper.get();
                                    return research_group_invite.id == 3
                                           && research_group_invite.research_group_id == 1
                                           && research_group_invite.account_name == "bob"
                                           && research_group_invite.research_group_token_amount == 100;
                                }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(clear_expired_invites)
{
    try
    {
        create_research_invite_objects();

        BOOST_CHECK_NO_THROW(data_service.clear_expired_invites());
        BOOST_CHECK_THROW((db.get<research_group_invite_object, by_id>(2)), boost::exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(is_expired)
{
    try
    {
        create_research_invite_objects();

        BOOST_CHECK(data_service.is_expired(db.get<research_group_invite_object>(2)) == true);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif

