#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/research_group_invite_object.hpp>
#include <deip/chain/dbs_research_group_invite.hpp>

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
        });

        db.create<research_group_invite_object>([&](research_group_invite_object& rgi_o) {
            rgi_o.id = 2;
            rgi_o.account_name = "alice";
            rgi_o.research_group_id = 2;
        });

        db.create<research_group_invite_object>([&](research_group_invite_object& rgi_o) {
            rgi_o.id = 3;
            rgi_o.account_name = "alice";
            rgi_o.research_group_id = 3;
        });
    }

    dbs_research_group_invite& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_group_invite_service, research_group_invite_service_fixture)

BOOST_AUTO_TEST_CASE(create_research_invite_object)
{
    try
    {
        auto& research_invite = data_service.create("bob", 2);

        BOOST_CHECK(research_invite.account_name == "bob");
        BOOST_CHECK(research_invite.research_group_id == 2);
 
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif

