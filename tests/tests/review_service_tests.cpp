#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/review_object.hpp>
#include <deip/chain/dbs_review.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class review_service_fixture : public clean_database_fixture
{
public:
    review_service_fixture()
            : data_service(db.obtain_service<dbs_review>())
    {
    }

    void create_reviews() {
        db.create<review_object>([&](review_object& r) {
            r.id = 1,
            r.research_content_id = 1;
            r.content = "Content 1";
            r.author = "alice";
            r.is_positive = true;
            r.created_at = db.head_block_time();
            r.disciplines = {1 , 2, 3};
        });

        db.create<review_object>([&](review_object& r) {
            r.id = 2,
            r.research_content_id = 1;
            r.content = "Content 2";
            r.author = "bob";
            r.is_positive = true;
            r.created_at = db.head_block_time();
            r.disciplines = {1, 3};
        });

        db.create<review_object>([&](review_object& r) {
            r.id = 3,
            r.research_content_id = 2;
            r.content = "Content 3";
            r.author = "alice";
            r.is_positive = true;
            r.created_at = db.head_block_time();
            r.disciplines = {1, 2};
        });
    }

    dbs_review& data_service;
};

BOOST_FIXTURE_TEST_SUITE(review_service, review_service_fixture)

BOOST_AUTO_TEST_CASE(create)
{
    try
    {
        auto& review = data_service.create(1, "content", true, "alice", {1, 2}, {1, 2}, {"test"});

        std::vector<discipline_id_type> disciplines;
        for (auto discipline : review.disciplines)
            disciplines.push_back(discipline);

        BOOST_CHECK(review.research_content_id == 1);
        BOOST_CHECK(review.content == "content");
        BOOST_CHECK(review.is_positive == true);
        BOOST_CHECK(review.author == "alice");
        BOOST_CHECK(review.references.size() == 2);
        BOOST_CHECK(disciplines.size() == 2);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get)
{
    try
    {
        create_reviews();

        auto& review = data_service.get(1);

        std::vector<discipline_id_type> disciplines;
        for (auto discipline : review.disciplines)
            disciplines.push_back(discipline);

        BOOST_CHECK(review.research_content_id == 1);
        BOOST_CHECK(review.content == "Content 1");
        BOOST_CHECK(review.is_positive == true);
        BOOST_CHECK(review.author == "alice");
        BOOST_CHECK(disciplines.size() == 3);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_research_reviews)
{
    try
    {
        create_reviews();

        auto reviews = data_service.get_research_content_reviews(1);

        BOOST_CHECK(reviews.size() == 2);
        BOOST_CHECK(std::any_of(reviews.begin(), reviews.end(), [](std::reference_wrapper<const review_object> wrapper){
            const review_object &review = wrapper.get();
            std::vector<discipline_id_type> disciplines;
            for (auto discipline : review.disciplines)
                disciplines.push_back(discipline);

            return  review.id == 1 &&
                    review.research_content_id == 1 &&
                    review.content == "Content 1" &&
                    review.author == "alice" &&
                    review.is_positive == true &&
                    disciplines[0] == 1 && disciplines[1] == 2 && disciplines[2] == 3;
        }));

        BOOST_CHECK(std::any_of(reviews.begin(), reviews.end(), [](std::reference_wrapper<const review_object> wrapper){
            const review_object &review = wrapper.get();
            std::vector<discipline_id_type> disciplines;
            for (auto discipline : review.disciplines)
                disciplines.push_back(discipline);

            return  review.id == 2 &&
                    review.research_content_id == 1 &&
                    review.content == "Content 2" &&
                    review.author == "bob" &&
                    review.is_positive == true &&
                    disciplines[0] == 1 && disciplines[1] == 3;
        }));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_author_reviews)
{
    try
    {
        create_reviews();

        auto reviews = data_service.get_author_reviews("alice");

        BOOST_CHECK(reviews.size() == 2);
        BOOST_CHECK(std::any_of(reviews.begin(), reviews.end(), [](std::reference_wrapper<const review_object> wrapper){
            const review_object &review = wrapper.get();
            std::vector<discipline_id_type> disciplines;
            for (auto discipline : review.disciplines)
                disciplines.push_back(discipline);

            return  review.id == 1 &&
                    review.research_content_id == 1 &&
                    review.content == "Content 1" &&
                    review.author == "alice" &&
                    review.is_positive == true &&
                    disciplines[0] == 1 && disciplines[1] == 2 && disciplines[2] == 3;
        }));

        BOOST_CHECK(std::any_of(reviews.begin(), reviews.end(), [](std::reference_wrapper<const review_object> wrapper){
            const review_object &review = wrapper.get();
            std::vector<discipline_id_type> disciplines;
            for (auto discipline : review.disciplines)
                disciplines.push_back(discipline);

            return  review.id == 3 &&
                    review.research_content_id == 2 &&
                    review.content == "Content 3" &&
                    review.author == "alice" &&
                    review.is_positive == true &&
                    disciplines[0] == 1 && disciplines[1] == 2;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
