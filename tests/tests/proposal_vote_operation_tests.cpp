#include <boost/test/unit_test.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <deip/protocol/exceptions.hpp>

#include <deip/protocol/types.hpp>
#include <deip/chain/dbs_proposal.hpp>
#include <deip/chain/dbs_account.hpp>
#include <deip/chain/dbs_research_group.hpp>
#include <deip/chain/dbs_research.hpp>
#include <deip/chain/proposal_vote_evaluator.hpp>
#include <deip/chain/deip_objects.hpp>

#include "database_fixture.hpp"

using namespace deip::chain;

namespace deip {
namespace tests {

using deip::protocol::account_name_type;
using deip::protocol::proposal_action_type;

typedef deip::chain::proposal_vote_evaluator_t<dbs_account, dbs_proposal, dbs_research_group, dbs_research, dbs_research_token>
        proposal_vote_evaluator;

class evaluator_mocked : public proposal_vote_evaluator {
public:
    evaluator_mocked(dbs_account &account_service,
                     dbs_proposal &proposal_service,
                     dbs_research_group &research_group_service,
                     dbs_research &research_service,
                     dbs_research_token &research_token_service)
            : proposal_vote_evaluator(account_service, proposal_service, research_group_service, research_service, research_token_service) {
    }

    void execute_proposal(const proposal_object &proposal) {
        proposal_vote_evaluator::execute_proposal(proposal);
    }
};

class proposal_vote_evaluator_fixture : public clean_database_fixture {
public:
    proposal_vote_evaluator_fixture()
            : evaluator(db.obtain_service<dbs_account>(),
                        db.obtain_service<dbs_proposal>(),
                        db.obtain_service<dbs_research_group>(),
                        db.obtain_service<dbs_research>(),
                        db.obtain_service<dbs_research_token>()) {
    }

    ~proposal_vote_evaluator_fixture() {
    }

    evaluator_mocked evaluator;
};

BOOST_FIXTURE_TEST_SUITE(proposal_vote_evaluator_tests, proposal_vote_evaluator_fixture)

BOOST_AUTO_TEST_CASE(invite_member_execute_test)
{
    ACTORS((alice)(bob))
    std::vector<account_name_type> accounts = {"alice"};
    setup_research_group(1, "research_group", "research group", 0, 1, 100, accounts);
    const std::string json_str = "{\"name\":\"bob\",\"research_group_id\":1,\"research_group_token_amount\":50}";
    proposal_create(1, dbs_proposal::action_t::invite_member, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);


    auto& research_group_service = db.obtain_service<dbs_research_group>();

    proposal_vote_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    evaluator.do_apply(op);

    auto& bobs_token = research_group_service.get_research_group_token_by_account_and_research_id("bob", 1);

    BOOST_CHECK(bobs_token.owner == "bob");
    BOOST_CHECK(bobs_token.amount == 50);
    BOOST_CHECK(bobs_token.research_group_id == 1);

    auto& research_group = research_group_service.get_research_group(1);
    BOOST_CHECK(research_group.total_tokens_amount == 250);
}

BOOST_AUTO_TEST_CASE(exclude_member_test)
{
    try
    {
        ACTORS((alice)(bob));

        auto& research_group_service = db.obtain_service<dbs_research_group>();
        vector<account_name_type> accounts = { "alice", "bob" };
        setup_research_group(1, "research_group", "research group", 0, 1, 100, accounts);

        const std::string exclude_member_json = "{\"name\":\"bob\",\"research_group_id\": 1}";
        proposal_create(1, dbs_proposal::action_t::dropout_member, exclude_member_json, "alice", 1, time_point_sec(0xffffffff), 1);

        proposal_vote_operation op;

        op.research_group_id = 1;
        op.proposal_id = 1;
        op.voter = "alice";

        evaluator.do_apply(op);

        auto& research_group = research_group_service.get_research_group(1);

        BOOST_CHECK_THROW(research_group_service.get_research_group_token_by_account_and_research_id("bob", 1), std::out_of_range);
        BOOST_CHECK(research_group.total_tokens_amount == 200);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_quorum_test)
{
    try
    {
        ACTORS((alice)(bob));

        auto& research_group_service = db.obtain_service<dbs_research_group>();
        vector<account_name_type> accounts = { "alice", "bob" };
        setup_research_group(1, "research_group", "research group", 0, 1, 100, accounts);

        const std::string change_quorum_json = "{\"quorum_percent\": 80,\"research_group_id\": 1}";
        proposal_create(1, dbs_proposal::action_t::change_quorum, change_quorum_json, "alice", 1, time_point_sec(0xffffffff), 1);

        proposal_vote_operation op;

        op.research_group_id = 1;
        op.proposal_id = 1;
        op.voter = "alice";

        evaluator.do_apply(op);

        auto& research_group = research_group_service.get_research_group(1);

        BOOST_CHECK(research_group.quorum_percent == 80);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(start_research_execute_test)
{
    ACTORS((alice))
    std::vector<account_name_type> accounts = {"alice"};
    setup_research_group(1, "research_group", "research group", 0, 1, 100, accounts);
    const std::string json_str = "{\"name\":\"test\","
            "\"research_group_id\":1,"
            "\"abstract\":\"abstract\","
            "\"permlink\":\"permlink\","
            "\"percent_for_review\": 10}";
    proposal_create(1, dbs_proposal::action_t::start_research, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    proposal_vote_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    evaluator.do_apply(op);

    auto& research_service = db.obtain_service<dbs_research>();
    auto& research = research_service.get_research(0);

    BOOST_CHECK(research.name == "test");
    BOOST_CHECK(research.abstract == "abstract");
    BOOST_CHECK(research.permlink == "permlink");
    BOOST_CHECK(research.research_group_id == 1);
    BOOST_CHECK(research.percent_for_review == 10);
}

BOOST_AUTO_TEST_CASE(transfer_research_tokens_execute_test)
{
    ACTORS((alice)(bob))
    fund("bob", 1000);
    std::vector<account_name_type> accounts = {"alice"};
    setup_research_group(1, "research_group", "research group", 0, 1, 100, accounts);
    const std::string json_str = "{\"research_id\":0,"
            "\"total_price\":500,"
            "\"account_name\":\"bob\","
            "\"amount\": 5}";

    auto& research = research_create("name","abstract", "permlink", 1, 10);
    proposal_create(1, dbs_proposal::action_t::transfer_research_tokens, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    proposal_vote_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    evaluator.do_apply(op);

    auto& research_token_service = db.obtain_service<dbs_research_token>();
    auto& bobs_token = research_token_service.get_research_token_by_account_name_and_research_id("bob", 0);

    auto& research_group_service = db.obtain_service<dbs_research_group>();
    auto& research_group = research_group_service.get_research_group(1);

    BOOST_CHECK(bobs_token.amount == 5);
    BOOST_CHECK(research_group.funds == 500);
    BOOST_CHECK(research.owned_tokens == 9995);
}

BOOST_AUTO_TEST_CASE(send_funds_execute_test)
{
    ACTORS((alice)(bob))
    fund("bob", 1000);
    std::vector<account_name_type> accounts = {"alice"};
    setup_research_group(1, "research_group", "research group", 750, 1, 100, accounts);
    const std::string json_str = "{\"research_group_id\":1,"
            "\"account_name\":\"bob\","
            "\"funds\": 250}";

    proposal_create(1, dbs_proposal::action_t::send_funds, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    proposal_vote_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    evaluator.do_apply(op);

    auto& account_service = db.obtain_service<dbs_account>();
    auto& bobs_account = account_service.get_account("bob");

    auto& research_group_service = db.obtain_service<dbs_research_group>();
    auto& research_group = research_group_service.get_research_group(1);

    BOOST_CHECK(research_group.funds == 500);
    BOOST_CHECK(bobs_account.balance.amount == 1250);
}

BOOST_AUTO_TEST_CASE(invite_member_validate_test)
{
    const std::string json_str = "{\"name\":\"\",\"research_group_id\":1,\"research_group_token_amount\":1000}";
    proposal_create(1, dbs_proposal::action_t::invite_member, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    proposal_vote_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(exclude_member_validate_test)
{
    try
    {
        const std::string exclude_member_json = "{\"name\":\"\",\"research_group_id\": 1}";
        proposal_create(1, dbs_proposal::action_t::dropout_member, exclude_member_json, "alice", 1, time_point_sec(0xffffffff), 1);

        proposal_vote_operation op;

        op.research_group_id = 1;
        op.proposal_id = 1;
        op.voter = "alice";

        BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_quorum_validate_test)
{
    try
    {
        const std::string change_quorum_json = "{\"quorum_percent\": 1000,\"research_group_id\": 1}";
        proposal_create(1, dbs_proposal::action_t::change_quorum, change_quorum_json, "alice", 1, time_point_sec(0xffffffff), 1);

        proposal_vote_operation op;

        op.research_group_id = 1;
        op.proposal_id = 1;
        op.voter = "alice";

        BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(start_research_validate_test)
{
    const std::string json_str = "{\"name\":\"\","
            "\"research_group_id\":1,"
            "\"abstract\":\"\","
            "\"permlink\":\"\","
            "\"percent_for_review\": 10000}";
    proposal_create(1, dbs_proposal::action_t::start_research, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    proposal_vote_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(transfer_research_tokens_validate_test)
{
    const std::string json_str = "{\"research_id\":0,"
            "\"total_price\":500,"
            "\"account_name\":\"bob\","
            "\"amount\": 5}";
    proposal_create(1, dbs_proposal::action_t::transfer_research_tokens, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    proposal_vote_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(send_funds_validate_test)
{
    const std::string json_str = "{\"research_group_id\":1,"
            "\"account_name\":\"bob\","
            "\"funds\": 250}";
    proposal_create(1, dbs_proposal::action_t::transfer_research_tokens, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    proposal_vote_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
}
BOOST_AUTO_TEST_SUITE_END()

}
}