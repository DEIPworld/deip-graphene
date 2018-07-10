#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/expert_token_object.hpp>
#include <deip/chain/dbs_expert_token.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class expert_token_service_fixture : public clean_database_fixture
{
public:
    expert_token_service_fixture()
            : data_service(db.obtain_service<dbs_expert_token>())
    {
    }

    void create_expert_tokens()
    {
        data_service.create("alice", 1, 100);
        data_service.create("bob", 2, 200);
        data_service.create("alice", 3, 300);
        data_service.create("john", 1, 150);        
    }

    dbs_expert_token& data_service;
};

BOOST_FIXTURE_TEST_SUITE(expert_token_service, expert_token_service_fixture)

BOOST_AUTO_TEST_CASE(create)
{
    ACTORS((alice))

    try
    {
        auto token = data_service.create("alice", 2, 6651);

        BOOST_CHECK(token.account_name == "alice");
        BOOST_CHECK(token.discipline_id == 2);
        BOOST_CHECK(token.amount == 6651);
        BOOST_CHECK(token.voting_power == DEIP_100_PERCENT);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_expert_tokens_vector_by_account_name)
{
    ACTORS((alice)(bob)(john))
    
    try
    {
        create_expert_tokens();
        auto expert_tokens = data_service.get_expert_tokens_by_account_name("alice");

        BOOST_CHECK(expert_tokens.size() == 2);
        BOOST_CHECK(std::any_of(expert_tokens.begin(), expert_tokens.end(), [](std::reference_wrapper<const expert_token_object> wrapper){
            const expert_token_object &token = wrapper.get();
            return token.account_name == "alice" && token.discipline_id == 1 && token.amount == 100;
        }));
        BOOST_CHECK(std::any_of(expert_tokens.begin(), expert_tokens.end(), [](std::reference_wrapper<const expert_token_object> wrapper){
            const expert_token_object &token = wrapper.get();
            return token.account_name == "alice" && token.discipline_id == 3 && token.amount == 300;
        }));
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(get_empty_expert_tokens_vector_by_account_name)
{
    ACTORS((alice)(bob)(john))

    try
    {
        create_expert_tokens();
        auto expert_tokens = data_service.get_expert_tokens_by_account_name("somebody");

        BOOST_CHECK(expert_tokens.size() == 0);
    }
    FC_LOG_AND_RETHROW()
}


BOOST_AUTO_TEST_CASE(get_expert_tokens_vector_by_discipline_id)
{
    ACTORS((alice)(bob)(john))

    try
    {
        create_expert_tokens();
        auto expert_tokens = data_service.get_expert_tokens_by_discipline_id(1);

        BOOST_CHECK(expert_tokens.size() == 23);
        BOOST_CHECK(std::any_of(expert_tokens.begin(), expert_tokens.end(), [](std::reference_wrapper<const expert_token_object> wrapper){
            const expert_token_object &token = wrapper.get();
            return token.account_name == "alice" && token.discipline_id == 1 && token.amount == 100;
        }));
        BOOST_CHECK(std::any_of(expert_tokens.begin(), expert_tokens.end(), [](std::reference_wrapper<const expert_token_object> wrapper){
            const expert_token_object &token = wrapper.get();
            return token.account_name == "john" && token.discipline_id == 1 && token.amount == 150;
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_empty_expert_tokens_vector_by_discipline_id)
{
    ACTORS((alice)(bob)(john))

    try
    {
        create_expert_tokens();
        auto expert_tokens = data_service.get_expert_tokens_by_discipline_id(1212);

        BOOST_CHECK(expert_tokens.size() == 0);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_expert_token_by_account_and_discipline_id)
{
    ACTORS((alice)(bob)(john))

    try
    {
        create_expert_tokens();
        auto expert_token = data_service.get_expert_token_by_account_and_discipline("alice", 1);
        
        BOOST_CHECK(expert_token.amount == 100);
        BOOST_CHECK(expert_token.discipline_id == 1);
        BOOST_CHECK(expert_token.account_name == "alice");

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_expert_token_existence_by_account_and_discipline_id)
{
    BOOST_CHECK_THROW(data_service.check_expert_token_existence_by_account_and_discipline("alice", 1), fc::assert_exception);
}




BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
