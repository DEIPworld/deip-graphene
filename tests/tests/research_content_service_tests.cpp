#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/schema/research_content_object.hpp>
#include <deip/chain/services/dbs_research_content.hpp>

#include "database_fixture.hpp"

#define RESEARCH_ID_1_REVIEW_SHARE_IN_PERCENT 10
#define RESEARCH_ID_2_REVIEW_SHARE_IN_PERCENT 20
#define COMPENSATION_IN_PERCENT 1500

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
            r.description = "Research #1";
            r.research_group_id = 1;
            r.is_finished = false;
            r.created_at = db.head_block_time();
            r.abstract = "abstract for Research #1";
        });

        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 0; // id of the first element in index is 0
            rc.research_id = 1;
            rc.type = research_content_type::milestone_data;
            rc.description = "title for milestone for Research #1";
            rc.content = "milestone for Research #1";
            rc.authors = {"alice", "bob"};
            rc.created_at = db.head_block_time();
        });

        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 1;
            rc.research_id = 1;
            rc.type = research_content_type::milestone_data;
            rc.description = "title for milestone for Research #1";
            rc.content = "milestone for Research #1";
            rc.authors = {"alice"};
            rc.created_at = db.head_block_time();
        });

        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 2;
            rc.research_id = 1;
            rc.type = research_content_type::final_result;
            rc.description = "title for final result for Research #1";
            rc.content = "final result for Research #1";
            rc.authors = {"bob"};
            rc.created_at = db.head_block_time();
        });

        db.create<research_object>([&](research_object& r) {

            r.id = 2;
            r.description = "Research #2";
            r.research_group_id = 2;
            r.is_finished = false;
            r.created_at = db.head_block_time();
            r.abstract = "abstract for research #2";
        });

        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 3;
            rc.research_id = 2;
            rc.type = research_content_type::announcement;
            rc.description = "title for announcement for Research #2";
            rc.content = "announcement for Research #2";
            rc.authors = {"john"};
            rc.created_at = db.head_block_time();
        });
    }

    void create_contents()
    {
        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 0; // id of the first element in index is 0
            rc.research_id = 1;
            rc.type = research_content_type::milestone_data;
            rc.description = "title for milestone for Research #1";
            rc.content = "milestone for Research #1";
            rc.authors = {"alice", "bob"};
            rc.created_at = db.head_block_time();
        });

        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 1;
            rc.research_id = 1;
            rc.type = research_content_type::milestone_book;
            rc.description = "title for milestone for Research #1";
            rc.content = "milestone for Research #1";
            rc.authors = {"alice"};
            rc.created_at = db.head_block_time();
        });

        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 2;
            rc.research_id = 1;
            rc.type = research_content_type::final_result;
            rc.description = "title for final result for Research #1";
            rc.content = "final result for Research #1";
            rc.authors = {"bob"};
            rc.created_at = db.head_block_time();
        });

        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 3;
            rc.research_id = 2;
            rc.type = research_content_type::announcement;
            rc.description = "title for announcement for Research #2";
            rc.content = "announcement for Research #2";
            rc.authors = {"john"};
            rc.created_at = db.head_block_time();
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
        auto announcement = data_service.get_research_content(3);
        
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
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_content_by_research_id)
{
    try
    {
        create_researches_with_content();
        auto contents = data_service.get_research_content_by_research_id(1);

        BOOST_CHECK(contents.size() == 3);
        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            std::vector<string> authors;
            for (auto author : content.authors)
                authors.push_back(author);

            return content.id == 0 && content.research_id == 1 && content.type == research_content_type::milestone_data
                && content.description == "title for milestone for Research #1"
                && content.content == "milestone for Research #1" && content.authors.size() == 2
                && authors[0] == "alice" && authors[1] == "bob" && content.references.size() == 1;
        }));
        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            std::vector<string> authors;
            for (auto author : content.authors)
                authors.push_back(author);

            return content.id == 1 && content.research_id == 1 && content.type == research_content_type::milestone_data
                && content.description == "title for milestone for Research #1"
                && content.content == "milestone for Research #1" && content.authors.size() == 1
                && authors[0] == "alice" && content.references.size() == 1;
        }));
        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            std::vector<string> authors;
            for (auto author : content.authors)
                authors.push_back(author);

            return content.id == 2 && content.research_id == 1 && content.type == research_content_type::final_result
                && content.description == "title for final result for Research #1"
                && content.content == "final result for Research #1" && content.authors.size() == 1
                && authors[0] == "bob" && content.references.size() == 1;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_no_content_for_non_existing_research_by_id)
{
    try
    {
        create_researches_with_content();
        auto contents = data_service.get_research_content_by_research_id(3);
        BOOST_CHECK(contents.size() == 0);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_content_by_research_id_and_content_type)
{
    try
    {
        create_researches_with_content();
        auto contents = data_service.get_by_research_and_type(2, research_content_type::announcement);

        BOOST_CHECK(contents.size() == 1);
        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            std::vector<string> authors;
            for (auto author : content.authors)
                authors.push_back(author);

            return content.id == 3 && content.research_id == 2 && content.type == research_content_type::announcement
                && content.description == "title for announcement for Research #2"
                && content.content == "announcement for Research #2" && content.authors.size() == 1
                && authors[0] == "john" && content.references.size() == 1;
        }));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_no_content_for_non_existing_research_by_id_and_content_type)
{
    try
    {
        create_researches_with_content();
        auto contents = data_service.get_by_research_and_type(3, research_content_type::milestone_article);
        BOOST_CHECK(contents.size() == 0);
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