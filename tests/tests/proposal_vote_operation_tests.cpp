#include <boost/test/unit_test.hpp>
#include <boost/range/algorithm/remove_if.hpp>
#include <deip/protocol/exceptions.hpp>

#include <deip/protocol/types.hpp>
#include <deip/chain/dbs_proposal.hpp>
#include <deip/chain/dbs_account.hpp>
#include <deip/chain/dbs_research_group.hpp>
#include <deip/chain/dbs_research.hpp>

#include <deip/chain/dbs_research_token_sale.hpp>
#include <deip/chain/dbs_research_content.hpp>
#include <deip/chain/dbs_dynamic_global_properties.hpp>

#include <deip/chain/proposal_vote_evaluator.hpp>
#include <deip/chain/deip_objects.hpp>

#include "database_fixture.hpp"

#define DROPOUT_COMPENSATION_IN_PERCENT 1500

using namespace deip::chain;

namespace deip {
namespace tests {

using deip::protocol::account_name_type;
using deip::protocol::proposal_action_type;

typedef deip::chain::proposal_vote_evaluator_t<dbs_account,
                                               dbs_proposal,
                                               dbs_research_group,
                                               dbs_research,
                                               dbs_research_token,
                                               dbs_research_content,
                                               dbs_research_token_sale,
                                               dbs_discipline,
                                               dbs_research_discipline_relation,
                                               dbs_research_group_invite,
                                               dbs_dynamic_global_properties>
        proposal_vote_evaluator;


class evaluator_mocked : public proposal_vote_evaluator {
public:
    evaluator_mocked(dbs_account &account_service,
                     dbs_proposal &proposal_service,
                     dbs_research_group &research_group_service,
                     dbs_research &research_service,
                     dbs_research_token &research_token_service,
                     dbs_research_content &research_content_service,
                     dbs_research_token_sale &research_token_sale_service,
                     dbs_discipline &discipline_service,
                     dbs_research_discipline_relation &research_discipline_relation_service,
                     dbs_research_group_invite &research_group_invite_service,
                     dbs_dynamic_global_properties &dynamic_global_properties_service)
            : proposal_vote_evaluator(account_service, proposal_service, research_group_service, research_service, research_token_service, research_content_service, research_token_sale_service, discipline_service, research_discipline_relation_service, research_group_invite_service, dynamic_global_properties_service) {
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
                    db.obtain_service<dbs_research_token>(),
                    db.obtain_service<dbs_research_content>(),
                    db.obtain_service<dbs_research_token_sale>(),
                    db.obtain_service<dbs_discipline>(),
                    db.obtain_service<dbs_research_discipline_relation>(),
                    db.obtain_service<dbs_research_group_invite>(),
                    db.obtain_service<dbs_dynamic_global_properties>())
    {
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
    create_proposal(1, dbs_proposal::action_t::invite_member, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);


    auto& research_group_invite_service = db.obtain_service<dbs_research_group_invite>();
 
    vote_proposal_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    evaluator.do_apply(op);

    auto& research_group_invite = research_group_invite_service.get_research_group_invite_by_account_name_and_research_group_id("bob", 1);

    BOOST_CHECK(research_group_invite.account_name == "bob");
    BOOST_CHECK(research_group_invite.research_group_id == 1);
    BOOST_CHECK(research_group_invite.research_group_token_amount == 50);
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
        create_proposal(1, dbs_proposal::action_t::dropout_member, exclude_member_json, "alice", 1, time_point_sec(0xffffffff), 1);

        vote_proposal_operation op;

        op.research_group_id = 1;
        op.proposal_id = 1;
        op.voter = "alice";

        evaluator.do_apply(op);

        auto& research_group = research_group_service.get_research_group(1);

        BOOST_CHECK_THROW(research_group_service.get_research_group_token_by_account_and_research_group_id("bob", 1), std::out_of_range);
        BOOST_CHECK(research_group.total_tokens_amount == 200);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_research_review_share_test)
{
    try
    {
        ACTORS((alice));

        auto& research_service = db.obtain_service<dbs_research>();

        research_group_create_by_operation("alice", "test permlink", "test description", 50,
                                           100);

        const std::string create_research_proposal_json = "{\"name\":\"testresearch\","
                                                          "\"research_group_id\":0,"
                                                          "\"abstract\":\"abstract\","
                                                          "\"permlink\":\"permlink\","
                                                          "\"review_share_in_percent\": 1000,"
                                                          "\"dropout_compensation_in_percent\": 1500,"
                                                          "\"disciplines\": [1, 2, 3]}";
        const std::string change_review_share_proposal_json = "{\"review_share_in_percent\": 4500,\"research_id\": 0}";

        create_proposal_by_operation("alice", 0, create_research_proposal_json,
                                     dbs_proposal::action_t::start_research,
                                     fc::time_point_sec(db.head_block_time().sec_since_epoch() + DAYS_TO_SECONDS(2)));

        vote_proposal_operation op;

        op.research_group_id = 0;
        op.proposal_id = 0;
        op.voter = "alice";

        generate_block();
        create_disciplines();

        evaluator.do_apply(op);
        
        generate_blocks(fc::time_point_sec(db.head_block_time().sec_since_epoch() + DAYS_TO_SECONDS(90)), true);

        create_proposal_by_operation("alice", 0, change_review_share_proposal_json,
                                     dbs_proposal::action_t::change_research_review_share_percent,
                                     fc::time_point_sec(db.head_block_time().sec_since_epoch() + DAYS_TO_SECONDS(2)));

        vote_proposal_operation crs_op;

        crs_op.research_group_id = 0;
        crs_op.proposal_id = 1;
        crs_op.voter = "alice";

        evaluator.do_apply(crs_op);

        auto& research = research_service.get_research(0);

        BOOST_CHECK(research.review_share_in_percent == 4500);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_research_review_share_rate_test)
{
    try
    {
        ACTORS((alice));

        auto& research_service = db.obtain_service<dbs_research>();

        research_group_create_by_operation("alice", "test permlink", "test description", 50,
                                           100);

        const std::string create_research_proposal_json = "{\"name\":\"testresearch\","
                                                          "\"research_group_id\":0,"
                                                          "\"abstract\":\"abstract\","
                                                          "\"permlink\":\"permlink\","
                                                          "\"review_share_in_percent\": 1000,"
                                                          "\"dropout_compensation_in_percent\": 1500,"
                                                          "\"disciplines\": [1, 2, 3]}";
        const std::string change_review_share_proposal_json = "{\"review_share_in_percent\": 4500,\"research_id\": 0}";

        create_proposal_by_operation("alice", 0, create_research_proposal_json,
                                     dbs_proposal::action_t::start_research,
                                     fc::time_point_sec(db.head_block_time().sec_since_epoch() + DAYS_TO_SECONDS(2)));

        vote_proposal_operation op;

        op.research_group_id = 0;
        op.proposal_id = 0;
        op.voter = "alice";

        create_disciplines();

        evaluator.do_apply(op);
        
        create_proposal_by_operation("alice", 0, change_review_share_proposal_json,
                                     dbs_proposal::action_t::change_research_review_share_percent,
                                     fc::time_point_sec(db.head_block_time().sec_since_epoch() + DAYS_TO_SECONDS(2)));

        vote_proposal_operation crs_op;

        crs_op.research_group_id = 0;
        crs_op.proposal_id = 1;
        crs_op.voter = "alice";

        auto& research = research_service.get_research(0);

        BOOST_CHECK_THROW(evaluator.do_apply(crs_op), fc::assert_exception);
        BOOST_CHECK(research.review_share_in_percent == 1000);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(exclude_member_with_research_token_compensation_test)
{
    try
    {
        ACTORS((alice)(bob));

        auto& research_group_service = db.obtain_service<dbs_research_group>();
        vector<account_name_type> accounts = { "alice", "bob" };
        setup_research_group(1, "research_group", "research group", 0, 1, 100, accounts);
        auto& research = research_create(0, "name","abstract", "permlink", 1, 10, DROPOUT_COMPENSATION_IN_PERCENT);

        const std::string exclude_member_json = "{\"name\":\"bob\",\"research_group_id\": 1}";
        create_proposal(1, dbs_proposal::action_t::dropout_member, exclude_member_json, "alice", 1, time_point_sec(0xffffffff), 1);

        vote_proposal_operation op;

        op.research_group_id = 1;
        op.proposal_id = 1;
        op.voter = "alice";

        evaluator.do_apply(op);

        auto& research_group = research_group_service.get_research_group(1);
        auto& research_token_service = db.obtain_service<dbs_research_token>();
        auto& research_token = research_token_service.get_research_token_by_account_name_and_research_id("bob", research.id);

        BOOST_CHECK_THROW(research_group_service.get_research_group_token_by_account_and_research_group_id("bob", 1), std::out_of_range);
        BOOST_CHECK(research_group.total_tokens_amount == 200);
        BOOST_CHECK(research_token.account_name == "bob");
        BOOST_CHECK(research_token.amount == 499);
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
        create_proposal(1, dbs_proposal::action_t::change_quorum, change_quorum_json, "alice", 1, time_point_sec(0xffffffff), 1);

        vote_proposal_operation op;

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

    create_disciplines();

    std::vector<account_name_type> accounts = {"alice"};
    setup_research_group(1, "research_group", "research group", 0, 1, 100, accounts);
    const std::string json_str = "{\"name\":\"test\","
            "\"research_group_id\":1,"
            "\"abstract\":\"abstract\","
            "\"permlink\":\"permlink\","
            "\"review_share_in_percent\": 10,"
            "\"dropout_compensation_in_percent\": 1500,"
            "\"disciplines\": [1, 2, 3]}";

    create_proposal(1, dbs_proposal::action_t::start_research, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    vote_proposal_operation op;
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
    BOOST_CHECK(research.review_share_in_percent == 10);
    BOOST_CHECK(research.dropout_compensation_in_percent == DROPOUT_COMPENSATION_IN_PERCENT);

    auto& research_discipline_relation_service = db.obtain_service<dbs_research_discipline_relation>();
    auto relations = research_discipline_relation_service.get_research_discipline_relations_by_research(0);

    BOOST_CHECK(relations.size() == 3);

    BOOST_CHECK(std::any_of(relations.begin(), relations.end(),
                            [](std::reference_wrapper<const research_discipline_relation_object> wrapper) {
                                const research_discipline_relation_object& research_discipline_relation = wrapper.get();
                                return research_discipline_relation.id == 0
                                       && research_discipline_relation.research_id == 0
                                       && research_discipline_relation.discipline_id == 1;
                            }));

    BOOST_CHECK(std::any_of(relations.begin(), relations.end(),
                            [](std::reference_wrapper<const research_discipline_relation_object> wrapper) {
                                const research_discipline_relation_object& research_discipline_relation = wrapper.get();
                                return research_discipline_relation.id == 1
                                       && research_discipline_relation.research_id == 0
                                       && research_discipline_relation.discipline_id == 2;
                            }));

    BOOST_CHECK(std::any_of(relations.begin(), relations.end(),
                            [](std::reference_wrapper<const research_discipline_relation_object> wrapper) {
                                const research_discipline_relation_object& research_discipline_relation = wrapper.get();
                                return research_discipline_relation.id == 2
                                       && research_discipline_relation.research_id == 0
                                       && research_discipline_relation.discipline_id == 3;
                            }));

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

    auto& research = research_create(0, "name","abstract", "permlink", 1, 10, DROPOUT_COMPENSATION_IN_PERCENT);
    create_proposal(1, dbs_proposal::action_t::transfer_research_tokens, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    vote_proposal_operation op;
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

    create_proposal(1, dbs_proposal::action_t::send_funds, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    vote_proposal_operation op;
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

BOOST_AUTO_TEST_CASE(rebalance_research_group_tokens_execute_test)
{
    ACTORS((alice)(bob))
    std::vector<account_name_type> accounts = {"alice", "bob"};
    setup_research_group(1, "research_group", "research group", 750, 1, 100, accounts);
    const std::string json_str = "{\"research_group_id\":1,"
            "\"accounts\":[{"
            "\"account_name\":\"alice\","
            "\"amount\":\"50\" },"
            "{"
            "\"account_name\":\"bob\","
            "\"amount\":\"25\""
            "}]}";
    create_proposal(1, dbs_proposal::action_t::rebalance_research_group_tokens, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    vote_proposal_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    evaluator.do_apply(op);


    auto& research_group_service = db.obtain_service<dbs_research_group>();
    auto& alice_token = research_group_service.get_research_group_token_by_account_and_research_group_id("alice", 1);
    auto& bobs_token = research_group_service.get_research_group_token_by_account_and_research_group_id("bob", 1);

    BOOST_CHECK(alice_token.amount == 150);
    BOOST_CHECK(bobs_token.amount == 125);
}

BOOST_AUTO_TEST_CASE(research_token_sale_execute_test)
{
    try
    {
    ACTORS((alice))
    std::vector<account_name_type> accounts = {"alice"};
    setup_research_group(1, "research_group", "research group", 0, 1, 100, accounts);
    const std::string json_str = "{\"research_id\":0,\"amount_for_sale\":90,\"start_time\":\"2020-02-08T16:00:54\",\"end_time\":\"2020-03-08T15:02:31\",\"soft_cap\":60,\"hard_cap\":90}";

    create_proposal(1, dbs_proposal::action_t::start_research_token_sale, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    auto& research = research_create(0, "name","abstract", "permlink", 1, 10, DROPOUT_COMPENSATION_IN_PERCENT);
    auto& research_token_sale_service = db.obtain_service<dbs_research_token_sale>();

    vote_proposal_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    evaluator.do_apply(op);

    auto& research_token_sale = research_token_sale_service.get_research_token_sale_by_research_id(0);

    BOOST_CHECK(research_token_sale.research_id == 0);
    BOOST_CHECK(research_token_sale.start_time == fc::time_point_sec(1581177654));
    BOOST_CHECK(research_token_sale.end_time == fc::time_point_sec(1583679751));
    BOOST_CHECK(research_token_sale.total_amount == 0);
    BOOST_CHECK(research_token_sale.balance_tokens == 90);
    BOOST_CHECK(research_token_sale.soft_cap == 60);
    BOOST_CHECK(research_token_sale.hard_cap == 90);
    BOOST_CHECK(research.owned_tokens == 9910);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(invite_member_data_validate_test)
{
    const std::string json_str = "{\"name\":\"\",\"research_group_id\":1,\"research_group_token_amount\":1000}";
    create_proposal(1, dbs_proposal::action_t::invite_member, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    vote_proposal_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(exclude_member_data_validate_test)
{
    try
    {
        const std::string exclude_member_json = "{\"name\":\"\",\"research_group_id\": 1}";
        create_proposal(1, dbs_proposal::action_t::dropout_member, exclude_member_json, "alice", 1, time_point_sec(0xffffffff), 1);

        vote_proposal_operation op;

        op.research_group_id = 1;
        op.proposal_id = 1;
        op.voter = "alice";

        BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_research_review_share_data_validate_test)
{
    try
    {
        ACTORS((alice));

        const std::string create_research_proposal_json = "{\"name\":\"testresearch\","
                                                          "\"research_group_id\":1,"
                                                          "\"abstract\":\"abstract\","
                                                          "\"permlink\":\"permlink\","
                                                          "\"review_share_in_percent\": 10,"
                                                          "\"dropout_compensation_in_percent\": 1500,"
                                                          "\"disciplines\": [1, 2, 3]}";
        const std::string change_review_share_proposal_json = "{\"review_share_in_percent\": 5100,\"research_id\": 0}";

        db.obtain_service<dbs_research_group>();
        vector<account_name_type> accounts = { "alice" };
        setup_research_group(1, "research_group", "research group", 0, 1, 100, accounts);
        create_disciplines();

        create_proposal(1, dbs_proposal::action_t::start_research, create_research_proposal_json, "alice", 1, fc::time_point_sec(0xffffffff),
                        1);

        vote_proposal_operation start_research_vote_op;
        start_research_vote_op.research_group_id = 1;
        start_research_vote_op.proposal_id = 1;
        start_research_vote_op.voter = "alice";

        evaluator.do_apply(start_research_vote_op);

        create_proposal(2, dbs_proposal::action_t::change_research_review_share_percent, change_review_share_proposal_json, "alice", 1, time_point_sec(0xffffffff), 1);

        vote_proposal_operation change_review_share_vote_op;

        change_review_share_vote_op.research_group_id = 1;
        change_review_share_vote_op.proposal_id = 2;
        change_review_share_vote_op.voter = "alice";

        BOOST_CHECK_THROW(evaluator.do_apply(change_review_share_vote_op), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_quorum_data_validate_test)
{
    try
    {
        const std::string change_quorum_json = "{\"quorum_percent\": 1000,\"research_group_id\": 1}";
        create_proposal(1, dbs_proposal::action_t::change_quorum, change_quorum_json, "alice", 1, time_point_sec(0xffffffff), 1);

        vote_proposal_operation op;

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
            "\"review_share_in_percent\": 5,"
            "\"dropout_compensation_in_percent\": 1500}";
    create_proposal(1, dbs_proposal::action_t::start_research, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    vote_proposal_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(transfer_research_tokens_data_validate_test)
{
    const std::string json_str = "{\"research_id\":0,"
            "\"total_price\":500,"
            "\"account_name\":\"bob\","
            "\"amount\": 5}";
    create_proposal(1, dbs_proposal::action_t::transfer_research_tokens, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    vote_proposal_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(send_funds_data_validate_test)
{
    const std::string json_str = "{\"research_group_id\":1,"
            "\"account_name\":\"bob\","
            "\"funds\": 250}";
    create_proposal(1, dbs_proposal::action_t::transfer_research_tokens, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    vote_proposal_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(rebalance_research_group_tokens_data_validate_test)
{
    const std::string json_str = "{\"research_group_id\":1,"
            "\"account_name\":\"bob\","
            "\"funds\": 250}";
    create_proposal(1, dbs_proposal::action_t::rebalance_research_group_tokens, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    vote_proposal_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(research_token_sale_data_validate_test)
{
    try
    {
    ACTORS((alice))
    std::vector<account_name_type> accounts = {"alice"};
    setup_research_group(1, "research_group", "research group", 0, 1, 100, accounts);

    // TODO: Add check for every value
    const std::string json_str = "{\"research_id\":0,\"amount_for_sale\":9999999999,\"start_time\":\"2020-02-08T15:02:31\",\"end_time\":\"2020-01-08T15:02:31\",\"soft_cap\":9999999999,\"hard_cap\":9999994444}";

    create_proposal(1, dbs_proposal::action_t::start_research_token_sale, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);
    research_create(0, "name","abstract", "permlink", 1, 10, DROPOUT_COMPENSATION_IN_PERCENT);

    vote_proposal_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    BOOST_CHECK_THROW(evaluator.do_apply(op), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(create_research_material)
{
    ACTORS((alice))
    std::vector<account_name_type> accounts = {"alice"};
    setup_research_group(1, "research_group", "research group", 0, 1, 100, accounts);

    db.create<research_object>([&](research_object& r) {
        r.id = 1;
        r.name = "Research #1";
        r.permlink = "Research #1 permlink";
        r.research_group_id = 1;
        r.review_share_in_percent = 10;
        r.dropout_compensation_in_percent = DROPOUT_COMPENSATION_IN_PERCENT;
        r.is_finished = false;
        r.created_at = db.head_block_time();
        r.abstract = "abstract for Research #1";
        r.owned_tokens = DEIP_100_PERCENT;
    });

    const std::string json_str = "{\"research_id\": 1,\"type\": 2,\"content\":\"milestone for Research #1\", \"authors\":[\"alice\"]}";

    create_proposal(1, dbs_proposal::action_t::create_research_material, json_str, "alice", 1, fc::time_point_sec(0xffffffff), 1);

    vote_proposal_operation op;
    op.research_group_id = 1;
    op.proposal_id = 1;
    op.voter = "alice";

    evaluator.do_apply(op);

    auto& research_content_service = db.obtain_service<dbs_research_content>();
    auto contents = research_content_service.get_content_by_research_id(1);

    BOOST_CHECK(contents.size() == 1);
    BOOST_CHECK(std::any_of(
        contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper) {
            const research_content_object& content = wrapper.get();
            return content.id == 0 && content.research_id == 1 && content.type == research_content_type::milestone
                && content.content == "milestone for Research #1" && content.authors.size() == 1
                && content.authors.begin()[0] == "alice";
        }));
}


BOOST_AUTO_TEST_SUITE_END()

}
}