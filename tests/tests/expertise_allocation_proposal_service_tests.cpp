#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/expertise_allocation_proposal_object.hpp>
#include <deip/chain/services/dbs_expertise_allocation_proposal.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class expertise_allocation_proposal_service_fixture : public clean_database_fixture
{
public:
    expertise_allocation_proposal_service_fixture()
            : data_service(db.obtain_service<dbs_expertise_allocation_proposal>())
    {
    }

    void expertise_allocation_proposals()
    {
        db.create<expertise_allocation_proposal_object>([&](expertise_allocation_proposal_object& eap_o) {
            eap_o.id = 0;
            eap_o.claimer = "bob";
            eap_o.discipline_id = 1;
            eap_o.total_voted_expertise = 0;
            eap_o.description = "test1";
            eap_o.expiration_time = time_point_sec(132);
        });
        db.create<expertise_allocation_proposal_object>([&](expertise_allocation_proposal_object& eap_o) {
            eap_o.id = 1;
            eap_o.claimer = "bob";
            eap_o.discipline_id = 2;
            eap_o.total_voted_expertise = 0;
            eap_o.description = "test2";
            eap_o.expiration_time = time_point_sec(0xffffffff);
        });
        db.create<expertise_allocation_proposal_object>([&](expertise_allocation_proposal_object& eap_o) {
            eap_o.id = 2;
            eap_o.claimer = "alice";
            eap_o.discipline_id = 2;
            eap_o.total_voted_expertise = 0;
            eap_o.description = "test3";
            eap_o.expiration_time = time_point_sec(0xffffffff);
        });
    }

    void expertise_allocation_proposal_votes()
    {
        db.create<expertise_allocation_proposal_vote_object>([&](expertise_allocation_proposal_vote_object& eapv_o) {
            eapv_o.id = 0;
            eapv_o.expertise_allocation_proposal_id = 0;
            eapv_o.discipline_id = 1;
            eapv_o.voter = "john";
            eapv_o.weight = DEIP_1_PERCENT * 50;
        });
        db.create<expertise_allocation_proposal_vote_object>([&](expertise_allocation_proposal_vote_object& eapv_o) {
            eapv_o.id = 1;
            eapv_o.expertise_allocation_proposal_id = 1;
            eapv_o.discipline_id = 2;
            eapv_o.voter = "mike";
            eapv_o.weight = DEIP_1_PERCENT * 50;
        });
        db.create<expertise_allocation_proposal_vote_object>([&](expertise_allocation_proposal_vote_object& eapv_o) {
            eapv_o.id = 2;
            eapv_o.expertise_allocation_proposal_id = 1;
            eapv_o.discipline_id = 2;
            eapv_o.voter = "alice";
            eapv_o.weight = DEIP_1_PERCENT * 30;
        });
        db.create<expertise_allocation_proposal_vote_object>([&](expertise_allocation_proposal_vote_object& eapv_o) {
            eapv_o.id = 3;
            eapv_o.expertise_allocation_proposal_id = 2;
            eapv_o.discipline_id = 2;
            eapv_o.voter = "cyntia";
            eapv_o.weight = DEIP_1_PERCENT * 50;
        });
        db.create<expertise_allocation_proposal_vote_object>([&](expertise_allocation_proposal_vote_object& eapv_o) {
            eapv_o.id = 4;
            eapv_o.expertise_allocation_proposal_id = 2;
            eapv_o.discipline_id = 3;
            eapv_o.voter = "mike";
            eapv_o.weight = DEIP_1_PERCENT * 10;
        });
    }
    dbs_expertise_allocation_proposal& data_service;
};

BOOST_FIXTURE_TEST_SUITE(expertise_allocation_proposal_service, expertise_allocation_proposal_service_fixture)

BOOST_AUTO_TEST_CASE(create)
{
    ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
    try
    {
        auto expertise_allocation_proposal = data_service.create("bob", 1, "test");

        BOOST_CHECK(expertise_allocation_proposal.id == 0);
        BOOST_CHECK(expertise_allocation_proposal.claimer == "bob");
        BOOST_CHECK(expertise_allocation_proposal.creation_time == db.head_block_time());
        BOOST_CHECK(expertise_allocation_proposal.expiration_time == db.head_block_time() + DAYS_TO_SECONDS(14));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get)
{
    ACTORS((alice)(bob)(john))

    try
    {
        expertise_allocation_proposals();
        auto expertise_allocation_proposal = data_service.get(0);

        BOOST_CHECK(expertise_allocation_proposal.claimer == "bob");
        BOOST_CHECK(expertise_allocation_proposal.discipline_id == 1);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_by_claimer)
{
    ACTORS((alice)(bob)(john))

    try
    {
        expertise_allocation_proposals();
        auto expertise_allocation_proposals = data_service.get_by_claimer("bob");

        BOOST_CHECK(expertise_allocation_proposals.size() == 2);
        BOOST_CHECK(std::any_of(expertise_allocation_proposals.begin(), expertise_allocation_proposals.end(),
                                [](std::reference_wrapper<const expertise_allocation_proposal_object> wrapper){
            const expertise_allocation_proposal_object &eap_o = wrapper.get();
            return eap_o.claimer == "bob" && eap_o.discipline_id == 1;
        }));
        BOOST_CHECK(std::any_of(expertise_allocation_proposals.begin(), expertise_allocation_proposals.end(),
                                [](std::reference_wrapper<const expertise_allocation_proposal_object> wrapper){
                                    const expertise_allocation_proposal_object &eap_o = wrapper.get();
                                    return eap_o.claimer == "bob" && eap_o.discipline_id == 2;
                                }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_by_claimer_and_discipline)
{
    ACTORS((alice)(bob)(john))

    try
    {
        expertise_allocation_proposals();
        auto& expertise_allocation_proposal = data_service.get_by_claimer_and_discipline("bob", 2);

        BOOST_CHECK(expertise_allocation_proposal.claimer == "bob");
        BOOST_CHECK(expertise_allocation_proposal.discipline_id == 2);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_by_discipline_id)
{
    ACTORS((alice)(bob)(john))

    try
    {
        expertise_allocation_proposals();
        auto expertise_allocation_proposals = data_service.get_by_discipline_id(2);

        BOOST_CHECK(expertise_allocation_proposals.size() == 2);
        BOOST_CHECK(std::any_of(expertise_allocation_proposals.begin(), expertise_allocation_proposals.end(),
                                [](std::reference_wrapper<const expertise_allocation_proposal_object> wrapper){
                                    const expertise_allocation_proposal_object &eap_o = wrapper.get();
                                    return eap_o.claimer == "bob" && eap_o.discipline_id == 2;
                                }));
        BOOST_CHECK(std::any_of(expertise_allocation_proposals.begin(), expertise_allocation_proposals.end(),
                                [](std::reference_wrapper<const expertise_allocation_proposal_object> wrapper){
                                    const expertise_allocation_proposal_object &eap_o = wrapper.get();
                                    return eap_o.claimer == "bob" && eap_o.discipline_id == 2;
                                }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_existence_by_claimer_and_discipline)
{
    expertise_allocation_proposals();

    BOOST_CHECK_NO_THROW(data_service.check_existence_by_claimer_and_discipline("bob", 1));
    BOOST_CHECK_THROW(data_service.check_existence_by_claimer_and_discipline("alice", 3), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(exists_by_claimer_and_discipline)
{
    expertise_allocation_proposals();

    BOOST_CHECK(data_service.exists_by_claimer_and_discipline("bob", 1) == true);
    BOOST_CHECK(data_service.exists_by_claimer_and_discipline("alice", 3) == false);
}

BOOST_AUTO_TEST_CASE(increase_total_voted_expertise)
{
    ACTORS((alice)(bob)(john)(mike))

    try
    {
        expertise_allocation_proposals();
        auto& expertise_allocation_proposal = data_service.get_by_claimer_and_discipline("bob", 2);

        data_service.downvote(expertise_allocation_proposal, "john", 1000);
        BOOST_CHECK(expertise_allocation_proposal.total_voted_expertise == -1000);

        data_service.upvote(expertise_allocation_proposal, "john", 1000);
        BOOST_CHECK(expertise_allocation_proposal.total_voted_expertise == 1000);

        data_service.upvote(expertise_allocation_proposal, "mike", 1000);
        BOOST_CHECK(expertise_allocation_proposal.total_voted_expertise == 2000);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(decrease_total_voted_expertise)
{
    ACTORS((alice)(bob)(john)(mike))

    try {
        expertise_allocation_proposals();
        auto& expertise_allocation_proposal = data_service.get_by_claimer_and_discipline("bob", 2);

        data_service.upvote(expertise_allocation_proposal, "john", 1000);
        BOOST_CHECK(expertise_allocation_proposal.total_voted_expertise == 1000);

        data_service.downvote(expertise_allocation_proposal, "john", 1000);
        BOOST_CHECK(expertise_allocation_proposal.total_voted_expertise == -1000);

        data_service.downvote(expertise_allocation_proposal, "mike", 1000);
        BOOST_CHECK(expertise_allocation_proposal.total_voted_expertise == -2000);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(delete_by_claimer_and_discipline)
{
    ACTORS((alice)(bob)(john)(mike))

    try {
        expertise_allocation_proposals();

        data_service.delete_by_claimer_and_discipline("bob", 2);

        BOOST_CHECK(db.get_index<expertise_allocation_proposal_index>().indices().size() == 2);
    }
    FC_LOG_AND_RETHROW()
}

// Expertise allocation proposal votes

BOOST_AUTO_TEST_CASE(create_vote)
{
    ACTORS_WITH_EXPERT_TOKENS((alice)(bob))
    try
    {
        auto expertise_allocation_proposal_vote = data_service.create_vote(0, 1, "alice", 5000);

        BOOST_CHECK(expertise_allocation_proposal_vote.id == 0);
        BOOST_CHECK(expertise_allocation_proposal_vote.expertise_allocation_proposal_id == 0);
        BOOST_CHECK(expertise_allocation_proposal_vote.discipline_id == 1);
        BOOST_CHECK(expertise_allocation_proposal_vote.voter == "alice");
        BOOST_CHECK(expertise_allocation_proposal_vote.weight == 5000);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_vote)
{
    ACTORS((alice)(bob)(john))

    try
    {
        expertise_allocation_proposal_votes();
        auto expertise_allocation_proposal_vote = data_service.get_vote(0);

        BOOST_CHECK(expertise_allocation_proposal_vote.id == 0);
        BOOST_CHECK(expertise_allocation_proposal_vote.expertise_allocation_proposal_id == 0);
        BOOST_CHECK(expertise_allocation_proposal_vote.discipline_id == 1);
        BOOST_CHECK(expertise_allocation_proposal_vote.voter == "john");
        BOOST_CHECK(expertise_allocation_proposal_vote.weight == 50 * DEIP_1_PERCENT);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_vote_by_voter_and_expertise_allocation_proposal_id)
{
    ACTORS((alice)(bob)(john))

    try
    {
        expertise_allocation_proposal_votes();
        auto expertise_allocation_proposal_vote = data_service.get_vote_by_voter_and_expertise_allocation_proposal_id("john", 0);

        BOOST_CHECK(expertise_allocation_proposal_vote.id == 0);
        BOOST_CHECK(expertise_allocation_proposal_vote.expertise_allocation_proposal_id == 0);
        BOOST_CHECK(expertise_allocation_proposal_vote.discipline_id == 1);
        BOOST_CHECK(expertise_allocation_proposal_vote.voter == "john");
        BOOST_CHECK(expertise_allocation_proposal_vote.weight == 50 * DEIP_1_PERCENT);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_votes_by_expertise_allocation_proposal_id)
{
    ACTORS((alice)(bob)(john))

    try
    {
        expertise_allocation_proposal_votes();
        auto expertise_allocation_proposal_votes = data_service.get_votes_by_expertise_allocation_proposal_id(1);

        BOOST_CHECK(expertise_allocation_proposal_votes.size() == 2);
        BOOST_CHECK(std::any_of(expertise_allocation_proposal_votes.begin(), expertise_allocation_proposal_votes.end(),
                                [](std::reference_wrapper<const expertise_allocation_proposal_vote_object> wrapper){
                                    const expertise_allocation_proposal_vote_object &eapv_o = wrapper.get();
                                    return eapv_o.expertise_allocation_proposal_id == 1
                                    && eapv_o.discipline_id == 2
                                    && eapv_o.voter == "mike";
                                }));
        BOOST_CHECK(std::any_of(expertise_allocation_proposal_votes.begin(), expertise_allocation_proposal_votes.end(),
                                [](std::reference_wrapper<const expertise_allocation_proposal_vote_object> wrapper){
                                    const expertise_allocation_proposal_vote_object &eapv_o = wrapper.get();
                                    return eapv_o.expertise_allocation_proposal_id == 1
                                           && eapv_o.discipline_id == 2
                                           && eapv_o.voter == "alice";
                                }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_votes_by_voter_and_discipline_id)
{
    ACTORS((alice)(bob)(john))

    try {
        expertise_allocation_proposal_votes();
        auto expertise_allocation_proposal_votes = data_service.get_votes_by_voter_and_discipline_id("mike", 2);

        BOOST_CHECK(expertise_allocation_proposal_votes.size() == 1);
        BOOST_CHECK(std::any_of(expertise_allocation_proposal_votes.begin(), expertise_allocation_proposal_votes.end(),
                                [](std::reference_wrapper<const expertise_allocation_proposal_vote_object> wrapper) {
                                    const expertise_allocation_proposal_vote_object &eapv_o = wrapper.get();
                                    return eapv_o.expertise_allocation_proposal_id == 1
                                           && eapv_o.discipline_id == 2
                                           && eapv_o.voter == "mike";
                                }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_votes_by_voter)
{
    ACTORS((alice)(bob)(john))

    try {
        expertise_allocation_proposal_votes();
        auto expertise_allocation_proposal_votes = data_service.get_votes_by_voter("mike");

        BOOST_CHECK(expertise_allocation_proposal_votes.size() == 2);
        BOOST_CHECK(std::any_of(expertise_allocation_proposal_votes.begin(), expertise_allocation_proposal_votes.end(),
                                [](std::reference_wrapper<const expertise_allocation_proposal_vote_object> wrapper) {
                                    const expertise_allocation_proposal_vote_object &eapv_o = wrapper.get();
                                    return eapv_o.expertise_allocation_proposal_id == 1
                                           && eapv_o.discipline_id == 2
                                           && eapv_o.voter == "mike";
                                }));
        BOOST_CHECK(std::any_of(expertise_allocation_proposal_votes.begin(), expertise_allocation_proposal_votes.end(),
                                [](std::reference_wrapper<const expertise_allocation_proposal_vote_object> wrapper) {
                                    const expertise_allocation_proposal_vote_object &eapv_o = wrapper.get();
                                    return eapv_o.expertise_allocation_proposal_id == 2
                                           && eapv_o.discipline_id == 3
                                           && eapv_o.voter == "mike";
                                }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
