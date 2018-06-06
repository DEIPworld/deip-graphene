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
            d.creation_time = db.head_block_time();
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
            d.creation_time = db.head_block_time();
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
            d.creation_time = db.head_block_time();
            d.expiration_time = fc::time_point_sec(0xffffff2f);
            d.quorum_percent = 72;
            d.voted_accounts.insert("alice");
        });

        db.create<proposal_object>([&](proposal_object& d) {
            d.id = 4;
            d.action = proposal_action_type::change_research_review_share_percent;
            d.data = "1123";
            d.creator = "alice";
            d.research_group_id = 1;
            d.creation_time = db.head_block_time();
            d.expiration_time = fc::time_point_sec(12313);
            d.quorum_percent = 50;
            d.voted_accounts.insert("bob"); d.voted_accounts.insert("john");
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

BOOST_AUTO_TEST_CASE(get_proposals_by_research_group_id)
{
    try
    {
        create_proposals();

        auto proposals = data_service.get_proposals_by_research_group_id(1);

        BOOST_CHECK(proposals.size() == 2);

        BOOST_CHECK(std::any_of(proposals.begin(), proposals.end(), [](std::reference_wrapper<const proposal_object> wrapper){
            const proposal_object &proposal = wrapper.get();
            return  proposal.id == 1 && proposal.research_group_id == 1 &&
                    proposal.action == proposal_action_type::dropout_member &&
                    proposal.data == "1" &&
                    proposal.creator == "alice" &&
                    proposal.expiration_time == fc::time_point_sec(0xffffffff) &&
                    proposal.quorum_percent == 40 &&
                    proposal.voted_accounts.size() == 2;
        }));
        BOOST_CHECK(std::any_of(proposals.begin(), proposals.end(), [](std::reference_wrapper<const proposal_object> wrapper){
            const proposal_object &proposal = wrapper.get();
            return  proposal.id == 4 && proposal.research_group_id == 1 &&
                    proposal.action == proposal_action_type::change_research_review_share_percent &&
                    proposal.data == "1123" &&
                    proposal.creator == "alice" &&
                    proposal.expiration_time == fc::time_point_sec(12313) &&
                    proposal.quorum_percent == 50 &&
                    proposal.voted_accounts.size() == 2;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(throw_on_get_proposal_by_non_existing_id)
{
    try
    {
        create_proposals();
        BOOST_CHECK_THROW(data_service.get_proposal(123), fc::exception);
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

BOOST_AUTO_TEST_CASE(check_proposal_existence)
 {
     try
     {
         create_proposals();
         BOOST_CHECK_THROW(data_service.check_proposal_existence(23), fc::assert_exception);
     }
     FC_LOG_AND_RETHROW()
 }

BOOST_AUTO_TEST_CASE(is_expired)
 {
     try
     {
         create_proposals();
         BOOST_CHECK(data_service.is_expired(db.get<proposal_object, by_id>(1)) == false);
         BOOST_CHECK(data_service.is_expired(db.get<proposal_object, by_id>(4)) == true);
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

BOOST_AUTO_TEST_CASE(vote_for_proposal)
 {
     try
     {
         create_proposals();

         auto& research_group_service = db.obtain_service<dbs_research_group>();

         research_group_service.create_research_group_token(1, 10, "alice");

         auto& token = research_group_service.get_research_group_token_by_account_and_research_group_id("alice", 1);
         auto& proposal = data_service.get_proposal(1);
         auto weight = token.amount;

         const proposal_vote_object& proposal_vote = data_service.vote_for(1, "alice");         

         BOOST_CHECK(proposal_vote.voter == "alice");
         BOOST_CHECK(proposal_vote.weight == weight);
         BOOST_CHECK(proposal_vote.proposal_id == 1);
         BOOST_CHECK(proposal_vote.research_group_id == 1);

         BOOST_CHECK(proposal.voted_accounts.find("alice") != proposal.voted_accounts.end());
     }
     FC_LOG_AND_RETHROW()
 }

BOOST_AUTO_TEST_CASE(get_votes_for)
 {
     try
     {
         create_proposal_votes();

         auto votes = data_service.get_votes_for(1);

         BOOST_CHECK(votes.size() == 2);

         BOOST_CHECK(std::any_of(votes.begin(), votes.end(), [](std::reference_wrapper<const proposal_vote_object> wrapper){
             const proposal_vote_object &vote = wrapper.get();
             return  vote.id == 1 && vote.research_group_id == 3 &&
                     vote.voter == "alice" &&
                     vote.proposal_id == 1 &&
                     vote.weight == 50;
         }));

         BOOST_CHECK(std::any_of(votes.begin(), votes.end(), [](std::reference_wrapper<const proposal_vote_object> wrapper){
             const proposal_vote_object &vote = wrapper.get();
             return  vote.id == 3 && vote.research_group_id == 2 &&
                     vote.voter == "bob" &&
                     vote.proposal_id == 1 &&
                     vote.weight == 60;
         }));
     }
     FC_LOG_AND_RETHROW()
 }

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
