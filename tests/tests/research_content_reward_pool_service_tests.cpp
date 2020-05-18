#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/reward_pool_object.hpp>
#include <deip/chain/services/dbs_reward_pool.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {


class reward_pool_service_fixture : public clean_database_fixture
{
public:
    reward_pool_service_fixture()
            : data_service(db.obtain_service<dbs_reward_pool>())
    {
    }

    void create_reward_pools()
    {
        db.create<reward_pool_object>([&](reward_pool_object& r) {
            r.id = 1;
            r.discipline_id = 1;
            r.research_content_id = 1;
            r.balance = asset(100, DEIP_SYMBOL);
            r.expertise = 100;
        });

        db.create<reward_pool_object>([&](reward_pool_object& r) {
            r.id = 2;
            r.discipline_id = 2;
            r.research_content_id = 1;
            r.balance = asset(200, DEIP_SYMBOL);
            r.expertise = 200;
        });

        db.create<reward_pool_object>([&](reward_pool_object& r) {
            r.id = 3;
            r.discipline_id = 1;
            r.research_content_id = 2;
            r.balance = asset(1500, DEIP_SYMBOL);
            r.expertise = 1500;
        });
    }

    dbs_reward_pool& data_service;
};

BOOST_FIXTURE_TEST_SUITE(reward_pool_service_tests, reward_pool_service_fixture)

BOOST_AUTO_TEST_CASE(create)
{
    try
    {
        const reward_pool_object& reward_pool =
                data_service.create(1, 1, asset(100, DEIP_SYMBOL), 200);

        BOOST_CHECK(reward_pool.id == 0);
        BOOST_CHECK(reward_pool.research_content_id == 1);
        BOOST_CHECK(reward_pool.discipline_id == 1);
        BOOST_CHECK(reward_pool.balance == asset(100, DEIP_SYMBOL));
        BOOST_CHECK(reward_pool.expertise == 200);

    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(get_by_research_content_id_and_discipline_id)
{
    try
    {
        create_reward_pools();
        BOOST_CHECK_THROW(data_service.get_by_research_content_id_and_discipline_id(1, 100), fc::exception);

        auto& reward_pool = data_service.get_by_research_content_id_and_discipline_id(1, 2);

        BOOST_CHECK(reward_pool.id == 2);
        BOOST_CHECK(reward_pool.research_content_id == 1);
        BOOST_CHECK(reward_pool.discipline_id == 2);
        BOOST_CHECK(reward_pool.balance == asset(200, DEIP_SYMBOL));
        BOOST_CHECK(reward_pool.expertise == 200);
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(get_reward_pools_by_content_id)
{
    try
    {
        create_reward_pools();
        auto reward_pools = data_service.get_reward_pools_by_content_id(1);

        BOOST_CHECK(reward_pools.size() == 2);
        BOOST_CHECK(std::any_of(reward_pools.begin(), reward_pools.end(),
                                [](std::reference_wrapper<const reward_pool_object> wrapper){
            const reward_pool_object &reward_pool = wrapper.get();

            return  reward_pool.id == 1 &&
                    reward_pool.research_content_id == 1 &&
                    reward_pool.discipline_id == 1 &&
                    reward_pool.balance == asset(100, DEIP_SYMBOL) &&
                    reward_pool.expertise == 100;
        }));
         BOOST_CHECK(std::any_of(reward_pools.begin(), reward_pools.end(),
                                [](std::reference_wrapper<const reward_pool_object> wrapper){
            const reward_pool_object &reward_pool = wrapper.get();

            return  reward_pool.id == 2 &&
                    reward_pool.research_content_id == 1 &&
                    reward_pool.discipline_id == 2 &&
                    reward_pool.balance == asset(200, DEIP_SYMBOL) &&
                    reward_pool.expertise == 200;
        }));
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(is_research_reward_pool_exists_by_research_content_id_and_discipline_id)
{
    try
    {
        create_reward_pools();

        BOOST_CHECK(data_service.is_research_reward_pool_exists_by_research_content_id_and_discipline_id(1, 1) == true);
        BOOST_CHECK(data_service.is_research_reward_pool_exists_by_research_content_id_and_discipline_id(5, 1) == false);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(increase_reward_pool)
{
    try
    {
        create_reward_pools();
        const auto& reward_pool = db.get<reward_pool_object, by_id>(1);

        data_service.increase_reward_pool(reward_pool, asset(100, DEIP_SYMBOL));

        BOOST_CHECK(reward_pool.balance == asset(200, DEIP_SYMBOL));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(increase_expertise_pool)
{
    try
    {
        create_reward_pools();
        const auto& reward_pool = db.get<reward_pool_object, by_id>(1);

        data_service.increase_expertise_pool(reward_pool, 100);

        BOOST_CHECK(reward_pool.expertise == 200);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
