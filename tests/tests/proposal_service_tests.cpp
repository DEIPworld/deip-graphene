#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/proposal_object.hpp>
#include <deip/chain/proposal_vote_object.hpp>
#include <deip/chain/dbs_proposal.hpp>
#include <deip/chain/dbs_research_group.hpp>


#include "database_fixture.hpp"

namespace deip {
namespace chain {

class proposal_service_fixture : public clean_database_fixture
{
public:
    proposal_service_fixture()
            : data_service(db.obtain_service<dbs_proposal>())
    {
    }

    void create_proposals()
    {
        db.create<proposal_object>([&](proposal_object& d) {
            d.id = 1;
            d.action = proposal_action_type::dropout_member;
            d.data = "1";
            d.creator = "alice";
            d.research_group_id = 1;
            d.expiration_time = fc::time_point_sec(0xffffffff);
            d.quorum_percent = 40;
            d.voted_accounts.insert("bob"); d.voted_accounts.insert("john");
        });

        db.create<proposal_object>([&](proposal_object& d) {
            d.id = 2;
            d.action = proposal_action_type::invite_member;
            d.data = "123";
            d.creator = "bob";
            d.research_group_id = 2;
            d.expiration_time = fc::time_point_sec(123);
            d.quorum_percent = 38;
            d.voted_accounts.insert("alice"); d.voted_accounts.insert("john");
        });

        db.create<proposal_object>([&](proposal_object& d) {
            d.id = 3;
            d.action = proposal_action_type::change_quorum;
            d.data = "1123john";
            d.creator = "john";
            d.research_group_id = 123;
            d.expiration_time = fc::time_point_sec(0xffffff2f);
            d.quorum_percent = 72;
            d.voted_accounts.insert("alice");
        });
    }

    void create_proposal_votes()
    {
        db.create<proposal_vote_object>([&](proposal_vote_object& d) {
            d.id = 1;
            d.voter = "alice";
            d.weight = 50;
            d.proposal_id = 1;
            d.research_group_id = 3;
        });

        db.create<proposal_vote_object>([&](proposal_vote_object& d) {
            d.id = 2;
            d.voter = "alice";
            d.weight = 50;
            d.proposal_id = 2;
            d.research_group_id = 3;
        });

        db.create<proposal_vote_object>([&](proposal_vote_object& d) {
            d.id = 3;
            d.voter = "bob";
            d.weight = 60;
            d.proposal_id = 1;
            d.research_group_id = 2;
        });
    }
    dbs_proposal& data_service;
};

BOOST_FIXTURE_TEST_SUITE(proposal_service, proposal_service_fixture)

BOOST_AUTO_TEST_CASE(create_proposal)
{
    try
    {
        const proposal_object& proposal = data_service.create_proposal(proposal_action_type::dropout_member, "1", "alice",
                                                                       1, fc::time_point_sec(0xffffffff), 40);

        BOOST_CHECK(proposal.action == proposal_action_type::dropout_member);
        BOOST_CHECK(proposal.data == "1");
        BOOST_CHECK(proposal.creator == "alice");
        BOOST_CHECK(proposal.research_group_id == 1);
        BOOST_CHECK(proposal.expiration_time == fc::time_point_sec(0xffffffff));
        BOOST_CHECK(proposal.quorum_percent == 40);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_proposal_by_id)
 {
     try
     {
         create_proposals();
         auto proposal = data_service.get_proposal(3);

         BOOST_CHECK(proposal.action == proposal_action_type::change_quorum);
         BOOST_CHECK(proposal.data == "1123john");
         BOOST_CHECK(proposal.creator == "john");
         BOOST_CHECK(proposal.research_group_id == 123);
         BOOST_CHECK(proposal.expiration_time == fc::time_point_sec(0xffffff2f));
         BOOST_CHECK(proposal.quorum_percent == 72);
         BOOST_CHECK(proposal.voted_accounts.size() == 1);

     }
     FC_LOG_AND_RETHROW()
 }

BOOST_AUTO_TEST_CASE(throw_on_get_proposal_by_non_existing_id)
{
    try
    {
        create_proposals();
        BOOST_CHECK_THROW(data_service.get_proposal(123), boost::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(remove_proposal)
 {
     try
     {
         create_proposals();
         auto& proposal =  db.get<proposal_object, by_id>(2);
         BOOST_CHECK_NO_THROW(data_service.remove(proposal));
         BOOST_CHECK_THROW((db.get<proposal_object, by_id>(2)), boost::exception);
     }
     FC_LOG_AND_RETHROW()
 }

BOOST_AUTO_TEST_CASE(clear_expired_proposals)
 {
     try
     {
         create_proposals();

         BOOST_CHECK_NO_THROW(data_service.clear_expired_proposals());
         BOOST_CHECK_THROW((db.get<proposal_object, by_id>(2)), boost::exception);

     }
     FC_LOG_AND_RETHROW()
 }

BOOST_AUTO_TEST_CASE(remove_proposal_votes)
 {
     try
     {
         create_proposal_votes();

         BOOST_CHECK_NO_THROW(data_service.remove_proposal_votes("alice", 3));
         BOOST_CHECK_THROW((db.get<proposal_vote_object, by_voter>(boost::make_tuple("alice", 3))), boost::exception);

     }
     FC_LOG_AND_RETHROW()
 }

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif//
// Created by dzeranov on 19.1.18.
//

