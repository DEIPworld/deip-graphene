#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/services/dbs_offer_research_tokens.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class offer_research_tokens_service_fixture : public clean_database_fixture
{
public:
    offer_research_tokens_service_fixture()
            : data_service(db.obtain_service<dbs_offer_research_tokens>())
    {
    }

    void create_offers() {
        db.create<offer_research_tokens_object>([&](offer_research_tokens_object& o) {
            o.id = 1;
            o.sender = "alice";
            o.receiver = "bob";
            o.research_id = 1;
            o.amount = 10 * DEIP_1_PERCENT;
            o.price = asset(100, DEIP_SYMBOL);
        });
        db.create<offer_research_tokens_object>([&](offer_research_tokens_object& o) {
            o.id = 2;
            o.sender = "alice";
            o.receiver = "john";
            o.research_id = 1;
            o.amount = 5 * DEIP_1_PERCENT;
            o.price = asset(50, DEIP_SYMBOL);
        });
        db.create<offer_research_tokens_object>([&](offer_research_tokens_object& o) {
            o.id = 3;
            o.sender = "bob";
            o.receiver = "john";
            o.research_id = 2;
            o.amount = 7 * DEIP_1_PERCENT;
            o.price = asset(100, DEIP_SYMBOL);
        });
        db.create<offer_research_tokens_object>([&](offer_research_tokens_object& o) {
            o.id = 4;
            o.sender = "alice";
            o.receiver = "mike";
            o.research_id = 1;
            o.amount = 11 * DEIP_1_PERCENT;
            o.price = asset(110, DEIP_SYMBOL);
        });
    }

    dbs_offer_research_tokens& data_service;
};

BOOST_FIXTURE_TEST_SUITE(research_service_tests, offer_research_tokens_service_fixture)

BOOST_AUTO_TEST_CASE(create_offer)
{
    try
    {
        auto& offer = data_service.create("alice", "bob", 1, 1000, asset(100, DEIP_SYMBOL));

        BOOST_CHECK(offer.sender == "alice");
        BOOST_CHECK(offer.receiver == "bob");
        BOOST_CHECK(offer.research_id == 1);
        BOOST_CHECK(offer.amount == 1000);
        BOOST_CHECK(offer.price == asset(100, DEIP_SYMBOL));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_offers)
{
    try
    {
        create_offers();

        const auto& offers = data_service.get_offers();

        BOOST_CHECK(offers.size() == 4);

        BOOST_CHECK(std::any_of(offers.begin(), offers.end(), [](std::reference_wrapper<const offer_research_tokens_object> wrapper){
            const offer_research_tokens_object &offer = wrapper.get();
            return  offer.id == 1 &&
                    offer.sender == "alice" &&
                    offer.receiver == "bob" &&
                    offer.research_id == 1 &&
                    offer.amount == 1000 &&
                    offer.price == asset(100, DEIP_SYMBOL);
        }));

        BOOST_CHECK(std::any_of(offers.begin(), offers.end(), [](std::reference_wrapper<const offer_research_tokens_object> wrapper){
            const offer_research_tokens_object &offer = wrapper.get();
            return  offer.id == 2 &&
                    offer.sender == "alice" &&
                    offer.receiver == "john" &&
                    offer.research_id == 1 &&
                    offer.amount == 500 &&
                    offer.price == asset(50, DEIP_SYMBOL);
        }));

        BOOST_CHECK(std::any_of(offers.begin(), offers.end(), [](std::reference_wrapper<const offer_research_tokens_object> wrapper){
            const offer_research_tokens_object &offer = wrapper.get();
            return  offer.id == 3 &&
                    offer.sender == "bob" &&
                    offer.receiver == "john" &&
                    offer.research_id == 2 &&
                    offer.amount == 700 &&
                    offer.price == asset(100, DEIP_SYMBOL);
        }));

        BOOST_CHECK(std::any_of(offers.begin(), offers.end(), [](std::reference_wrapper<const offer_research_tokens_object> wrapper){
            const offer_research_tokens_object &offer = wrapper.get();
            return  offer.id == 4 &&
                    offer.sender == "alice" &&
                    offer.receiver == "mike" &&
                    offer.research_id == 1 &&
                    offer.amount == 1100 &&
                    offer.price == asset(110, DEIP_SYMBOL);
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_offer)
{
    try
    {
        create_offers();

        auto& offer = data_service.get(1);

        BOOST_CHECK(offer.id == 1);
        BOOST_CHECK(offer.sender == "alice");
        BOOST_CHECK(offer.receiver == "bob");
        BOOST_CHECK(offer.research_id == 1);
        BOOST_CHECK(offer.amount == 1000);
        BOOST_CHECK(offer.price == asset(100, DEIP_SYMBOL));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_offers_by_receiver)
{
    try
    {
        create_offers();

        const auto& offers = data_service.get_offers_by_receiver("john");

        BOOST_CHECK(offers.size() == 2);

        BOOST_CHECK(std::any_of(offers.begin(), offers.end(), [](std::reference_wrapper<const offer_research_tokens_object> wrapper){
            const offer_research_tokens_object &offer = wrapper.get();
            return  offer.id == 2 &&
                    offer.sender == "alice" &&
                    offer.receiver == "john" &&
                    offer.research_id == 1 &&
                    offer.amount == 500 &&
                    offer.price == asset(50, DEIP_SYMBOL);
        }));

        BOOST_CHECK(std::any_of(offers.begin(), offers.end(), [](std::reference_wrapper<const offer_research_tokens_object> wrapper){
            const offer_research_tokens_object &offer = wrapper.get();
            return  offer.id == 3 &&
                    offer.sender == "bob" &&
                    offer.receiver == "john" &&
                    offer.research_id == 2 &&
                    offer.amount == 700 &&
                    offer.price == asset(100, DEIP_SYMBOL);
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_offer_by_receiver_and_research_id)
{
    try
    {
        create_offers();

        auto& offer = data_service.get_offer_by_receiver_and_research_id("bob", 1);

        BOOST_CHECK(offer.id == 1);
        BOOST_CHECK(offer.sender == "alice");
        BOOST_CHECK(offer.receiver == "bob");
        BOOST_CHECK(offer.research_id == 1);
        BOOST_CHECK(offer.amount == 1000);
        BOOST_CHECK(offer.price == asset(100, DEIP_SYMBOL));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_offers_by_research_id)
{
    try
    {
        create_offers();

        const auto& offers = data_service.get_offers_by_research_id(1);

        BOOST_CHECK(offers.size() == 3);

        BOOST_CHECK(std::any_of(offers.begin(), offers.end(), [](std::reference_wrapper<const offer_research_tokens_object> wrapper){
            const offer_research_tokens_object &offer = wrapper.get();
            return  offer.id == 1 &&
                    offer.sender == "alice" &&
                    offer.receiver == "bob" &&
                    offer.research_id == 1 &&
                    offer.amount == 1000 &&
                    offer.price == asset(100, DEIP_SYMBOL);
        }));

        BOOST_CHECK(std::any_of(offers.begin(), offers.end(), [](std::reference_wrapper<const offer_research_tokens_object> wrapper){
            const offer_research_tokens_object &offer = wrapper.get();
            return  offer.id == 2 &&
                    offer.sender == "alice" &&
                    offer.receiver == "john" &&
                    offer.research_id == 1 &&
                    offer.amount == 500 &&
                    offer.price == asset(50, DEIP_SYMBOL);
        }));

        BOOST_CHECK(std::any_of(offers.begin(), offers.end(), [](std::reference_wrapper<const offer_research_tokens_object> wrapper){
            const offer_research_tokens_object &offer = wrapper.get();
            return  offer.id == 4 &&
                    offer.sender == "alice" &&
                    offer.receiver == "mike" &&
                    offer.research_id == 1 &&
                    offer.amount == 1100 &&
                    offer.price == asset(110, DEIP_SYMBOL);
        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_offer_existence)
{
    try
    {
        create_offers();

        BOOST_CHECK_NO_THROW(data_service.check_offer_existence(2));
        BOOST_CHECK_THROW(data_service.check_offer_existence(5), fc::assert_exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
