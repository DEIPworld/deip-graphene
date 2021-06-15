#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/witness_objects.hpp>

#include <deip/chain/schema/research_group_object.hpp>
#include <deip/chain/services/dbs_research_group.hpp>


#include "database_fixture.hpp"

namespace deip {
namespace chain {

class research_group_service_fixture : public clean_database_fixture
{
 public:
   research_group_service_fixture()
           : data_service(db.obtain_service<dbs_research_group>())
   {

   }

    void create_research_groups()
    {

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 21;
            d.name = "test21";
            d.description = "test";
        });

        db.create<research_group_object>([&](research_group_object& d) {
            d.id = 22;
            d.name = "test22";
            d.description = "test";
          });
    }

    dbs_research_group& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_group_service_tests, research_group_service_fixture)

BOOST_AUTO_TEST_CASE(get_research_group_by_id_test)
{
    try
    {
        create_research_groups();
        auto& research_group = data_service.get_research_group(21);

        BOOST_CHECK(research_group.name == "test21");
        BOOST_CHECK(research_group.description == "test");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
