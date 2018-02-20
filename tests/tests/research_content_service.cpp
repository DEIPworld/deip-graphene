#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/research_object.hpp>
#include <deip/chain/research_content_object.hpp>
#include <deip/chain/dbs_research_content.hpp>

#include "database_fixture.hpp"

#define DROPOUT_COMPENSATION_IN_PERCENT 1500

namespace deip {
namespace chain {


class research_content_service_fixture : public clean_database_fixture
{
public:
    research_content_service_fixture()
            : data_service(db.obtain_service<dbs_research_content>())
    {
    }

    void create_researches_with_content()
    {
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

        db.create<research_content_object>([&](research_content_object& rc) {
            rc.id = 0; // id of the first element in index is 0
            rc.research_id = 1;
            rc.type = research_content_type::milestone;
            rc.content = "milestone for Research #1";
            rc.authors = {"alice", "bob"};
            rc.created_at = db.head_block_time();
            rc.research_references = {1};
            rc.research_external_references = {"one", "two", "four"};
        });

        db.create<research_content_object>([&](research_content_object& rc) {
            rc.id = 1;
            rc.research_id = 1;
            rc.type = research_content_type::review;
            rc.content = "review for Research #1";
            rc.authors = {"alice"};
            rc.created_at = db.head_block_time();
            rc.research_references = {1};
            rc.research_external_references = {"one", "four"};
        });

        db.create<research_content_object>([&](research_content_object& rc) {
            rc.id = 2;
            rc.research_id = 1;
            rc.type = research_content_type::final_result;
            rc.content = "final result for Research #1";
            rc.authors = {"bob"};
            rc.created_at = db.head_block_time();
            rc.research_references = {1};
            rc.research_external_references = {"one", "two", "three"};
        });

        db.create<research_object>([&](research_object& r) {
            r.id = 2;
            r.name = "Research #2";
            r.permlink = "permlink for Research #2";
            r.research_group_id = 2;
            r.review_share_in_percent = 10;
            r.dropout_compensation_in_percent = DROPOUT_COMPENSATION_IN_PERCENT;
            r.is_finished = false;
            r.created_at = db.head_block_time();
            r.abstract = "abstract for research #2";
            r.owned_tokens = DEIP_100_PERCENT;
        });

        db.create<research_content_object>([&](research_content_object& rc) {
            rc.id = 3;
            rc.research_id = 2;
            rc.type = research_content_type::announcement;
            rc.content = "announcement for Research #2";
            rc.authors = {"john"};
            rc.created_at = db.head_block_time();
            rc.research_references = {1};
            rc.research_external_references = {"one", "two"};
        });
    }

    dbs_research_content& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_content_service_tests, research_content_service_fixture)

BOOST_AUTO_TEST_CASE(get_content_by_id)
{
    try
    {
        create_researches_with_content();
        auto announcement = data_service.get_content_by_id(3);

        BOOST_CHECK(announcement.id == 3);
        BOOST_CHECK(announcement.research_id == 2);
        BOOST_CHECK(announcement.type == research_content_type::announcement);
        BOOST_CHECK(announcement.content == "announcement for Research #2");
        BOOST_CHECK(announcement.authors.size() == 1);
        BOOST_CHECK(announcement.authors.begin()[0] == "john");
        BOOST_CHECK(announcement.research_references.size() == 1);
        BOOST_CHECK(announcement.research_external_references.size() == 2);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_no_content_by_non_existing_id)
{
    try
    {
        create_researches_with_content();
        BOOST_CHECK_THROW(data_service.get_content_by_id(10), fc::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_content_by_research_id)
{
    try
    {
        create_researches_with_content();
        auto contents = data_service.get_content_by_research_id(1);

        BOOST_CHECK(contents.size() == 3);
        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            return content.id == 0 && content.research_id == 1 && 
                    content.type == research_content_type::milestone &&
                    content.content == "milestone for Research #1" &&
                    content.authors.size() == 2 && 
                    content.authors.begin()[0] == "alice" && content.authors.begin()[1] == "bob" &&
                    content.research_references.size() == 1 &&
                    content.research_external_references.size() == 3;

        }));
        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            return content.id == 1 && content.research_id == 1 && 
                    content.type == research_content_type::review && 
                    content.content == "review for Research #1" &&
                    content.authors.size() == 1 && 
                    content.authors.begin()[0] == "alice" &&
                    content.research_references.size() == 1 &&
                    content.research_external_references.size() == 2;
        }));
        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            return content.id == 2 && content.research_id == 1 && 
                    content.type == research_content_type::final_result && 
                    content.content == "final result for Research #1" &&
                    content.authors.size() == 1 && 
                    content.authors.begin()[0] == "bob" &&
                    content.research_references.size() == 1 &&
                    content.research_external_references.size() == 3;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_no_content_for_non_existing_research_by_id)
{
    try
    {
        create_researches_with_content();
        auto contents = data_service.get_content_by_research_id(3);
        BOOST_CHECK(contents.size() == 0);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_content_by_research_id_and_content_type)
{
    try
    {
        create_researches_with_content();
        auto contents = data_service.get_content_by_research_id_and_content_type(1, research_content_type::review);

        BOOST_CHECK(contents.size() == 1);
        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            return content.id == 1 && content.research_id == 1 && 
                    content.type == research_content_type::review && 
                    content.content == "review for Research #1" && 
                    content.authors.size() == 1 && 
                    content.authors.begin()[0] == "alice" &&
                    content.research_references.size() == 1 &&
                    content.research_external_references.size() == 2;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_no_content_for_non_existing_research_by_id_and_content_type)
{
    try
    {
        create_researches_with_content();
        auto contents = data_service.get_content_by_research_id_and_content_type(3, research_content_type::review);
        BOOST_CHECK(contents.size() == 0);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_research_content)
{
    try
    {
        create_researches_with_content();
        research_content_type type = research_content_type::milestone;
        std::string content = "milestone for Research #2";

        flat_set<account_name_type> authors = {"sam"};
        std::vector<research_id_type> references = {1, 2, 3};
        std::vector<string> external_references = {"one", "two", "three"};

        auto milestone = data_service.create(2, type, content, authors, references, external_references);
        BOOST_CHECK(milestone.research_id == 2);
        BOOST_CHECK(milestone.type == research_content_type::milestone);
        BOOST_CHECK(milestone.content == "milestone for Research #2");
        BOOST_CHECK(milestone.authors.size() == 1);
        BOOST_CHECK(milestone.authors.begin()[0] == "sam");
        BOOST_CHECK(milestone.research_references.size() == 3);
        BOOST_CHECK(milestone.research_external_references.size() == 3);



        auto db_milestone = db.get<research_content_object, by_id>(milestone.id);
        BOOST_CHECK(db_milestone.research_id == 2);
        BOOST_CHECK(db_milestone.type == research_content_type::milestone);
        BOOST_CHECK(db_milestone.content == "milestone for Research #2"); 
        BOOST_CHECK(db_milestone.authors.size() == 1);
        BOOST_CHECK(db_milestone.authors.begin()[0] == "sam");
        BOOST_CHECK(db_milestone.research_references.size() == 3);
        BOOST_CHECK(db_milestone.research_external_references.size() == 3);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
