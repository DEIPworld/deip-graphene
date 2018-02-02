#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <deip/chain/deip_objects.hpp>
#include <deip/chain/history_object.hpp>
#include <deip/account_history/account_history_plugin.hpp>
#include <deip/witness/witness_plugin.hpp>
#include <deip/chain/genesis_state.hpp>


#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

void create_initdelegate_for_genesis_state(genesis_state_type& genesis_state)
{
    private_key_type init_delegate_priv_key = private_key_type::regenerate(fc::sha256::hash(string("init_key")));
    public_key_type init_public_key = init_delegate_priv_key.get_public_key();

    genesis_state.accounts.push_back(
        { "initdelegate", "null", init_public_key, genesis_state.init_supply, uint64_t(0) });

    genesis_state.witness_candidates.push_back({ "initdelegate", init_public_key });
}

database_fixture::database_fixture()
    : app()
    , db(*app.chain_database())
    , init_account_priv_key(private_key_type::regenerate(fc::sha256::hash(string("init_key"))))
    , init_account_pub_key(init_account_priv_key.get_public_key())
    , debug_key(graphene::utilities::key_to_wif(init_account_priv_key))
    , default_skip(0 | database::skip_undo_history_check | database::skip_authority_check)
{
    genesis_state.init_supply = TEST_INITIAL_SUPPLY;
    genesis_state.init_rewards_supply = TEST_REWARD_INITIAL_SUPPLY;
    genesis_state.initial_chain_id = TEST_CHAIN_ID;
    genesis_state.initial_timestamp = fc::time_point_sec(TEST_GENESIS_TIMESTAMP);

    create_initdelegate_for_genesis_state(genesis_state);
}

database_fixture::~database_fixture()
{
}

clean_database_fixture::clean_database_fixture()
{
    try
    {
        int argc = boost::unit_test::framework::master_test_suite().argc;
        char** argv = boost::unit_test::framework::master_test_suite().argv;
        for (int i = 1; i < argc; i++)
        {
            const std::string arg = argv[i];
            if (arg == "--record-assert-trip")
                fc::enable_record_assert_trip = true;
            if (arg == "--show-test-names")
                std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
        }
        auto ahplugin = app.register_plugin<deip::account_history::account_history_plugin>();
        db_plugin = app.register_plugin<deip::plugin::debug_node::debug_node_plugin>();
        auto wit_plugin = app.register_plugin<deip::witness::witness_plugin>();

        boost::program_options::variables_map options;

        db_plugin->logging = false;
        ahplugin->plugin_initialize(options);
        db_plugin->plugin_initialize(options);
        wit_plugin->plugin_initialize(options);

        open_database();

        generate_block();
        db.set_hardfork(DEIP_NUM_HARDFORKS);
        generate_block();

        // ahplugin->plugin_startup();
        db_plugin->plugin_startup();
        vest(TEST_INIT_DELEGATE_NAME, 10000);

        // Fill up the rest of the required miners
        for (int i = DEIP_NUM_INIT_DELEGATES; i < DEIP_MAX_WITNESSES; i++)
        {
            account_create(TEST_INIT_DELEGATE_NAME + fc::to_string(i), init_account_pub_key);
            fund(TEST_INIT_DELEGATE_NAME + fc::to_string(i), DEIP_MIN_PRODUCER_REWARD.amount.value);
            witness_create(TEST_INIT_DELEGATE_NAME + fc::to_string(i), init_account_priv_key, "foo.bar",
                           init_account_pub_key, DEIP_MIN_PRODUCER_REWARD.amount);
        }

        validate_database();
    }
    catch (const fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }

    return;
}

clean_database_fixture::~clean_database_fixture()
{
    try
    {
        // If we're unwinding due to an exception, don't do any more checks.
        // This way, boost test's last checkpoint tells us approximately where the error was.
        if (!std::uncaught_exception())
        {
            BOOST_CHECK(db.get_node_properties().skip_flags == database::skip_nothing);
        }

        if (data_dir)
            db.close();

        return;
    }
    FC_CAPTURE_AND_RETHROW()
}

void clean_database_fixture::resize_shared_mem(uint64_t size)
{
    db.wipe(data_dir->path(), data_dir->path(), true);
    int argc = boost::unit_test::framework::master_test_suite().argc;
    char** argv = boost::unit_test::framework::master_test_suite().argv;
    for (int i = 1; i < argc; i++)
    {
        const std::string arg = argv[i];
        if (arg == "--record-assert-trip")
            fc::enable_record_assert_trip = true;
        if (arg == "--show-test-names")
            std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
    }

    db.open(data_dir->path(), data_dir->path(), size, chainbase::database::read_write, genesis_state);

    boost::program_options::variables_map options;

    generate_block();
    db.set_hardfork(DEIP_NUM_HARDFORKS);
    generate_block();

    vest(TEST_INIT_DELEGATE_NAME, 10000);

    // Fill up the rest of the required miners
    for (int i = DEIP_NUM_INIT_DELEGATES; i < DEIP_MAX_WITNESSES; i++)
    {
        account_create(TEST_INIT_DELEGATE_NAME + fc::to_string(i), init_account_pub_key);
        fund(TEST_INIT_DELEGATE_NAME + fc::to_string(i), DEIP_MIN_PRODUCER_REWARD.amount.value);
        witness_create(TEST_INIT_DELEGATE_NAME + fc::to_string(i), init_account_priv_key, "foo.bar",
                       init_account_pub_key, DEIP_MIN_PRODUCER_REWARD.amount);
    }

    validate_database();
}

live_database_fixture::live_database_fixture()
{
    try
    {
        ilog("Loading saved chain");
        _chain_dir = fc::current_path() / "test_blockchain";
        FC_ASSERT(fc::exists(_chain_dir), "Requires blockchain to test on in ./test_blockchain");

        auto ahplugin = app.register_plugin<deip::account_history::account_history_plugin>();
        ahplugin->plugin_initialize(boost::program_options::variables_map());

        db.open(_chain_dir, _chain_dir, 0, 0, genesis_state);

        validate_database();
        generate_block();

        ilog("Done loading saved chain");
    }
    FC_LOG_AND_RETHROW()
}

live_database_fixture::~live_database_fixture()
{
    try
    {
        // If we're unwinding due to an exception, don't do any more checks.
        // This way, boost test's last checkpoint tells us approximately where the error was.
        if (!std::uncaught_exception())
        {
            BOOST_CHECK(db.get_node_properties().skip_flags == database::skip_nothing);
        }

        db.pop_block();
        db.close();
        return;
    }
    FC_LOG_AND_RETHROW()
}

timed_blocks_database_fixture::timed_blocks_database_fixture()
{
    default_deadline = db.get_slot_time(BLOCK_LIMIT_DEFAULT);
    if (!_time_printed)
    {
        const size_t w = 20;
        std::cout << std::setw(w) << "head_block_time = " << db.head_block_time().to_iso_string() << std::endl;
        for (int slot = 1; slot <= BLOCK_LIMIT_DEFAULT; ++slot)
        {
            std::stringstream title;
            title << "slot_time(" << slot << ") = ";
            std::cout << std::setw(w) << title.str() << db.get_slot_time(slot).to_iso_string() << std::endl;
        }
        std::cout << std::setw(w) << "default_deadline = " << default_deadline.to_iso_string() << std::endl;
        _time_printed = true;
    }
}

bool timed_blocks_database_fixture::_time_printed = false;

private_key_type database_fixture::generate_private_key(const std::string& seed)
{
    static const private_key_type committee = private_key_type::regenerate(fc::sha256::hash(std::string("init_key")));
    if (seed == "init_key")
        return committee;
    return fc::ecc::private_key::regenerate(fc::sha256::hash(seed));
}

void database_fixture::open_database()
{
    if (!data_dir)
    {
        data_dir = fc::temp_directory(graphene::utilities::temp_directory_path());
        db._log_hardforks = false;
        db.open(data_dir->path(), data_dir->path(), TEST_SHARED_MEM_SIZE_8MB, chainbase::database::read_write,
                genesis_state);
    }
}

void database_fixture::generate_block(uint32_t skip, const fc::ecc::private_key& key, int miss_blocks)
{
    skip |= default_skip;
    db_plugin->debug_generate_blocks(graphene::utilities::key_to_wif(key), 1, skip, miss_blocks);
}

void database_fixture::generate_blocks(uint32_t block_count)
{
    auto produced = db_plugin->debug_generate_blocks(debug_key, block_count, default_skip, 0);
    BOOST_REQUIRE(produced == block_count);
}

void database_fixture::generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks)
{
    db_plugin->debug_generate_blocks_until(debug_key, timestamp, miss_intermediate_blocks, default_skip);
    BOOST_REQUIRE((db.head_block_time() - timestamp).to_seconds() < DEIP_BLOCK_INTERVAL);
}

const account_object& database_fixture::account_create(const string& name,
                                                       const string& creator,
                                                       const private_key_type& creator_key,
                                                       const share_type& fee,
                                                       const public_key_type& key,
                                                       const public_key_type& post_key,
                                                       const string& json_metadata)
{
    try
    {

        account_create_with_delegation_operation op;
        op.new_account_name = name;
        op.creator = creator;
        op.fee = asset(fee, DEIP_SYMBOL);
        op.delegation = asset(0, VESTS_SYMBOL);
        op.owner = authority(1, key, 1);
        op.active = authority(1, key, 1);
        op.posting = authority(1, post_key, 1);
        op.memo_key = key;
        op.json_metadata = json_metadata;

        trx.operations.push_back(op);

        trx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        trx.sign(creator_key, db.get_chain_id());
        trx.validate();
        db.push_transaction(trx, 0);
        trx.operations.clear();
        trx.signatures.clear();

        const account_object& acct = db.get_account(name);

        return acct;
    }
    FC_CAPTURE_AND_RETHROW((name)(creator))
}

const account_object&
database_fixture::account_create(const string& name, const public_key_type& key, const public_key_type& post_key)
{
    try
    {
        return account_create(name, TEST_INIT_DELEGATE_NAME, init_account_priv_key,
                              std::max(db.get_witness_schedule_object().median_props.account_creation_fee.amount
                                           * DEIP_CREATE_ACCOUNT_WITH_DEIP_MODIFIER,
                                       share_type(100)),
                              key, post_key, "");
    }
    FC_CAPTURE_AND_RETHROW((name));
}

const account_object& database_fixture::account_create(const string& name, const public_key_type& key)
{
    return account_create(name, key, key);
}


const discipline_object& database_fixture::discipline_create(const discipline_id_type& id, 
                                                            const discipline_name_type& name, 
                                                            const discipline_id_type& parent_id,
                                                            const share_type& votes_in_last_ten_weeks)
{
   const discipline_object& discipline = db.create<discipline_object>([&](discipline_object& d) {
            d.id = id;
            d.name = name;
            d.parent_id = parent_id;
            d.votes_in_last_ten_weeks = votes_in_last_ten_weeks;
    });

    return discipline;
}

const witness_object& database_fixture::witness_create(const string& owner,
                                                       const private_key_type& owner_key,
                                                       const string& url,
                                                       const public_key_type& signing_key,
                                                       const share_type& fee)
{
    try
    {
        witness_update_operation op;
        op.owner = owner;
        op.url = url;
        op.block_signing_key = signing_key;
        op.fee = asset(fee, DEIP_SYMBOL);

        trx.operations.push_back(op);
        trx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        trx.sign(owner_key, db.get_chain_id());
        trx.validate();
        db.push_transaction(trx, 0);
        trx.operations.clear();
        trx.signatures.clear();

        return db.get_witness(owner);
    }
    FC_CAPTURE_AND_RETHROW((owner)(url))
}

const research_group_object&
database_fixture::research_group_create(const uint32_t& id, const string& permlink,
                                        const string& description, const share_type funds,
                                        const uint32_t& quorum_percent, const int32_t& tokens_amount)
{
    const research_group_object& new_research_group
        = db.create<research_group_object>([&](research_group_object& rg) {
              rg.id = id;
              fc::from_string(rg.permlink, permlink);
              fc::from_string(rg.description, description);
              rg.funds = funds;
              rg.quorum_percent = quorum_percent;
              rg.total_tokens_amount = tokens_amount;
          });

    return new_research_group;
}

const research_group_token_object& database_fixture::research_group_token_create(
    const research_group_id_type& research_group_id, const account_name_type& account, const int32_t& amount = 10)
{
    auto& research_group_service = db.obtain_service<dbs_research_group>();
    const research_group_token_object& new_research_group_token = research_group_service.create_research_group_token(research_group_id, amount, account);
    research_group_service.adjust_research_group_token_amount(research_group_id, amount);
    return new_research_group_token;
}

const research_group_object& database_fixture::setup_research_group(const uint32_t &id,
                                                                    const string &permlink,
                                                                    const string &description,
                                                                    const share_type funds,
                                                                    const uint32_t &quorum_percent,
                                                                    const int32_t &tokens_amount,
                                                                    const vector<account_name_type> &accounts)
{
    const auto& research_group = research_group_create(id, permlink, description, funds, quorum_percent, tokens_amount);

    for (const auto& account : accounts)
    {
        research_group_token_create(research_group.id, account, tokens_amount);
    }

    return research_group;
}

const proposal_object& database_fixture::proposal_create(const uint32_t id, const dbs_proposal::action_t action,
                                       const std::string json_data,
                                       const account_name_type& creator,
                                       const research_group_id_type& research_group_id,
                                       const fc::time_point_sec expiration_time,
                                       const uint32_t quorum_percent)
{
    const proposal_object& new_proposal = db.create<proposal_object>([&](proposal_object& proposal) {
        proposal.action = action;
        proposal.id = id;
        proposal.data = json_data;
        proposal.creator = creator;
        proposal.research_group_id = research_group_id;
        proposal.creation_time = fc::time_point_sec();
        proposal.expiration_time = expiration_time;
        proposal.quorum_percent = quorum_percent;
    });

    return new_proposal;
}

const research_object& database_fixture::research_create(const string &name, const string &abstract, const string &permlink,
                                            const research_group_id_type &research_group_id, const uint32_t &percent_for_review)
{
    const auto& new_research = db.create<research_object>([&](research_object& research) {
        research.name = name;
        research.abstract = abstract;
        research.permlink = permlink;
        research.research_group_id = research_group_id;
        research.review_share_in_percent = percent_for_review;
        research.is_finished = false;
        research.owned_tokens = DEIP_100_PERCENT;
        research.created_at = db.head_block_time();
    });

    return new_research;
}
    
void database_fixture::fund(const string& account_name, const share_type& amount)
{
    try
    {
        transfer(TEST_INIT_DELEGATE_NAME, account_name, amount);
    }
    FC_CAPTURE_AND_RETHROW((account_name)(amount))
}

void database_fixture::fund(const string& account_name, const asset& amount)
{
    try
    {
        db_plugin->debug_update(
            [=](database& db) {
                db.modify(db.get_account(account_name), [&](account_object& a) {
                    if (amount.symbol == DEIP_SYMBOL)
                        a.balance += amount;
                });

                db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& gpo) {
                    if (amount.symbol == DEIP_SYMBOL)
                        gpo.current_supply += amount;
                });
            },
            default_skip);
    }
    FC_CAPTURE_AND_RETHROW((account_name)(amount))
}

void database_fixture::transfer(const string& from, const string& to, const share_type& amount)
{
    try
    {
        transfer_operation op;
        op.from = from;
        op.to = to;
        op.amount = amount;

        trx.operations.push_back(op);
        trx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        trx.validate();
        db.push_transaction(trx, ~0);
        trx.operations.clear();
    }
    FC_CAPTURE_AND_RETHROW((from)(to)(amount))
}

void database_fixture::vest(const string& from, const share_type& amount)
{
    try
    {
        transfer_to_vesting_operation op;
        op.from = from;
        op.to = "";
        op.amount = asset(amount, DEIP_SYMBOL);

        trx.operations.push_back(op);
        trx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        trx.validate();
        db.push_transaction(trx, ~0);
        trx.operations.clear();
    }
    FC_CAPTURE_AND_RETHROW((from)(amount))
}

void database_fixture::vest(const string& account, const asset& amount)
{
    if (amount.symbol != DEIP_SYMBOL)
        return;

    db_plugin->debug_update(
        [=](database& db) {
            db.modify(db.get_dynamic_global_properties(),
                      [&](dynamic_global_property_object& gpo) { gpo.current_supply += amount; });

            dbs_account& account_service = db.obtain_service<dbs_account>();
            account_service.create_vesting(db.get_account(account), amount);
        },
        default_skip);
}

void database_fixture::proxy(const string& account, const string& proxy)
{
    try
    {
        account_witness_proxy_operation op;
        op.account = account;
        op.proxy = proxy;
        trx.operations.push_back(op);
        db.push_transaction(trx, ~0);
        trx.operations.clear();
    }
    FC_CAPTURE_AND_RETHROW((account)(proxy))
}

const asset& database_fixture::get_balance(const string& account_name) const
{
    return db.get_account(account_name).balance;
}

void database_fixture::sign(signed_transaction& trx, const fc::ecc::private_key& key)
{
    trx.sign(key, db.get_chain_id());
}

vector<operation> database_fixture::get_last_operations(uint32_t num_ops)
{
    vector<operation> ops;
    const auto& acc_hist_idx = db.get_index<account_history_index>().indices().get<by_id>();
    auto itr = acc_hist_idx.end();

    while (itr != acc_hist_idx.begin() && ops.size() < num_ops)
    {
        itr--;
        ops.push_back(fc::raw::unpack<deip::chain::operation>(db.get(itr->op).serialized_op));
    }

    return ops;
}

void database_fixture::validate_database(void)
{
    try
    {
        db.validate_invariants();
    }
    FC_LOG_AND_RETHROW();
}

namespace test {

bool _push_block(database& db, const signed_block& b, uint32_t skip_flags /* = 0 */)
{
    return db.push_block(b, skip_flags);
}

void _push_transaction(database& db, const signed_transaction& tx, uint32_t skip_flags /* = 0 */)
{
    try
    {
        db.push_transaction(tx, skip_flags);
    }
    FC_CAPTURE_AND_RETHROW((tx))
}

} // namespace test
} // namespace chain
} // namespace deip
