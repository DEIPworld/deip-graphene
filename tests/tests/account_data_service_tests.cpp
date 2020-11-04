#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <deip/chain/schema/account_object.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

private_key_type generate_private_key(const std::string& str)
{
    return private_key_type::regenerate(fc::sha256::hash(std::string(str)));
}

class create_account_with_data_service_fixture : public clean_database_fixture
{
public:
    create_account_with_data_service_fixture()
        : public_key(generate_private_key("user private key").get_public_key())
        , data_service(db.obtain_service<dbs_account>())
    {
    }

    void create_account()
    {
        fc::optional<std::string> json_metadata;
        flat_map<uint16_t, authority> active_overrides;
        flat_set<account_trait> traits;

        data_service.create_account_by_faucets("user", "initdelegate", public_key, json_metadata, authority(),
                                               authority(), active_overrides, asset(0, DEIP_SYMBOL), traits,
                                               true);
    }

    share_type calc_fee()
    {
        return std::max(db.get_witness_schedule_object().median_props.account_creation_fee.amount
                            * DEIP_CREATE_ACCOUNT_WITH_DEIP_MODIFIER,
                        share_type(100));
    }

    const public_key_type public_key;
    dbs_account& data_service;
};

BOOST_FIXTURE_TEST_SUITE(create_account_with_data_service, create_account_with_data_service_fixture)

BOOST_AUTO_TEST_CASE(check_account_name)
{
    try
    {
        create_account();

        const account_object& acount = db.get_account("user");

        BOOST_CHECK(acount.name == "user");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_recovery_account_name)
{
    try
    {
        create_account();

        const account_object& acount = db.get_account("user");

        BOOST_CHECK(acount.recovery_account == "initdelegate");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(fail_on_second_creation)
{
    try
    {
        create_account();
        fc::optional<std::string> json_metadata;
        flat_map<uint16_t, authority> active_overrides;
        flat_set<account_trait> traits;

        BOOST_CHECK_THROW(
            data_service.create_account_by_faucets("user", "initdelegate", public_key, json_metadata, authority(),
                                                   authority(), active_overrides, asset(0, DEIP_SYMBOL),
                                                   traits, true),
            boost::exception_detail::clone_impl<boost::exception_detail::error_info_injector<std::logic_error>>);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(create_without_fee)
{
    try
    {
        dbs_account_balance& balance_service = db.obtain_service<dbs_account_balance>();

        auto balance_before_creation = balance_service.get_account_balance_by_owner_and_asset("initdelegate", DEIP_SYMBOL);

        create_account();

        BOOST_CHECK(balance_service.get_account_balance_by_owner_and_asset("initdelegate", DEIP_SYMBOL).amount == balance_before_creation.amount);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_fee_after_creation)
{
    try
    {
        dbs_account_balance& balance_service = db.obtain_service<dbs_account_balance>();

        auto balance_before_creation = balance_service.get_account_balance_by_owner_and_asset("initdelegate", DEIP_SYMBOL);

        const share_type fee = calc_fee();
        fc::optional<std::string> json_metadata;
        flat_map<uint16_t, authority> active_overrides;
        flat_set<account_trait> traits;

        data_service.create_account_by_faucets("user", "initdelegate", public_key, json_metadata, authority(),
                                               authority(), active_overrides, asset(fee, DEIP_SYMBOL), traits,
                                               true);

        BOOST_CHECK(balance_service.get_account_balance_by_owner_and_asset("initdelegate", DEIP_SYMBOL).amount == balance_before_creation.amount - fee);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace chain
} // namespace deip

#endif
