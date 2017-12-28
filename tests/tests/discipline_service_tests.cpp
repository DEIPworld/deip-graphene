#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/discipline_object.hpp>
#include <deip/chain/dbs_discipline.hpp>

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

    void create_disciplines()
    {
        db.create<discipline_object>([&](discipline_object& d) {
            d.id = 0;
            d.name = "Physics";
            d.parent_id = NULL;
            d.votes_in_last_ten_weeks = 100;
        });

        db.create<discipline_object>([&](discipline_object& d) {
            d.id = 1;
            d.name = "Mathematics";
            d.parent_id = NULL;
            d.votes_in_last_ten_weeks = 150;
        });

        db.create<discipline_object>([&](discipline_object& d) {
            d.id = 3;
            d.name = "Cryptography";
            d.parent_id = 1;
            d.votes_in_last_ten_weeks = 30;
        });
    }

    dbs_discipline& data_service;
};

BOOST_FIXTURE_TEST_SUITE(discipline_service, discipline_service_fixture)

BOOST_AUTO_TEST_CASE(get_all_disciplines)
{
    try
    {
        create_disciplines();

        auto disciplines = data_service.get_disciplines();

        BOOST_CHECK(disciplines.size() == 3);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_discipline)
{
    try
    {
        create_disciplines();

        const discipline_object& discipline = data_service.get_discipline(0);

        BOOST_CHECK(discipline.id == 0 && discipline.name == "Physics");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_discipline_by_name)
{
    try
    {
        create_disciplines();

        const discipline_object& discipline = data_service.get_discipline_by_name("Physics");

        BOOST_CHECK(discipline.name == "Physics");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_disciplines_by_parent_id)
{
    try
    {
        create_disciplines();

        vector<discipline_object> discipline_objects;

        auto disciplines = data_service.get_disciplines_by_parent_id(1);
        for (const chain::discipline_object &discipline : disciplines) {
            discipline_objects.push_back(discipline);
        }

        BOOST_CHECK(discipline_objects.size() == 1 && discipline_objects[0].id == 2);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
