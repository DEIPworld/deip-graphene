#include <boost/test/unit_test.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <deip/protocol/exceptions.hpp>

#include <deip/protocol/types.hpp>
#include <deip/chain/dbs_proposal.hpp>
#include <deip/chain/dbs_account.hpp>
#include <deip/chain/dbs_research_group.hpp>
#include <deip/chain/proposal_vote_evaluator.hpp>
#include <deip/chain/proposal_object.hpp>

#include "defines.hpp"
#include "database_fixture.hpp"

namespace tests {

    using deip::protocol::account_name_type;
    using deip::chain::proposal_object;
    using deip::chain::proposal_vote_operation;
    using deip::protocol::proposal_action_type;
    using deip::chain::proposal_id_type;


    typedef deip::chain::proposal_vote_evaluator_t<deip::chain::dbs_account, deip::chain::dbs_proposal, deip::chain::dbs_research_group>
            proposal_vote_evaluator;

    class evaluator_mocked : public proposal_vote_evaluator
    {
    public:
        evaluator_mocked(deip::chain::dbs_account& account_service,
                         deip::chain::dbs_proposal& proposal_service,
                         deip::chain::dbs_research_group& research_group_service)
                : proposal_vote_evaluator(account_service, proposal_service, research_group_service)
        {
        }

        void execute_proposal(const proposal_object& proposal)
        {
            proposal_vote_evaluator::execute_proposal(proposal);
        }
    };

    class proposal_vote_evaluator_fixture : public deip::chain::clean_database_fixture
    {
    public:
        proposal_vote_evaluator_fixture()
                : evaluator(db.obtain_service<deip::chain::dbs_account>(),
                            db.obtain_service<deip::chain::dbs_proposal>(),
                            db.obtain_service<deip::chain::dbs_research_group>())
        {
        }

        ~proposal_vote_evaluator_fixture()
        {
        }

        void apply()
        {
            evaluator.do_apply(op);
        }

        const proposal_object& proposal_create(const uint32_t id,
                                               const deip::chain::dbs_proposal::action_t action,
                                               const std::string json_data,
                                               const account_name_type& creator,
                                               const deip::chain::research_group_id_type& research_group_id,
                                               const fc::time_point_sec expiration_time,
                                               const uint32_t quorum_percent)
        {
            const proposal_object& new_proposal = db.create<proposal_object>([&](proposal_object& proposal) {
                proposal.action = action;
                proposal.id = id;
                proposal.data = json_data;
                proposal.creator = creator;
                proposal.research_group_id = research_group_id;
                proposal.creation_time = fc::time_point_sec();
                proposal.expiration_time = expiration_time;
                proposal.quorum_percent = quorum_percent;
            });

            return new_proposal;
        }

        proposal_vote_operation op;
        evaluator_mocked evaluator;
    };

BOOST_FIXTURE_TEST_SUITE(proposal_vote_evaluator_tests, proposal_vote_evaluator_fixture)


BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(proposal_execute_tests, proposal_vote_evaluator_fixture)


BOOST_AUTO_TEST_SUITE_END()

}//
// Created by dzeranov on 25.1.18.
//

