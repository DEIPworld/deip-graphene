#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/services/dbs_contract.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

class contract_service_fixture : public clean_database_fixture
{
 public:
    contract_service_fixture()
           : data_service(db.obtain_service<dbs_contract>())
   {

   }

    void create_contracts()
    {
        db.create<contract_object>([&](contract_object& c_o) {
            c_o.id = 0;
            c_o.creator = "alice";
            c_o.signee = "bob";
            c_o.creator_key = public_key_type("DEIP52Xh45F7F7xZqC5ZQAteFCNrDtys12exDo4TMqvQrJW2GLNT83");
            c_o.signee_key = protocol::public_key_type();
            c_o.contract_hash = "contract 1";
            c_o.status = contract_status::contract_sent;
            c_o.created_at = fc::time_point_sec(123123);
            c_o.start_date = fc::time_point_sec(123124);
            c_o.end_date = fc::time_point_sec(123125);
        });

        db.create<contract_object>([&](contract_object& c_o) {
            c_o.id = 1;
            c_o.creator = "alice";
            c_o.signee = "mike";
            c_o.creator_key = public_key_type("DEIP52Xh45F7F7xZqC5ZQAteFCNrDtys12exDo4TMqvQrJW2GLNT83");
            c_o.signee_key = public_key_type("DEIP8bwPxtWUEffTs7C8vRNT98Sv7XGCVDWCXVUeHvtNH5TxvrwVLj");
            c_o.contract_hash = "contract 2";
            c_o.status = contract_status::contract_signed;
            c_o.created_at = fc::time_point_sec(1231);
            c_o.start_date = fc::time_point_sec(1232);
            c_o.end_date = fc::time_point_sec(1233);
        });

        db.create<contract_object>([&](contract_object& c_o) {
            c_o.id = 2;
            c_o.creator = "john";
            c_o.signee = "bob";
            c_o.creator_key = public_key_type("DEIP5mFLmqoifrz3c9iTmFVpxjU29QjyKSGiKZf7GUM2T6cJpm6Gb7");
            c_o.signee_key = protocol::public_key_type();
            c_o.contract_hash = "contract 3";
            c_o.status = contract_status::contract_declined;
            c_o.created_at = fc::time_point_sec(1231231);
            c_o.start_date = fc::time_point_sec(1231232);
            c_o.end_date = fc::time_point_sec(1231233);
        });
    }

    dbs_contract& data_service;
};

BOOST_FIXTURE_TEST_SUITE(contract_service_tests, contract_service_fixture)

BOOST_AUTO_TEST_CASE(create_contract_test)
{
    try
    {
        auto& contract = data_service.create("alice", "bob",
                                             public_key_type("DEIP52Xh45F7F7xZqC5ZQAteFCNrDtys12exDo4TMqvQrJW2GLNT83"),
                                             "test", fc::time_point_sec(123), fc::time_point_sec(1234), fc::time_point_sec(1235));

        BOOST_CHECK(contract.creator == "alice");
        BOOST_CHECK(contract.signee == "bob");
        BOOST_CHECK(contract.creator_key == public_key_type("DEIP52Xh45F7F7xZqC5ZQAteFCNrDtys12exDo4TMqvQrJW2GLNT83"));
        BOOST_CHECK(contract.signee_key == protocol::public_key_type());
        BOOST_CHECK(contract.contract_hash == "test");
        BOOST_CHECK(contract.status == contract_status::contract_sent);
        BOOST_CHECK(contract.created_at == fc::time_point_sec(123));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_contract_test)
{
    try
    {
        create_contracts();
        auto& contract = data_service.get(1);

        BOOST_CHECK(contract.creator == "alice");
        BOOST_CHECK(contract.signee == "mike");
        BOOST_CHECK(contract.creator_key == public_key_type("DEIP52Xh45F7F7xZqC5ZQAteFCNrDtys12exDo4TMqvQrJW2GLNT83"));
        BOOST_CHECK(contract.signee_key == public_key_type("DEIP8bwPxtWUEffTs7C8vRNT98Sv7XGCVDWCXVUeHvtNH5TxvrwVLj"));
        BOOST_CHECK(contract.contract_hash == "contract 2");
        BOOST_CHECK(contract.status == contract_status::contract_signed);
        BOOST_CHECK(contract.created_at == fc::time_point_sec(1231));;

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_research_group_existence_test)
{
    try
    {
        create_contracts();

        BOOST_CHECK_NO_THROW(data_service.check_contract_existence(1));
        BOOST_CHECK_THROW(data_service.check_contract_existence(432), fc::assert_exception);
        BOOST_CHECK_THROW(data_service.check_contract_existence(251), fc::assert_exception);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_contracts_by_creator)
{
    try
    {
        create_contracts();
        auto contracts = data_service.get_by_creator("alice");

        BOOST_CHECK(contracts.size() == 2);
        BOOST_CHECK(std::any_of(contracts.begin(), contracts.end(), [](std::reference_wrapper<const contract_object> wrapper){
            const contract_object &contract = wrapper.get();
            return contract.id == 0 &&
                    contract.creator == "alice" &&
                    contract.signee == "bob" &&
                    contract.creator_key == public_key_type("DEIP52Xh45F7F7xZqC5ZQAteFCNrDtys12exDo4TMqvQrJW2GLNT83") &&
                    contract.signee_key == protocol::public_key_type() &&
                    contract.contract_hash == "contract 1" &&
                    contract.status == contract_status::contract_sent &&
                    contract.created_at == fc::time_point_sec(123123);

        }));
        BOOST_CHECK(std::any_of(contracts.begin(), contracts.end(), [](std::reference_wrapper<const contract_object> wrapper){
            const contract_object &contract = wrapper.get();
            return contract.id == 1 &&
                   contract.creator == "alice" &&
                   contract.signee == "mike" &&
                   contract.creator_key == public_key_type("DEIP52Xh45F7F7xZqC5ZQAteFCNrDtys12exDo4TMqvQrJW2GLNT83") &&
                   contract.signee_key == public_key_type("DEIP8bwPxtWUEffTs7C8vRNT98Sv7XGCVDWCXVUeHvtNH5TxvrwVLj") &&
                   contract.contract_hash == "contract 2" &&
                   contract.status == contract_status::contract_signed &&
                   contract.created_at == fc::time_point_sec(1231);

        }));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(sign_by_receiver_test)
{
    try
    {
        create_contracts();

        auto& contract = data_service.get(0);
        data_service.sign(contract, public_key_type("DEIP5SKiHdSzMGPpPfTGKMWg4YFGKbevp6AeGScP9VvPmn27bxcUdi"));

        BOOST_CHECK(contract.creator == "alice");
        BOOST_CHECK(contract.signee == "bob");
        BOOST_CHECK(contract.creator_key == public_key_type("DEIP52Xh45F7F7xZqC5ZQAteFCNrDtys12exDo4TMqvQrJW2GLNT83"));
        BOOST_CHECK(contract.signee_key == public_key_type("DEIP5SKiHdSzMGPpPfTGKMWg4YFGKbevp6AeGScP9VvPmn27bxcUdi"));
        BOOST_CHECK(contract.contract_hash == "contract 1");
        BOOST_CHECK(contract.status == contract_status::contract_signed);
        BOOST_CHECK(contract.created_at == fc::time_point_sec(123123));

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(set_new_contract_status_test)
{
    try
    {
        create_contracts();
        auto& contract = data_service.get(0);
        data_service.set_new_contract_status(contract, contract_status::contract_signed);

        BOOST_CHECK(contract.creator == "alice");
        BOOST_CHECK(contract.signee == "bob");
        BOOST_CHECK(contract.status == contract_status::contract_signed);

    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
