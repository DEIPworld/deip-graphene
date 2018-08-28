#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/research_content_reward_pool_object.hpp>
#include <deip/chain/services/dbs_research_content_reward_pool.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {


class research_content_reward_pool_service_fixture : public clean_database_fixture
{
public:
    research_content_reward_pool_service_fixture()
            : data_service(db.obtain_service<dbs_research_content_reward_pool>())
    {
    }

    void create_research_content_reward_pools()
    {
        db.create<research_content_reward_pool_object>([&](research_content_reward_pool_object& r) {
            r.id = 1;
            r.discipline_id = 1;
            r.research_content_id = 1;
            r.reward_share = 100;
            r.expertise_share = 100;
        });

        db.create<research_content_reward_pool_object>([&](research_content_reward_pool_object& r) {
            r.id = 2;
            r.discipline_id = 2;
            r.research_content_id = 1;
            r.reward_share = 200;
            r.expertise_share = 200;
        });

        db.create<research_content_reward_pool_object>([&](research_content_reward_pool_object& r) {
            r.id = 3;
            r.discipline_id = 1;
            r.research_content_id = 2;
            r.reward_share = 1500;
            r.expertise_share = 1500;
        });
    }

    dbs_research_content_reward_pool& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_content_reward_pool_service_tests, research_content_reward_pool_service_fixture)

BOOST_AUTO_TEST_CASE(create)
{
    try
    {
        const research_content_reward_pool_object& research_content_reward_pool =
                data_service.create(1, 1, 100, 200);

        BOOST_CHECK(research_content_reward_pool.id == 0);
        BOOST_CHECK(research_content_reward_pool.research_content_id == 1);
        BOOST_CHECK(research_content_reward_pool.discipline_id == 1);
        BOOST_CHECK(research_content_reward_pool.reward_share == 100);
        BOOST_CHECK(research_content_reward_pool.expertise_share == 200);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get)
{
    try
    {
        create_research_content_reward_pools();
        BOOST_CHECK_THROW(data_service.get(10), fc::exception);

        auto& research_content_reward_pool = data_service.get(1);

        BOOST_CHECK(research_content_reward_pool.id == 1);
        BOOST_CHECK(research_content_reward_pool.research_content_id == 1);
        BOOST_CHECK(research_content_reward_pool.discipline_id == 1);
        BOOST_CHECK(research_content_reward_pool.reward_share == 100);
        BOOST_CHECK(research_content_reward_pool.expertise_share == 100);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_by_research_content_id_and_discipline_id)
{
    try
    {
        create_research_content_reward_pools();
        BOOST_CHECK_THROW(data_service.get_by_research_content_id_and_discipline_id(1, 100), fc::exception);

        auto& research_content_reward_pool = data_service.get_by_research_content_id_and_discipline_id(1, 2);

        BOOST_CHECK(research_content_reward_pool.id == 2);
        BOOST_CHECK(research_content_reward_pool.research_content_id == 1);
        BOOST_CHECK(research_content_reward_pool.discipline_id == 2);
        BOOST_CHECK(research_content_reward_pool.reward_share == 200);
        BOOST_CHECK(research_content_reward_pool.expertise_share == 200);
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(get_research_content_reward_pools_by_content_id)
{
    try
    {
        create_research_content_reward_pools();
        auto research_content_reward_pools = data_service.get_research_content_reward_pools_by_content_id(1);

        BOOST_CHECK(research_content_reward_pools.size() == 2);
        BOOST_CHECK(std::any_of(research_content_reward_pools.begin(), research_content_reward_pools.end(),
                                [](std::reference_wrapper<const research_content_reward_pool_object> wrapper){
            const research_content_reward_pool_object &research_content_reward_pool = wrapper.get();

            return  research_content_reward_pool.id == 1 &&
                    research_content_reward_pool.research_content_id == 1 &&
                    research_content_reward_pool.discipline_id == 1 &&
                    research_content_reward_pool.reward_share == 100 &&
                    research_content_reward_pool.expertise_share == 100;
        }));
         BOOST_CHECK(std::any_of(research_content_reward_pools.begin(), research_content_reward_pools.end(),
                                [](std::reference_wrapper<const research_content_reward_pool_object> wrapper){
            const research_content_reward_pool_object &research_content_reward_pool = wrapper.get();

            return  research_content_reward_pool.id == 2 &&
                    research_content_reward_pool.research_content_id == 1 &&
                    research_content_reward_pool.discipline_id == 2 &&
                    research_content_reward_pool.reward_share == 200 &&
                    research_content_reward_pool.expertise_share == 200;
        }));
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(is_research_reward_pool_exists_by_research_content_id_and_discipline_id)
{
    try
    {
        create_research_content_reward_pools();

        BOOST_CHECK(data_service.is_research_reward_pool_exists_by_research_content_id_and_discipline_id(1, 1) == true);
        BOOST_CHECK(data_service.is_research_reward_pool_exists_by_research_content_id_and_discipline_id(5, 1) == false);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(increase_reward_pool)
{
    try
    {
        create_research_content_reward_pools();
        const auto& research_content_reward_pool = db.get<research_content_reward_pool_object, by_id>(1);

        data_service.increase_reward_pool(research_content_reward_pool, 100);

        BOOST_CHECK(research_content_reward_pool.reward_share == 200);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(increase_expertise_pool)
{
    try
    {
        create_research_content_reward_pools();
        const auto& research_content_reward_pool = db.get<research_content_reward_pool_object, by_id>(1);

        data_service.increase_expertise_pool(research_content_reward_pool, 100);

        BOOST_CHECK(research_content_reward_pool.expertise_share == 200);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
