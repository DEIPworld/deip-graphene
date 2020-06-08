#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/discipline_object.hpp>
#include <deip/chain/services/dbs_discipline.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class discipline_service_fixture : public clean_database_fixture
{
public:
    discipline_service_fixture()
            : data_service(db.obtain_service<dbs_discipline>())
    {
    }

    dbs_discipline& data_service;
};

BOOST_FIXTURE_TEST_SUITE(discipline_service, discipline_service_fixture)



BOOST_AUTO_TEST_CASE(get_discipline)
{
    try
    {
        const discipline_object& discipline = data_service.get_discipline(2);

        BOOST_CHECK(discipline.id == 2 && discipline.name == "Physics");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
