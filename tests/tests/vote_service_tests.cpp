#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/vote_object.hpp>
#include <deip/chain/dbs_vote.hpp>

#include "database_fixture.hpp"

#define RESEARCH 1
#define MATERIAL 11
#define REVIEW 21
#define DISCIPLINE_MATH 10
#define VOTER "voter"
#define WEIGHT 100

namespace deip {
namespace chain {

class vote_service_fixture : public clean_database_fixture
{
public:
    vote_service_fixture()
            : data_service(db.obtain_service<dbs_vote>())
    {
    }

    void create_votes() {
        db.create<vote_object>([&](vote_object& r) {
            r.id = 1,
            r.research_id = RESEARCH,
            r.discipline_id = DISCIPLINE_MATH;
        });

        db.create<vote_object>([&](vote_object& r) {
            r.id = 2,
            r.research_id = RESEARCH,
            r.discipline_id = NULL;
        });
    }

    dbs_vote& data_service;
};

BOOST_FIXTURE_TEST_SUITE(vote_service, vote_service_fixture)

BOOST_AUTO_TEST_CASE(create_research_vote)
{
    try
    {
        auto time = db.head_block_time();
        auto& vote = data_service.create_research_vote(DISCIPLINE_MATH, VOTER, RESEARCH, WEIGHT, time);

        BOOST_CHECK(vote.research_id == RESEARCH);
        BOOST_CHECK(vote.discipline_id == DISCIPLINE_MATH);
        BOOST_CHECK(vote.weight == WEIGHT);
        BOOST_CHECK(vote.voter == VOTER);
        BOOST_CHECK(vote.voting_time == time);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_votes_by_discipline)
{
    try
    {
        create_votes();

        const auto& relations = data_service.get_votes_by_discipline(DISCIPLINE_MATH);


        BOOST_CHECK_EQUAL(relations.size(), 1);

        for (const chain::vote_object& relation : relations) {
            BOOST_CHECK(relation.discipline_id == DISCIPLINE_MATH);
        }

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_votes_by_null_discipline)
{
    try
    {
        create_votes();

        const auto& relations = data_service.get_votes_by_discipline(NULL);

        BOOST_CHECK_EQUAL(relations.size(), 1);

        for (const chain::vote_object& relation : relations) {
            BOOST_CHECK(relation.discipline_id == NULL);
        }

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_votes_by_research)
{
    try
    {
        create_votes();

        const auto& relations = data_service.get_votes_by_research(RESEARCH);


        BOOST_CHECK_EQUAL(relations.size(), 2);

        for (const chain::vote_object& relation : relations) {
            BOOST_CHECK(relation.research_id == RESEARCH);
        }

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
