#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/research_object.hpp>
#include <deip/chain/research_content_object.hpp>
#include <deip/chain/dbs_research_content.hpp>

#include "database_fixture.hpp"

#define RESEARCH_ID_1_REVIEW_SHARE_IN_PERCENT 10
#define RESEARCH_ID_2_REVIEW_SHARE_IN_PERCENT 20
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
            r.title = "Research #1";
            r.permlink = "Research #1 permlink";
            r.research_group_id = 1;
            r.review_share_in_percent = RESEARCH_ID_1_REVIEW_SHARE_IN_PERCENT;
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
            rc.title = "title for milestone for Research #1";
            rc.content = "milestone for Research #1";
            rc.authors = {"alice", "bob"};
            rc.created_at = db.head_block_time();
            rc.references.insert(2);
            rc.external_references = {"one", "two", "four"};
        });

        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 1;
            rc.research_id = 1;
            rc.type = research_content_type::review;
            rc.title = "title for review for Research #1";
            rc.content = "review for Research #1";
            rc.authors = {"alice"};
            rc.created_at = db.head_block_time();
            rc.references.insert(2);
            rc.external_references = {"one", "four"};
        });

        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 2;
            rc.research_id = 1;
            rc.type = research_content_type::final_result;
            rc.title = "title for final result for Research #1";
            rc.content = "final result for Research #1";
            rc.authors = {"bob"};
            rc.created_at = db.head_block_time();
            rc.references.insert(2);
            rc.external_references = {"one", "two", "three"};
        });

        db.create<research_object>([&](research_object& r) {

            r.id = 2;
            r.title = "Research #2";
            r.permlink = "permlink for Research #2";
            r.research_group_id = 2;
            r.review_share_in_percent = RESEARCH_ID_2_REVIEW_SHARE_IN_PERCENT;
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
            rc.title = "title for announcement for Research #2";
            rc.content = "announcement for Research #2";
            rc.authors = {"john"};
            rc.created_at = db.head_block_time();
            rc.references.insert(1);
            rc.external_references = {"one", "two"};
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
        
        std::vector<string> authors;
        for (auto author : announcement.authors)
            authors.push_back(author);

        BOOST_CHECK(announcement.id == 3);
        BOOST_CHECK(announcement.research_id == 2);
        BOOST_CHECK(announcement.type == research_content_type::announcement);
        BOOST_CHECK(announcement.content == "announcement for Research #2");
        BOOST_CHECK(announcement.authors.size() == 1);
        BOOST_CHECK(authors[0] == "john");
        BOOST_CHECK(announcement.references.size() == 1);
        BOOST_CHECK(announcement.external_references.size() == 2);
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
            std::vector<string> authors;
            for (auto author : content.authors)
                authors.push_back(author);

            return content.id == 0 && content.research_id == 1 && 
                    content.type == research_content_type::milestone &&
                    content.title == "title for milestone for Research #1" &&
                    content.content == "milestone for Research #1" &&
                    content.authors.size() == 2 &&
                    authors[0] == "alice" && authors[1] == "bob" &&
                    content.references.size() == 1 &&
                    content.external_references.size() == 3;

        }));
        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            std::vector<string> authors;
            for (auto author : content.authors)
                authors.push_back(author);

            return content.id == 1 && content.research_id == 1 && 
                    content.type == research_content_type::review &&
                    content.title == "title for review for Research #1" && 
                    content.content == "review for Research #1" &&
                    content.authors.size() == 1 && 
                    authors[0] == "alice" &&
                    content.references.size() == 1 &&
                    content.external_references.size() == 2;
        }));
        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            std::vector<string> authors;
            for (auto author : content.authors)
                authors.push_back(author);

            return content.id == 2 && content.research_id == 1 && 
                    content.type == research_content_type::final_result && 
                    content.title == "title for final result for Research #1" && 
                    content.content == "final result for Research #1" &&
                    content.authors.size() == 1 && 
                    authors[0] == "bob" &&
                    content.references.size() == 1 &&
                    content.external_references.size() == 3;
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
            std::vector<string> authors;
            for (auto author : content.authors)
                authors.push_back(author);

            return content.id == 1 && content.research_id == 1 && 
                    content.type == research_content_type::review && 
                    content.title == "title for review for Research #1" &&
                    content.content == "review for Research #1" &&
                    content.authors.size() == 1 && 
                    authors[0] == "alice" &&
                    content.references.size() == 1 &&
                    content.external_references.size() == 2;
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

        std::string title = "title for milestone for Research #2";
        std::string content = "milestone for Research #2";

        std::vector<account_name_type> authors = {"sam"};

        std::vector<research_content_id_type> research_references;
        research_references.push_back(1);
        std::vector<string> external_references = {"one", "two", "three"};

        auto milestone = data_service.create(2, type, title, content, authors, research_references, external_references);
        BOOST_CHECK(milestone.research_id == 2);
        BOOST_CHECK(milestone.type == research_content_type::milestone);
        BOOST_CHECK(milestone.title == "title for milestone for Research #2");
        BOOST_CHECK(milestone.content == "milestone for Research #2");
        BOOST_CHECK(milestone.authors.size() == 1);
        BOOST_CHECK(milestone.authors.find("sam") != milestone.authors.end());
        BOOST_CHECK(milestone.references.size() == 1);
        BOOST_CHECK(milestone.external_references.size() == 3);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_research_content_existence)
{
    try
    {
        create_researches_with_content();

        BOOST_CHECK_NO_THROW(data_service.check_research_content_existence(1));
        BOOST_CHECK_THROW(data_service.check_research_content_existence(123), fc::assert_exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif