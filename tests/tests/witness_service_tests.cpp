#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/witness_objects.hpp>
#include <deip/chain/dbs_witness.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class witness_service_fixture : public clean_database_fixture
{
public:
    witness_service_fixture()
            : data_service(db.obtain_service<dbs_witness>())
    {
    }

    void create_witnesses_objects() {
        db.create<witness_object>([&](witness_object& w_o) {
            w_o.id = 1;
            w_o.owner = "alice";
            w_o.created = fc::time_point_sec(123);
            w_o.votes = 100;
            w_o.virtual_scheduled_time = 1123;
        });

        db.create<witness_object>([&](witness_object& w_o) {
            w_o.id = 2;
            w_o.owner = "bob";
            w_o.created = fc::time_point_sec(1234);
            w_o.votes = 200;
            w_o.virtual_scheduled_time = 1124;
        });
    }

    void create_witness_schedule_object() {
        db.create<witness_schedule_object>([&](witness_schedule_object& ws_o) {
            ws_o.id = 1;
            ws_o.current_virtual_time = 1234;
        });
    }

    dbs_witness& data_service;
};

BOOST_FIXTURE_TEST_SUITE(witness_service, witness_service_fixture)

BOOST_AUTO_TEST_CASE(get_witness)
{
    try
    {
        create_witnesses_objects();

        auto& witness = data_service.get_witness("alice");

        BOOST_CHECK(witness.id == 1);
        BOOST_CHECK(witness.owner == "alice");
        BOOST_CHECK(witness.created == fc::time_point_sec(123));
        BOOST_CHECK(witness.votes == 100);
        BOOST_CHECK(witness.virtual_scheduled_time == 1123);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_top_witness)
{
    try
    {
        create_witnesses_objects();

        auto& witness = data_service.get_top_witness();

        BOOST_CHECK(witness.id == 2);
        BOOST_CHECK(witness.owner == "bob");
        BOOST_CHECK(witness.created == fc::time_point_sec(1234));
        BOOST_CHECK(witness.votes == 200);
        BOOST_CHECK(witness.virtual_scheduled_time == 1124);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_witness_schedule_object)
{
    try
    {
        create_witness_schedule_object();

        auto& witness_schedule = data_service.get_witness_schedule_object();

        BOOST_CHECK(witness_schedule.id == 1);
        BOOST_CHECK(witness_schedule.current_virtual_time == 1234);

    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif