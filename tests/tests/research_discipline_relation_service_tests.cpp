#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/research_discipline_relation_object.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>

#include "database_fixture.hpp"

#define RESEARCH_ONE 1
#define RESEARCH_TWO 2
#define DISCIPLINE_MATH 10
#define DISCIPLINE_PHYSICS 20
#define VOTES_COUNT 100

namespace deip {
namespace chain {

class research_discipline_relation_service_fixture : public clean_database_fixture
{
public:
    research_discipline_relation_service_fixture()
            : data_service(db.obtain_service<dbs_research_discipline_relation>())
    {
    }

    void create_research_discipline_relations() {
        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
            r.id = 1,
            r.research_id = RESEARCH_ONE,
            r.discipline_id = DISCIPLINE_MATH;
            r.votes_count = VOTES_COUNT;
        });

        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
            r.id = 2,
            r.research_id = RESEARCH_ONE,
            r.discipline_id = DISCIPLINE_PHYSICS;
            r.votes_count = VOTES_COUNT;
        });

        db.create<research_discipline_relation_object>([&](research_discipline_relation_object& r) {
            r.id = 3,
            r.research_id = RESEARCH_TWO,
            r.discipline_id = DISCIPLINE_MATH;
            r.votes_count = VOTES_COUNT;
        });
    }

    dbs_research_discipline_relation& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_discipline_relation_service, research_discipline_relation_service_fixture)

BOOST_AUTO_TEST_CASE(create_research_discipline_relation)
{
    try
    {
        auto& relation = data_service.create_research_relation(RESEARCH_ONE, DISCIPLINE_MATH);

        BOOST_CHECK(relation.research_id == RESEARCH_ONE);
        BOOST_CHECK(relation.discipline_id == DISCIPLINE_MATH);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_discipline_relation)
{
    try
    {
        create_research_discipline_relations();

        const research_discipline_relation_object& research_discipline_relation = data_service.get_research_discipline_relation(1);

        BOOST_CHECK(research_discipline_relation.id == 1 &&  research_discipline_relation.research_id == RESEARCH_ONE
                    && research_discipline_relation.discipline_id == DISCIPLINE_MATH);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_discipline_relations_by_discipline)
{
    try
    {
        create_research_discipline_relations();

        const auto& relations = data_service.get_research_discipline_relations_by_discipline(DISCIPLINE_MATH);

        BOOST_CHECK_EQUAL(relations.size(), 2);

        for (const chain::research_discipline_relation_object& relation : relations) {
            BOOST_CHECK(relation.discipline_id == DISCIPLINE_MATH);
        }

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_discipline_relations_by_research)
{
    try
    {
        create_research_discipline_relations();

        const auto& relations = data_service.get_research_discipline_relations_by_research(RESEARCH_ONE);

        BOOST_CHECK_EQUAL(relations.size(), 2);

        for (const chain::research_discipline_relation_object& relation : relations) {
            BOOST_CHECK(relation.research_id == RESEARCH_ONE);
        }

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_discipline_relation_by_research_and_discipline)
{
    try
    {
        create_research_discipline_relations();

        const auto& relation = data_service.get_research_discipline_relation_by_research_and_discipline(RESEARCH_ONE, DISCIPLINE_MATH);

        BOOST_CHECK(relation.research_id == RESEARCH_ONE);
        BOOST_CHECK(relation.discipline_id == DISCIPLINE_MATH);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(update_votes_count_delta)
{
    try
    {
        create_research_discipline_relations();

        int delta = 50;

        const auto& updated_relation = data_service.update_votes_count(RESEARCH_ONE, DISCIPLINE_PHYSICS, delta);

        BOOST_CHECK(updated_relation.votes_count == (VOTES_COUNT + delta));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(update_votes_count_negative_delta_expect_error)
{
    try
    {
        create_research_discipline_relations();

        int delta = -150;

        BOOST_CHECK_THROW(data_service.update_votes_count(RESEARCH_ONE, DISCIPLINE_PHYSICS, delta), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(exists_by_research_and_discipline)
{
    try
    {
        create_research_discipline_relations();

        BOOST_CHECK_NO_THROW(data_service.check_existence_by_research_and_discipline(1, 10));
        BOOST_CHECK_THROW(data_service.check_existence_by_research_and_discipline(3, 10), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
