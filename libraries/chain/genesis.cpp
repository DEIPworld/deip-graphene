#include <deip/chain/database.hpp>
#include <deip/chain/genesis_state.hpp>
#include <deip/chain/dbs_budget.hpp>

#include <deip/chain/account_object.hpp>
#include <deip/chain/block_summary_object.hpp>
#include <deip/chain/chain_property_object.hpp>
#include <deip/chain/deip_objects.hpp>
#include <deip/chain/discipline_object.hpp>
#include <deip/chain/expert_token_object.hpp>

#include <fc/io/json.hpp>

#define DEIP_DEFAULT_INIT_PUBLIC_KEY "STM5omawYzkrPdcEEcFiwLdEu7a3znoJDSmerNgf96J2zaHZMTpWs"
#define DEIP_DEFAULT_GENESIS_TIME fc::time_point_sec(1508331600);
#define DEIP_DEFAULT_INIT_SUPPLY (1000000u)

namespace deip {
namespace chain {
namespace utils {

using namespace deip::protocol;

void generate_default_genesis_state(genesis_state_type& genesis)
{
    const sp::public_key_type init_public_key(DEIP_DEFAULT_INIT_PUBLIC_KEY);

    genesis.init_supply = DEIP_DEFAULT_INIT_SUPPLY;
    genesis.initial_timestamp = DEIP_DEFAULT_GENESIS_TIME;

    genesis.accounts.push_back({ "initdelegate", "", init_public_key, genesis.init_supply, uint64_t(0) });

    genesis.witness_candidates.push_back({ "initdelegate", init_public_key });

    genesis.initial_chain_id = fc::sha256::hash(fc::json::to_string(genesis));
}

} // namespace utils

//////////////////////////////////////////////////////////////////////////
fc::time_point_sec database::get_genesis_time() const
{
    return _const_genesis_time;
}

void database::init_genesis(const genesis_state_type& genesis_state)
{
    try
    {
        FC_ASSERT(genesis_state.initial_timestamp != time_point_sec(), "Must initialize genesis timestamp.");
        FC_ASSERT(genesis_state.witness_candidates.size() > 0, "Cannot start a chain with zero witnesses.");

        struct auth_inhibitor
        {
            auth_inhibitor(database& db)
                : db(db)
                , old_flags(db.node_properties().skip_flags)
            {
                db.node_properties().skip_flags |= skip_authority_check;
            }

            ~auth_inhibitor()
            {
                db.node_properties().skip_flags = old_flags;
            }

        private:
            database& db;
            uint32_t old_flags;
        } inhibitor(*this);

        _const_genesis_time = genesis_state.initial_timestamp;
        create<chain_property_object>([&](chain_property_object& cp) { cp.chain_id = genesis_state.initial_chain_id; });

        init_genesis_accounts(genesis_state);
        init_genesis_witnesses(genesis_state);
        init_genesis_witness_schedule(genesis_state);
        init_genesis_global_property_object(genesis_state);
        init_genesis_disciplines(genesis_state);
        init_expert_tokens(genesis_state);

        // Nothing to do
        for (int i = 0; i < 0x10000; i++)
            create<block_summary_object>([&](block_summary_object&) {});

        create<hardfork_property_object>(
            [&](hardfork_property_object& hpo) { hpo.processed_hardforks.push_back(get_genesis_time()); });
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::init_genesis_accounts(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::account_type>& accounts = genesis_state.accounts;

    for (auto& account : accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");

        create<account_object>([&](account_object& a) {
            a.name = account.name;
            a.memo_key = account.public_key;
            a.balance = asset(account.deip_amount, DEIP_SYMBOL);
            a.json_metadata = "{created_at: 'GENESIS'}";
            a.recovery_account = account.recovery_account;
        });

        create<account_authority_object>([&](account_authority_object& auth) {
            auth.account = account.name;
            auth.owner.add_authority(account.public_key, 1);
            auth.owner.weight_threshold = 1;
            auth.active = auth.owner;
            auth.posting = auth.active;
        });
    }
}

void database::init_genesis_witnesses(const genesis_state_type& genesis_state)
{
    const std::vector<genesis_state_type::witness_type>& witnesses = genesis_state.witness_candidates;

    for (auto& witness : witnesses)
    {
        FC_ASSERT(!witness.owner_name.empty(), "Witness 'owner_name' should not be empty.");

        create<witness_object>([&](witness_object& w) {
            w.owner = witness.owner_name;
            w.signing_key = witness.block_signing_key;
            w.schedule = witness_object::top20;
            w.hardfork_time_vote = get_genesis_time();
        });
    }
}

void database::init_genesis_witness_schedule(const genesis_state_type& genesis_state)
{
    const std::vector<genesis_state_type::witness_type>& witness_candidates = genesis_state.witness_candidates;

    create<witness_schedule_object>([&](witness_schedule_object& wso) {
        for (size_t i = 0; i < wso.current_shuffled_witnesses.size() && i < witness_candidates.size(); ++i)
        {
            wso.current_shuffled_witnesses[i] = witness_candidates[i].owner_name;
        }
    });
}

void database::init_genesis_global_property_object(const genesis_state_type& genesis_state)
{
    create<dynamic_global_property_object>([&](dynamic_global_property_object& gpo) {
        gpo.time = get_genesis_time();
        gpo.recent_slots_filled = fc::uint128::max_value();
        gpo.participation_count = 128;
        gpo.current_supply = asset(genesis_state.init_supply, DEIP_SYMBOL);
        gpo.maximum_block_size = DEIP_MAX_BLOCK_SIZE;
    });
}

void database::init_genesis_disciplines(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::discipline_type>& disciplines = genesis_state.disciplines;

    for (auto& discipline : disciplines)
    {
        FC_ASSERT(!discipline.name.empty(), "Discipline 'name' should not be empty.");

        create<discipline_object>([&](discipline_object& d) {
            d.id = discipline.id;
            d.name = discipline.name;
            d.parent_id = discipline.parent_id;
            d.votes_in_last_ten_weeks = discipline.votes_in_last_ten_weeks;
        });
    }
}


void database::init_expert_tokens(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::expert_token_type>& expert_tokens = genesis_state.expert_tokens;

    for (auto& expert_token : expert_tokens)
    {
        FC_ASSERT(!expert_token.account_name.empty(), "Expertise token 'account_name' must not be empty.");
        FC_ASSERT(expert_token.discipline_id != 0,  "Expertise token 'discipline_id' must not be empty.");
        FC_ASSERT(expert_token.amount != 0,  "Expertise token 'amount' must not be equal to 0 for genesis state.");

        auto account = get<account_object, by_name>(expert_token.account_name);
        FC_ASSERT(account.name == expert_token.account_name); // verify that account exists

        auto discipline = get<discipline_object, by_id>(expert_token.discipline_id); // verify that discipline exists
        FC_ASSERT(discipline.id._id == expert_token.discipline_id); // verify that discipline exists

        create<expert_token_object>([&](expert_token_object& d) {
            d.account_name = account.name;
            d.discipline_id = discipline.id._id;
            d.amount = expert_token.amount;
        });
    }
}

} // namespace chain
} // namespace deip
