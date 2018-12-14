#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/schema/research_content_object.hpp>
#include <deip/chain/services/dbs_research_content.hpp>

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
            rc.type = research_content_type::milestone_data;
            rc.title = "title for milestone for Research #1";
            rc.content = "milestone for Research #1";
            rc.permlink = "milestone-research-one";
            rc.authors = {"alice", "bob"};
            rc.created_at = db.head_block_time();
            rc.references.insert(2);
            rc.external_references = {"one", "two", "four"};
        });

        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 1;
            rc.research_id = 1;
            rc.type = research_content_type::milestone_data;
            rc.title = "title for milestone for Research #1";
            rc.content = "milestone for Research #1";
            rc.permlink = "another-milestone-research-one";
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
            rc.permlink = "final-research-one";
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
            rc.permlink = "announcement-research-two";
            rc.authors = {"john"};
            rc.created_at = db.head_block_time();
            rc.references.insert(1);
            rc.external_references = {"one", "two"};
        });
    }

    void create_contents()
    {
        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 0; // id of the first element in index is 0
            rc.research_id = 1;
            rc.type = research_content_type::milestone_data;
            rc.title = "title for milestone for Research #1";
            rc.content = "milestone for Research #1";
            rc.permlink = "milestone-research-one";
            rc.authors = {"alice", "bob"};
            rc.created_at = db.head_block_time();
            rc.references.insert(2);
            rc.external_references = {"one", "two", "four"};
        });

        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 1;
            rc.research_id = 1;
            rc.type = research_content_type::milestone_book;
            rc.title = "title for milestone for Research #1";
            rc.content = "milestone for Research #1";
            rc.permlink = "another-milestone-research-one";
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
            rc.permlink = "final-research-one";
            rc.authors = {"bob"};
            rc.created_at = db.head_block_time();
            rc.references.insert(2);
            rc.external_references = {"one", "two", "three"};
        });

        db.create<research_content_object>([&](research_content_object& rc) {

            rc.id = 3;
            rc.research_id = 2;
            rc.type = research_content_type::announcement;
            rc.title = "title for announcement for Research #2";
            rc.content = "announcement for Research #2";
            rc.permlink = "announcement-research-two";
            rc.authors = {"john"};
            rc.created_at = db.head_block_time();
            rc.references.insert(1);
            rc.external_references = {"one", "two"};
        });
    }

    void create_grant_applications()
    {
        db.create<grant_application_object>([&](grant_application_object& ga_o) {
            ga_o.id = 0;
            ga_o.grant_id = 1;
            ga_o.research_id = 1;
            ga_o.creator = "alice";
            ga_o.application_hash = "test1";
        });

        db.create<grant_application_object>([&](grant_application_object& ga_o) {
            ga_o.id = 1;
            ga_o.grant_id = 1;
            ga_o.research_id = 2;
            ga_o.creator = "bob";
            ga_o.application_hash = "test2";
        });

        db.create<grant_application_object>([&](grant_application_object& ga_o) {
            ga_o.id = 2;
            ga_o.grant_id = 2;
            ga_o.research_id = 2;
            ga_o.creator = "john";
            ga_o.application_hash = "test3";
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
        auto announcement = data_service.get(3);
        
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
        BOOST_CHECK_THROW(data_service.get(10), fc::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_content_by_research_id)
{
    try
    {
        create_researches_with_content();
        auto contents = data_service.get_by_research_id(1);

        BOOST_CHECK(contents.size() == 3);
        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            std::vector<string> authors;
            for (auto author : content.authors)
                authors.push_back(author);

            return content.id == 0 && content.research_id == 1 && 
                    content.type == research_content_type::milestone_data &&
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
                    content.type == research_content_type::milestone_data &&
                    content.title == "title for milestone for Research #1" &&
                    content.content == "milestone for Research #1" &&
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
        auto contents = data_service.get_by_research_id(3);
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

            return content.id == 3 && content.research_id == 2 &&
                    content.type == research_content_type::announcement &&
                    content.title == "title for announcement for Research #2" &&
                    content.content == "announcement for Research #2" &&
                    content.authors.size() == 1 && 
                    authors[0] == "john" &&
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
        auto contents = data_service.get_by_research_and_type(3, research_content_type::milestone_article);
        BOOST_CHECK(contents.size() == 0);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_research_content)
{
    try
    {
        create_researches_with_content();
        research_content_type type = research_content_type::milestone_data;

        std::string title = "title for milestone for Research #2";
        std::string content = "milestone for Research #2";

        std::vector<account_name_type> authors = {"sam"};

        std::vector<research_content_id_type> research_references;
        research_references.push_back(1);
        std::vector<string> external_references = {"one", "two", "three"};

        auto milestone = data_service.create(2, type, title, content, "permlink", authors, research_references, external_references);
        BOOST_CHECK(milestone.research_id == 2);
        BOOST_CHECK(milestone.type == research_content_type::milestone_data);
        BOOST_CHECK(milestone.title == "title for milestone for Research #2");
        BOOST_CHECK(milestone.content == "milestone for Research #2");
        BOOST_CHECK(milestone.permlink == "permlink");
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


BOOST_AUTO_TEST_CASE(get_all_milestones_by_research_id)
{
    try
    {
        create_contents();
        auto contents = data_service.get_all_milestones_by_research_id(1);

        BOOST_CHECK(contents.size() == 2);
        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            std::vector<string> authors;
            for (auto author : content.authors)
                authors.push_back(author);

            return content.id == 0 && content.research_id == 1 &&
                    content.type == research_content_type::milestone_data &&
                    content.title == "title for milestone for Research #1" &&
                    content.content == "milestone for Research #1" &&
                    content.authors.size() == 2 &&
                    authors[0] == "alice" &&
                    authors[1] == "bob" &&
                    content.references.size() == 1 &&
                    content.external_references.size() == 3;
        }));

        BOOST_CHECK(std::any_of(contents.begin(), contents.end(), [](std::reference_wrapper<const research_content_object> wrapper){
            const research_content_object &content = wrapper.get();
            std::vector<string> authors;
            for (auto author : content.authors)
                authors.push_back(author);

            return content.id == 1 && content.research_id == 1 &&
                   content.type == research_content_type::milestone_book &&
                   content.title =="title for milestone for Research #1" &&
                   content.content == "milestone for Research #1" &&
                   content.authors.size() == 1 &&
                   authors[0] == "alice" &&
                   content.references.size() == 1 &&
                   content.external_references.size() == 2;
        }));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_grant_application)
{
    try
    {
        auto& grant_application = data_service.create_grant_application(1, 1, "test", "alice");
        BOOST_CHECK(grant_application.grant_id == 1);
        BOOST_CHECK(grant_application.research_id == 1);
        BOOST_CHECK(grant_application.creator == "alice");
        BOOST_CHECK(grant_application.application_hash == "test");
        BOOST_CHECK(grant_application.created_at == db.head_block_time());
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_grant_application)
{
    try
    {
        create_grant_applications();

        auto& grant_application = data_service.get_grant_application(1);

        BOOST_CHECK(grant_application.grant_id == 1);
        BOOST_CHECK(grant_application.research_id == 2);
        BOOST_CHECK(grant_application.creator == "bob");
        BOOST_CHECK(grant_application.application_hash == "test2");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_applications_by_grant)
{
    try
    {
        create_grant_applications();
        auto applications = data_service.get_applications_by_grant(1);

        BOOST_CHECK(applications.size() == 2);
        BOOST_CHECK(std::any_of(applications.begin(), applications.end(), [](std::reference_wrapper<const grant_application_object> wrapper){
            const grant_application_object &application = wrapper.get();

            return application.id == 0 && application.research_id == 1 &&
                    application.grant_id == 1 &&
                    application.creator == "alice" &&
                    application.application_hash == "test1";
        }));

        BOOST_CHECK(std::any_of(applications.begin(), applications.end(), [](std::reference_wrapper<const grant_application_object> wrapper){
            const grant_application_object &application = wrapper.get();

            return application.id == 1 && application.research_id == 2 &&
                   application.grant_id == 1 &&
                   application.creator == "bob" &&
                   application.application_hash == "test2";
        }));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_applications_by_research_id)
{
    try
    {
        create_grant_applications();
        auto applications = data_service.get_applications_by_research_id(2);

        BOOST_CHECK(applications.size() == 2);

        BOOST_CHECK(std::any_of(applications.begin(), applications.end(), [](std::reference_wrapper<const grant_application_object> wrapper){
            const grant_application_object &application = wrapper.get();

            return application.id == 1 && application.research_id == 2 &&
                   application.grant_id == 1 &&
                   application.creator == "bob" &&
                   application.application_hash == "test2";
        }));

        BOOST_CHECK(std::any_of(applications.begin(), applications.end(), [](std::reference_wrapper<const grant_application_object> wrapper){
            const grant_application_object &application = wrapper.get();

            return application.id == 2 && application.research_id == 2 &&
                   application.grant_id == 2 &&
                   application.creator == "john" &&
                   application.application_hash == "test3";
        }));

    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(delete_appication_by_id)
{
    try
    {
        create_grant_applications();
        BOOST_CHECK_NO_THROW(data_service.delete_appication_by_id(1));
        BOOST_CHECK_THROW(db.get<grant_application_object>(1), std::out_of_range);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif