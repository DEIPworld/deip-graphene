#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/research_object.hpp>
#include <deip/chain/dbs_research.hpp>

#include "database_fixture.hpp"

#define RESEARCH_TITLE "RESEARCH"
#define DISCIPLINE_MATH 10
#define DISCIPLINE_PHYSICS 20
#define RESEARCH_GROUP_ID 1
#define REVIEW_SHARE 1000
#define ABSTRACT "ABSTRACT"
#define DROPOUT_COMPENSATION 1500

namespace deip {
namespace chain {

class research_service_fixture : public clean_database_fixture
{
public:
    research_service_fixture()
            : data_service(db.obtain_service<dbs_research>())
    {
    }

    void create_researches() {
        db.create<research_object>([&](research_object& r) {
            r.id = 1;
            r.title = RESEARCH_TITLE;
            r.permlink = RESEARCH_TITLE;
            r.research_group_id = RESEARCH_GROUP_ID;
            r.review_share = 1000;
            r.dropout_compensation = DROPOUT_COMPENSATION;
            r.is_finished = false;
            r.created_at = db.head_block_time();
            r.abstract = ABSTRACT;
            r.owned_tokens = DEIP_100_PERCENT;
        });

        db.create<research_object>([&](research_object& r) {
            r.id = 2;
            r.permlink = "Second";
            r.research_group_id = 2;
            r.review_share = 1000;
            r.dropout_compensation = DROPOUT_COMPENSATION;
            r.is_finished = false;
            r.created_at = db.head_block_time();
            r.abstract = ABSTRACT;
            r.owned_tokens = DEIP_100_PERCENT;
        });

        db.create<research_object>([&](research_object& r) {
            r.id = 3;
            r.permlink = "Third";
            r.research_group_id = 2;
            r.review_share = 1000;
            r.dropout_compensation = DROPOUT_COMPENSATION;
            r.is_finished = false;
            r.created_at = db.head_block_time();
            r.abstract = ABSTRACT;
            r.owned_tokens = DEIP_100_PERCENT;
        });
    }

    dbs_research& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_service_tests, research_service_fixture)

BOOST_AUTO_TEST_CASE(create_research)
{
    try
    {
        auto& research = data_service.create(RESEARCH_TITLE, ABSTRACT, RESEARCH_TITLE, RESEARCH_GROUP_ID, REVIEW_SHARE, DROPOUT_COMPENSATION);

        BOOST_CHECK(research.title == RESEARCH_TITLE);
        BOOST_CHECK(research.permlink == RESEARCH_TITLE);
        BOOST_CHECK(research.research_group_id == RESEARCH_GROUP_ID);
        BOOST_CHECK(research.review_share == REVIEW_SHARE);
        BOOST_CHECK(research.dropout_compensation == DROPOUT_COMPENSATION);
        BOOST_CHECK(research.is_finished == false);
        BOOST_CHECK(research.created_at <= db.head_block_time());
        BOOST_CHECK(research.abstract == ABSTRACT);
        BOOST_CHECK(research.owned_tokens == DEIP_100_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_researches)
{
    try
    {
        create_researches();

        const auto& researches = data_service.get_researches();

        BOOST_CHECK(researches.size() == 3);

        BOOST_CHECK(std::any_of(researches.begin(), researches.end(), [](std::reference_wrapper<const research_object> wrapper){
            const research_object &research = wrapper.get();
            return  research.id == 1 &&
                    research.permlink == RESEARCH_TITLE &&
                    research.research_group_id == RESEARCH_GROUP_ID &&
                    research.review_share == 1000 &&
                    research.dropout_compensation == DROPOUT_COMPENSATION &&
                    research.is_finished == false &&
                    research.abstract == ABSTRACT &&
                    research.owned_tokens == DEIP_100_PERCENT;
        }));

        BOOST_CHECK(std::any_of(researches.begin(), researches.end(), [](std::reference_wrapper<const research_object> wrapper){
            const research_object &research = wrapper.get();
            return  research.id == 2 &&
                    research.permlink == "Second" &&
                    research.research_group_id == 2 &&
                    research.review_share == 1000 &&
                    research.dropout_compensation == DROPOUT_COMPENSATION &&
                    research.is_finished == false &&
                    research.abstract == ABSTRACT &&
                    research.owned_tokens == DEIP_100_PERCENT;
        }));

        BOOST_CHECK(std::any_of(researches.begin(), researches.end(), [](std::reference_wrapper<const research_object> wrapper){
            const research_object &research = wrapper.get();
            return  research.id == 3 &&
                    research.permlink == "Third" &&
                    research.research_group_id == 2 &&
                    research.review_share == 1000 &&
                    research.dropout_compensation == DROPOUT_COMPENSATION &&
                    research.is_finished == false &&
                    research.abstract == ABSTRACT &&
                    research.owned_tokens == DEIP_100_PERCENT;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_researches_by_research_group)
{
    try
    {
        create_researches();

        const auto& researches = data_service.get_researches_by_research_group(2);

        BOOST_CHECK(researches.size() == 2);

        BOOST_CHECK(std::any_of(researches.begin(), researches.end(), [](std::reference_wrapper<const research_object> wrapper){
            const research_object &research = wrapper.get();
            return  research.id == 2 &&
                    research.permlink == "Second" &&
                    research.research_group_id == 2 &&
                    research.review_share == 1000 &&
                    research.dropout_compensation == DROPOUT_COMPENSATION &&
                    research.is_finished == false &&
                    research.abstract == ABSTRACT &&
                    research.owned_tokens == DEIP_100_PERCENT;
        }));

        BOOST_CHECK(std::any_of(researches.begin(), researches.end(), [](std::reference_wrapper<const research_object> wrapper){
            const research_object &research = wrapper.get();
            return  research.id == 3 &&
                    research.permlink == "Third" &&
                    research.research_group_id == 2 &&
                    research.review_share == 1000 &&
                    research.dropout_compensation == DROPOUT_COMPENSATION &&
                    research.is_finished == false &&
                    research.abstract == ABSTRACT &&
                    research.owned_tokens == DEIP_100_PERCENT;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research)
{
    try
    {
        create_researches();

        auto& research = data_service.get_research(1);

        BOOST_CHECK(research.id == 1);
        BOOST_CHECK(research.title == RESEARCH_TITLE);
        BOOST_CHECK(research.permlink == RESEARCH_TITLE);
        BOOST_CHECK(research.research_group_id == RESEARCH_GROUP_ID);
        BOOST_CHECK(research.review_share == 1000);
        BOOST_CHECK(research.dropout_compensation == DROPOUT_COMPENSATION);
        BOOST_CHECK(research.is_finished == false);
        BOOST_CHECK(research.created_at <= db.head_block_time());
        BOOST_CHECK(research.abstract == ABSTRACT);
        BOOST_CHECK(research.owned_tokens == DEIP_100_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_by_permlink)
{
    try
    {
        create_researches();

        auto& research = data_service.get_research_by_permlink(RESEARCH_TITLE);

        BOOST_CHECK(research.title == RESEARCH_TITLE);
        BOOST_CHECK(research.permlink == RESEARCH_TITLE);
        BOOST_CHECK(research.research_group_id == RESEARCH_GROUP_ID);
        BOOST_CHECK(research.review_share == 1000);
        BOOST_CHECK(research.dropout_compensation == DROPOUT_COMPENSATION);
        BOOST_CHECK(research.is_finished == false);
        BOOST_CHECK(research.created_at <= db.head_block_time());
        BOOST_CHECK(research.abstract == ABSTRACT);
        BOOST_CHECK(research.owned_tokens == DEIP_100_PERCENT);

    }
    FC_LOG_AND_RETHROW()
}
    
BOOST_AUTO_TEST_CASE(decrease_owned_tokens)
{
    try
    {
        create_researches();

        auto& research = db.get<research_object, by_id>(1);
        BOOST_CHECK_NO_THROW(data_service.decrease_owned_tokens(research, 200));
        BOOST_CHECK(research.owned_tokens == 9800);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(increase_owned_tokens)
{
    try
    {
        create_researches();

        auto& research = db.get<research_object, by_id>(1);
        BOOST_CHECK_NO_THROW(data_service.increase_owned_tokens(research, 200));
        BOOST_CHECK(research.owned_tokens == 10200);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_research_review_share_percent)
{
    try
    {
        create_researches();

        auto& research = db.get<research_object, by_id>(1);
        BOOST_CHECK_NO_THROW(data_service.change_research_review_share_percent(1,  4500));
        BOOST_CHECK(research.review_share ==  4500);

    }
    FC_LOG_AND_RETHROW()
}        

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
