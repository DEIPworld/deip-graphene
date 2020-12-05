#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <deip/chain/schema/deip_objects.hpp>
#include <deip/blockchain_history/account_history_object.hpp>
#include <deip/blockchain_history/blockchain_history_plugin.hpp>
#include <deip/witness/witness_plugin.hpp>
#include <deip/chain/genesis_state.hpp>


#include <fc/crypto/digest.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>
#include  <deip/chain/schema/account_balance_object.hpp>
#include <deip/chain/schema/research_token_object.hpp>

#include "database_fixture.hpp"

namespace deip {
namespace chain {

void create_initdelegate_for_genesis_state(genesis_state_type& genesis_state)
{
    private_key_type init_delegate_priv_key = private_key_type::regenerate(fc::sha256::hash(string("init_key")));
    public_key_type init_public_key = init_delegate_priv_key.get_public_key();

    genesis_state.accounts.push_back({ "initdelegate", "null", init_public_key });
    
    genesis_state_type::account_balance_type ballance;
    ballance.owner = "initdelegate";
    ballance.amount = genesis_state.init_supply;
    genesis_state.account_balances.push_back(ballance);

    genesis_state.witness_candidates.push_back({ "initdelegate", init_public_key });
}

void create_initdelegate_expert_tokens_for_genesis_state(genesis_state_type& genesis_state)
{
  genesis_state.expert_tokens.push_back({ "initdelegate", 10000, "12345" });
}

void create_disciplines_for_genesis_state(genesis_state_type& genesis_state)
{
    genesis_state.disciplines.push_back({ "Common", "6c4bb3bcf1a88e3b51de88576d592f1f980c5bbb", "" });
    genesis_state.disciplines.push_back({ "Mathematics", "d0710fa4fe2665e09cb0ee8caab54ef1223b126e", ""});
    genesis_state.disciplines.push_back({ "Physics", "9f0224709d86e02b9625b5ebf2786b80ba6bed17", "" });
    genesis_state.disciplines.push_back({ "Chemistry", "6a8b20f002a7dedf7b873dbc86e0b0051d4fa898", "" });
    genesis_state.disciplines.push_back({ "Biology", "7c3d37cbfea2513a7e03e674448bbeee8ae3d862", "" });
    genesis_state.disciplines.push_back({ "Computer Science", "e606abac5cff95d3ec1bc151bf24d3f0796d1dc7", "" });
    genesis_state.disciplines.push_back({ "Medicine", "721e75eb0535e152669b0c3fbbb9e21675483553", "" });
    genesis_state.disciplines.push_back({ "Pure Mathematics", "d0710fa4fe2665e09cb0ee8caab54ef1223b126e", "" });
    genesis_state.disciplines.push_back({ "Mathematical logic", "c0eef9067616b3b53c266b392f2420cde03efb0b", "d0710fa4fe2665e09cb0ee8caab54ef1223b126e" });
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
    auto registrar = genesis_state_type::registrar_account_type();
    registrar.name = DEIP_REGISTRAR_ACCOUNT_NAME;
    registrar.public_key = public_key_type();
    registrar.common_tokens_amount = 0;
    genesis_state.registrar_account = registrar;

    genesis_state.assets.push_back({"TESTS", 3, 0});

    create_disciplines_for_genesis_state(genesis_state);
    create_initdelegate_for_genesis_state(genesis_state);
    create_initdelegate_expert_tokens_for_genesis_state(genesis_state);
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
        auto bhplugin = app.register_plugin<deip::blockchain_history::blockchain_history_plugin>();
        db_plugin = app.register_plugin<deip::plugin::debug_node::debug_node_plugin>();
        auto wit_plugin = app.register_plugin<deip::witness::witness_plugin>();

        boost::program_options::variables_map options;

        db_plugin->logging = false;
        bhplugin->plugin_initialize(options);
        db_plugin->plugin_initialize(options);
        wit_plugin->plugin_initialize(options);

        open_database();

        generate_block();
        db.set_hardfork(DEIP_NUM_HARDFORKS);
        generate_block();

        // bhplugin->plugin_startup();
        db_plugin->plugin_startup();

        // Fill up the rest of the required miners
        for (int i = DEIP_NUM_INIT_DELEGATES; i < DEIP_MAX_WITNESSES; i++)
        {
            create_account(TEST_INIT_DELEGATE_NAME + fc::to_string(i), init_account_pub_key);
            fund(TEST_INIT_DELEGATE_NAME + fc::to_string(i), 1000);
        }

        generate_block();

        for (int i = DEIP_NUM_INIT_DELEGATES; i < DEIP_MAX_WITNESSES; i++)
        {
            expert_token(TEST_INIT_DELEGATE_NAME + fc::to_string(i), 1, 10000);

            witness_create(TEST_INIT_DELEGATE_NAME + fc::to_string(i), init_account_priv_key, "foo.bar",
                           init_account_pub_key, DEIP_MIN_PRODUCER_REWARD.amount.value);
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

    // Fill up the rest of the required miners
    for (int i = DEIP_NUM_INIT_DELEGATES; i < DEIP_MAX_WITNESSES; i++)
    {
        create_account(TEST_INIT_DELEGATE_NAME + fc::to_string(i), init_account_pub_key);
        fund(TEST_INIT_DELEGATE_NAME + fc::to_string(i), DEIP_MIN_PRODUCER_REWARD.amount.value);
    }

    generate_block();

    for (int i = DEIP_NUM_INIT_DELEGATES; i < DEIP_MAX_WITNESSES; i++)
    {
        expert_token(TEST_INIT_DELEGATE_NAME + fc::to_string(i), 1, 10000);
    }

    for (int i = DEIP_NUM_INIT_DELEGATES; i < DEIP_MAX_WITNESSES; i++)
    {
        witness_create(TEST_INIT_DELEGATE_NAME + fc::to_string(i), init_account_priv_key, "foo.bar",
                       init_account_pub_key, DEIP_MIN_PRODUCER_REWARD.amount.value);
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

        auto ahplugin = app.register_plugin<deip::blockchain_history::blockchain_history_plugin>();
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
        db.open(data_dir->path(), data_dir->path(), TEST_SHARED_MEM_SIZE_128MB, chainbase::database::read_write,
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

const account_object& database_fixture::create_account(const string& name,
                                                       const string& creator,
                                                       const private_key_type& creator_key,
                                                       const share_type& fee,
                                                       const public_key_type& key,
                                                       const public_key_type& post_key,
                                                       const string& json_metadata)
{
    try
    {

        create_account_operation op;
        op.new_account_name = name;
        op.creator = creator;
        op.fee = asset(fee, DEIP_SYMBOL);
        op.owner = authority(1, key, 1);
        op.active = authority(1, key, 1);
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
database_fixture::create_account(const string& name, const public_key_type& key, const public_key_type& post_key)
{
    try
    {
        return create_account(name, TEST_INIT_DELEGATE_NAME, init_account_priv_key,
                              std::max(db.get_witness_schedule_object().median_props.account_creation_fee.amount
                                           * DEIP_CREATE_ACCOUNT_WITH_DEIP_MODIFIER,
                                       share_type(10000)),
                              key, post_key, "");
    }
    FC_CAPTURE_AND_RETHROW((name));
}

const account_object& database_fixture::create_account(const string& name, const public_key_type& key)
{
    return create_account(name, key, key);
}


const discipline_object& database_fixture::discipline_create(const discipline_id_type& id, 
                                                            const std::string& name,
                                                            const discipline_id_type& parent_id)
{
   const discipline_object& discipline = db.create<discipline_object>([&](discipline_object& d) {
            d.id = id;
            fc::from_string(d.name, name);
            d.parent_id = parent_id;
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
database_fixture::research_group_create(const int64_t& id,
                                        const string& name,
                                        const string& permlink,
                                        const string& description,
                                        const share_type funds,
                                        const bool is_dao,
                                        const bool is_personal)
{
    const research_group_object& new_research_group
        = db.create<research_group_object>([&](research_group_object& rg) {
              rg.id = id;
              fc::from_string(rg.name, name);
              fc::from_string(rg.permlink, permlink);
              fc::from_string(rg.description, description);
              rg.balance = funds;
              rg.is_dao = is_dao;
              rg.is_personal = is_personal;
          });

    return new_research_group;
}

const research_group_token_object& database_fixture::research_group_token_create(
    const research_group_id_type& research_group_id, const account_name_type& account, const share_type tokens_amount)
{
    auto& research_group_service = db.obtain_service<dbs_research_group>();
    const research_group_token_object& new_research_group_token = research_group_service.add_member_to_research_group(account, research_group_id, tokens_amount, account_name_type());
    return new_research_group_token;
}

const research_group_object&
database_fixture::setup_research_group(const int64_t& id,
                                       const string& name,
                                       const string& permlink,
                                       const string& description,
                                       const share_type funds,
                                       const bool is_dao,
                                       const bool is_personal,
                                       const vector<std::pair<account_name_type, share_type>>& accounts)
{
    const auto& research_group = research_group_create(id, name, permlink, description, funds, is_dao, is_personal);

    for (const auto& account : accounts)
    {
        research_group_token_create(research_group.id, account.first, account.second);
    }

    return research_group;
}

const research_object& database_fixture::research_create(const int64_t id,
                                                         const string& title,
                                                         const string& abstract,
                                                         const string& permlink,
                                                         const research_group_id_type& research_group_id)
{
    const auto& new_research = db.create<research_object>([&](research_object& r) {
        r.id = id;
        fc::from_string(r.description, description);
        fc::from_string(r.permlink, permlink);
        r.research_group_id = research_group_id;
        r.is_finished = false;
        r.created_at = db.head_block_time();
        r.review_share_last_update = db.head_block_time();
    });

    return new_research;
}

const research_token_object& database_fixture::research_token_create(const int64_t id, 
                                                                     const account_name_type& owner,
                                                                     const uint16_t amount,
                                                                     const int64_t research_id)    
{
    const auto& new_research_token = db.create<research_token_object>([&](research_token_object& r) {
        r.id = id;
        r.account_name = owner;
        r.amount = amount;
        r.research_id = research_id;
    });

    return new_research_token;
}
    
const research_content_object& database_fixture::research_content_create(
                                const int64_t& id,
                                const int64_t& research_id,
                                const research_content_type& type,
                                const std::string& title,
                                const std::string& content,
                                const int16_t& activity_round,
                                const research_content_activity_state& activity_state,
                                const time_point_sec& activity_window_start,
                                const time_point_sec& activity_window_end,
                                const std::vector<account_name_type>& authors,
                                const std::vector<research_content_id_type>& references)
{
    const auto& new_research_content = db.create<research_content_object>([&](research_content_object& rc) {

        auto now = db.head_block_time();

        rc.id = id;
        rc.research_id = research_id;
        rc.type = type;
        fc::from_string(rc.description, description);
        fc::from_string(rc.content, content);
        rc.created_at = now;
        rc.authors.insert(authors.begin(), authors.end());
        rc.activity_round = activity_round;
        rc.activity_state = activity_state;
        rc.activity_window_start = activity_window_start;
        rc.activity_window_end = activity_window_end;
    });

    return new_research_content;
}

const expert_token_object& database_fixture::expert_token_create(const int64_t id,
                                                                 const account_name_type& account,
                                                                 const discipline_id_type& discipline_id,
                                                                 const share_type& amount)
{
    auto& expert_token = db.create<expert_token_object>([&](expert_token_object& token) {
        token.id = id;
        token.account_name = account;
        token.discipline_id = discipline_id;
        token.amount = amount;
    });
    return expert_token;
}

void database_fixture::create_disciplines()
{
    db.create<discipline_object>([&](discipline_object& d) {
        d.id = 1;
        d.name = "Physics";
        d.parent_id = 0;
    });

    db.create<discipline_object>([&](discipline_object& d) {
        d.id = 2;
        d.name = "Mathematics";
        d.parent_id = 0;
    });

    db.create<discipline_object>([&](discipline_object& d) {
        d.id = 3;
        d.name = "Cryptography";
        d.parent_id = 1;
    });
}


const research_token_sale_object& database_fixture::research_token_sale_create(const uint32_t id,
                                                                               research_id_type research_id,
                                                                               fc::time_point_sec start_time,
                                                                               fc::time_point_sec end_time,
                                                                               share_type total_amount,
                                                                               share_type soft_cap,
                                                                               share_type hard_cap)
{
    auto& research_token_sale = db.create<research_token_sale_object>([&](research_token_sale_object& rts_o) {
        rts_o.id = id;
        rts_o.research_id = research_id;
        rts_o.start_time = start_time;
        rts_o.end_time = end_time;
        rts_o.total_amount = total_amount;
        rts_o.soft_cap = soft_cap;
        rts_o.hard_cap = hard_cap;
        rts_o.status = static_cast<uint16_t>(research_token_sale_status::active);
    });
    return research_token_sale;
}

const research_token_sale_contribution_object& database_fixture::research_token_sale_contribution_create(research_token_sale_contribution_id_type id,
                                                                                                         research_token_sale_id_type research_token_sale_id,
                                                                                                         account_name_type owner,
                                                                                                         share_type amount,
                                                                                                         fc::time_point_sec contribution_time)
{
    auto& research_token_sale_contribution = db.create<research_token_sale_contribution_object>([&](research_token_sale_contribution_object& rtsc_o) {
        rtsc_o.id = id;
        rtsc_o.research_token_sale_id = research_token_sale_id;
        rtsc_o.owner = owner;
        rtsc_o.amount = amount;
        rtsc_o.contribution_time = contribution_time;
    });
    return research_token_sale_contribution;
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
                const auto& idx = db.get_index<account_balance_index>().indices().get<by_owner_and_asset_symbol>();
                if (idx.find(boost::make_tuple(account_name, DEIP_SYMBOL)) != idx.cend()){
                    auto& balance = db.get<account_balance_object, by_owner_and_asset_symbol>(boost::make_tuple(account_name, amount.symbol));
                    db.modify(balance, [&](account_balance_object& ab_o) {
                        ab_o.amount += amount.amount;
                    });
                }
                else {
                    db.create<account_balance_object>([&](account_balance_object &account_balance) {
                        account_balance.owner = account_name;
                        account_balance.symbol = amount.symbol;
                        account_balance.amount = amount.amount;
                    });
                }
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
        op.amount = asset(amount, DEIP_SYMBOL);

        trx.operations.push_back(op);
        trx.set_expiration(db.head_block_time() + DEIP_MAX_TIME_UNTIL_EXPIRATION);
        trx.validate();
        db.push_transaction(trx, ~0);
        trx.operations.clear();
    }
    FC_CAPTURE_AND_RETHROW((from)(to)(amount))
}

void database_fixture::create_all_discipline_expert_tokens_for_account(const string& account)
{    
    for (uint32_t i = 1; i < genesis_state.disciplines.size(); i++)
    {
        expert_token(account, i, 10000);
    }
}

void database_fixture::convert_deip_to_common_token(const string& from, const share_type& amount)
{
    try
    {
        transfer_to_common_tokens_operation op;
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

const expert_token_object&
database_fixture::expert_token(const string& account, const discipline_id_type& discipline_id, const share_type& amount)
{
    dbs_expert_token& expert_token_service = db.obtain_service<dbs_expert_token>();
    auto& expert_token = expert_token_service.create_expert_token(account, discipline_id, amount, false);

    return expert_token;
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

const asset database_fixture::get_balance(const string& account_name) const
{
    const auto& account_balance_service = db.obtain_service<dbs_account_balance>();
    const auto& account_balance = account_balance_service.get_account_balance_by_owner_and_asset(account_name, DEIP_SYMBOL);
    return asset(account_balance.amount, account_balance.symbol);
}

void database_fixture::sign(signed_transaction& trx, const fc::ecc::private_key& key)
{
    trx.sign(key, db.get_chain_id());
}

vector<operation> database_fixture::get_last_operations(uint32_t num_ops)
{
    using deip::blockchain_history::account_operations_full_history_index;

    vector<operation> ops;
    const auto& acc_hist_idx = db.get_index<account_operations_full_history_index>().indices().get<by_id>();
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
