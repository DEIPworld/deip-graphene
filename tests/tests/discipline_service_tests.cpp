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

BOOST_AUTO_TEST_CASE(get_all_disciplines)
{
    try
    {
        auto disciplines = data_service.get_disciplines();

        BOOST_CHECK(disciplines.size() == 10);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_discipline)
{
    try
    {
        const discipline_object& discipline = data_service.get_discipline(2);

        BOOST_CHECK(discipline.id == 2 && discipline.name == "Physics");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_discipline_by_name)
{
    try
    {
        const discipline_object& discipline = data_service.get_discipline_by_name("Physics");

        BOOST_CHECK(discipline.name == "Physics");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_discipline_existence_by_name)
{
    try
    {
        BOOST_CHECK_THROW(data_service.check_discipline_existence_by_name("NonExistentDiscipline"), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_discipline_existence)
{
    try
    {
        BOOST_CHECK_THROW(data_service.check_discipline_existence(1000000), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_disciplines_by_parent_id)
{
    try
    {
        vector<discipline_object> discipline_objects;

        auto disciplines = data_service.get_disciplines_by_parent_id(1);
        for (const chain::discipline_object& discipline : disciplines)
        {
            discipline_objects.push_back(discipline);
        }

        BOOST_CHECK(discipline_objects.size() == 2 && discipline_objects[0].id == 7);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
