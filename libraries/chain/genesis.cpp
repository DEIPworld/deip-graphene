#include <deip/chain/database/database.hpp>
#include <deip/chain/genesis_state.hpp>
#include <deip/chain/services/dbs_discipline_supply.hpp>

#include <deip/chain/schema/account_object.hpp>
#include <deip/chain/schema/block_summary_object.hpp>
#include <deip/chain/schema/chain_property_object.hpp>
#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/schema/discipline_object.hpp>
#include <deip/chain/schema/expert_token_object.hpp>
#include <deip/chain/schema/research_group_object.hpp>
#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/schema/research_content_object.hpp>
#include <deip/chain/schema/research_discipline_relation_object.hpp>
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/schema/proposal_object.hpp>
#include <deip/chain/schema/proposal_vote_object.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/schema/offer_research_tokens_object.hpp>

#include <fc/io/json.hpp>
#include <deip/chain/schema/vesting_balance_object.hpp>
#include <deip/chain/schema/expertise_stats_object.hpp>
#include <deip/chain/services/dbs_research_content.hpp>

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

        create<expertise_stats_object>([&](expertise_stats_object& es) {
           es.total_used_expertise = 0;
           es.used_expertise_per_block = 0;
        });

        init_genesis_global_property_object(genesis_state);
        init_genesis_accounts(genesis_state);
        init_genesis_witnesses(genesis_state);
        init_genesis_witness_schedule(genesis_state);
        init_genesis_disciplines(genesis_state);
        init_expert_tokens(genesis_state);
        init_research_groups(genesis_state);
        init_personal_research_groups(genesis_state);
        init_research(genesis_state);
        init_research_content(genesis_state);
        init_genesis_vesting_balances(genesis_state);

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
    auto& account_service = obtain_service<dbs_account>();

    const genesis_state_type::account_type& registrar = genesis_state.registrar_account;
    const vector<genesis_state_type::account_type>& accounts = genesis_state.accounts;

    FC_ASSERT(!registrar.name.empty(), "Registrar account 'name' should not be empty.");
    FC_ASSERT(is_valid_account_name(registrar.name), "Registrar account name ${n} is invalid", ("n", registrar.name));

    create<account_object>([&](account_object& a) {
        a.name = registrar.name;
        a.memo_key = registrar.public_key;
        a.balance = asset(registrar.deip_amount, DEIP_SYMBOL);
        a.json_metadata = "{created_at: 'GENESIS'}";
        a.recovery_account = registrar.recovery_account;
    });

    create<account_authority_object>([&](account_authority_object& auth) {
        auth.account = registrar.name;
        auth.owner.add_authority(registrar.public_key, 1);
        auth.owner.weight_threshold = 1;
        auth.active = auth.owner;
        auth.posting = auth.active;
    });

    for (auto& account : accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");
        FC_ASSERT(is_valid_account_name(account.name), "Account name ${n} is invalid", ("n", account.name));

        auto owner_authority = authority();
        owner_authority.add_authority(account.public_key, 1);
        owner_authority.weight_threshold = 1;

        auto& new_account = account_service.create_account_by_faucets(account.name,
                registrar.name,
                account.public_key,
                "{created at: 'GENESIS'}",
                owner_authority,
                owner_authority,
                owner_authority,
                asset(DEIP_MIN_ACCOUNT_CREATION_FEE, DEIP_SYMBOL));

        account_service.adjust_balance(new_account, account.deip_amount);
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
            d.total_active_weight = 0;
            d.total_expertise_amount = 0;
        });
    }
}


void database::init_expert_tokens(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::expert_token_type>& expert_tokens = genesis_state.expert_tokens;

    dbs_expert_token& expert_token_service = obtain_service<dbs_expert_token>();
    for (auto& expert_token : expert_tokens)
    {
        FC_ASSERT(!expert_token.account_name.empty(), "Expertise token 'account_name' must not be empty.");
        FC_ASSERT(expert_token.amount != 0,  "Expertise token 'amount' must not be equal to 0 for genesis state.");

        auto account = get<account_object, by_name>(expert_token.account_name);
        FC_ASSERT(account.name == expert_token.account_name); // verify that account exists

        auto& discipline = get<discipline_object, by_id>(expert_token.discipline_id); // verify that discipline exists
        FC_ASSERT(discipline.id._id == expert_token.discipline_id); // verify that discipline exists

        expert_token_service.create(expert_token.account_name, expert_token.discipline_id, expert_token.amount, true);
    }


#ifdef IS_TEST_NET
    if (find<account_object, by_name>("hermes") != nullptr) {
        // Init 'hermes' user with tokens in every discipline
        const vector<genesis_state_type::discipline_type>& disciplines = genesis_state.disciplines;

        for (auto& discipline : disciplines)
        {
            if (discipline.id != 0) {
                expert_token_service.create("hermes", discipline.id, 10000, true);
            }
        }
    }
#endif
}



void database::init_research(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::research_type>& researches = genesis_state.researches;
    const vector<genesis_state_type::research_content_type>& research_contents = genesis_state.research_contents;

    for (auto& research : researches)
    {
        FC_ASSERT(!research.title.empty(), "Research 'title' must be provided");
        FC_ASSERT(!research.permlink.empty(),  "Research group 'permlink' must be provided");
        FC_ASSERT(research.dropout_compensation_in_percent >= 0 && research.dropout_compensation_in_percent <= DEIP_100_PERCENT,
                  "Dropout compensation percent should be in 0% to 100% range");
        FC_ASSERT(research.review_share_in_percent >= 0 && research.review_share_in_percent <= 50 * DEIP_1_PERCENT,
                  "Percent for review should be in 0% to 50% range");

        uint16_t contents_amount = 0;

        for (auto& research_content : research_contents) {
            FC_ASSERT(!research_content.title.empty(), "Research content 'title' must be specified");
            FC_ASSERT(!research_content.content.empty(), "Research content must be specified");
            FC_ASSERT(research_content.authors.size() > 0, "Research group must contain at least 1 member");

            for (auto& author : research_content.authors)
                auto account = get<account_object, by_name>(author);

            if (research.id == research_content.research_id)
                contents_amount++;
        }

        create<research_object>([&](research_object& r){
            r.id = research.id;
            r.research_group_id = research.research_group_id;
            fc::from_string(r.title, research.title);
            fc::from_string(r.abstract, research.abstract);
            fc::from_string(r.permlink, research.permlink);
            r.created_at = get_genesis_time();
            r.last_update_time = get_genesis_time();
            r.is_finished = research.is_finished;
            r.owned_tokens = DEIP_100_PERCENT;
            r.review_share_in_percent = research.review_share_in_percent;
            r.review_share_in_percent_last_update = get_genesis_time();
            r.dropout_compensation_in_percent = research.dropout_compensation_in_percent;
            r.contents_amount = contents_amount;
        });

        for (auto& discipline_id : research.disciplines)
        {
            create<research_discipline_relation_object>([&](research_discipline_relation_object& rel){
                rel.research_id = research.id;
                rel.discipline_id = discipline_id;
                rel.votes_count = 0;
            });
        }
    }
}

void database::init_research_content(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::research_content_type>& research_contents = genesis_state.research_contents;
    
    for (auto& research_content : research_contents)
    {
        FC_ASSERT(!research_content.title.empty(), "Research content 'title' must be specified");
        FC_ASSERT(!research_content.content.empty(),  "Research content must be specified");
        FC_ASSERT(research_content.authors.size() > 0, "Research group must contain at least 1 member");

        for (auto& author : research_content.authors)
        {
            // validate for account existence
            auto account = get<account_object, by_name>(author);
        }

        auto research = get<research_object, by_id>(research_content.research_id);

        auto& c = create<research_content_object>([&](research_content_object& rc) {

            rc.research_id = research_content.research_id;
            rc.type = static_cast<deip::chain::research_content_type>(research_content.type);
            fc::from_string(rc.title, research_content.title);
            fc::from_string(rc.content, research_content.content);
            fc::from_string(rc.permlink, research_content.permlink);
            rc.authors.insert(research_content.authors.begin(), research_content.authors.end());
            rc.references.insert(research_content.references.begin(), research_content.references.end());
            rc.created_at = get_genesis_time();
            rc.activity_round = 1;
            rc.activity_state = static_cast<deip::chain::research_content_activity_state>(1);
            rc.activity_window_start = get_genesis_time();
            rc.activity_window_end = get_genesis_time() + DAYS_TO_SECONDS(14);
        });
    }
}

void database::init_research_groups(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::research_group_type>& research_groups = genesis_state.research_groups;

    for (auto& research_group : research_groups)
    {
        FC_ASSERT(!research_group.name.empty(), "Research group 'name' must be specified");
        FC_ASSERT(!research_group.permlink.empty(), "Research group 'permlink' must be specified");
        FC_ASSERT(research_group.members.size() > 0, "Research group must contain at least 1 member");
        FC_ASSERT(research_group.quorum_percent >= 5 * DEIP_1_PERCENT && research_group.quorum_percent <= DEIP_100_PERCENT, "Quorum percent must be in 5% to 100% range");

        create<research_group_object>([&](research_group_object& rg) {
            rg.id = research_group.id;
            fc::from_string(rg.name, research_group.name);
            fc::from_string(rg.description, research_group.description);
            fc::from_string(rg.permlink, research_group.permlink);
            rg.balance = asset(0, DEIP_SYMBOL);
            rg.quorum_percent = research_group.quorum_percent;

            std::map<uint16_t , share_type> proposal_quorums;

            for (int i = First_proposal; i <= Last_proposal; i++)
                proposal_quorums.insert(std::make_pair(i, research_group.quorum_percent));

            rg.proposal_quorums.insert(proposal_quorums.begin(), proposal_quorums.end());
            rg.is_personal = research_group.is_personal;
        });

        // TODO: Check that total amount of research group tokens is 10000
        for (auto& member : research_group.members)
        {
            auto account = get<account_object, by_name>(member);
            create<research_group_token_object>([&](research_group_token_object& rgt) {
               rgt.research_group_id = research_group.id;
               rgt.amount = DEIP_100_PERCENT / research_group.members.size();
               rgt.owner = account.name;
           });
        }
    }
}

void database::init_personal_research_groups(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::account_type>& accounts = genesis_state.accounts;

    for (auto& account : accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");
        FC_ASSERT(is_valid_account_name(account.name), "Account name ${n} is invalid", ("n", account.name));

        std::map<uint16_t, share_type> personal_research_group_proposal_quorums;

        for (int i = First_proposal; i <= Last_proposal; i++)
            personal_research_group_proposal_quorums.insert(std::make_pair(i, DEIP_100_PERCENT));

        auto& research_group = create<research_group_object>([&](research_group_object& research_group) {
            fc::from_string(research_group.name, account.name);
            fc::from_string(research_group.permlink, account.name);
            fc::from_string(research_group.description, account.name);
            research_group.proposal_quorums.insert(personal_research_group_proposal_quorums.begin(), personal_research_group_proposal_quorums.end());
            research_group.is_personal = true;
        });

        create<research_group_token_object>([&](research_group_token_object& research_group_token) {
            research_group_token.research_group_id = research_group.id;
            research_group_token.amount = DEIP_100_PERCENT;
            research_group_token.owner = account.name;
        });
    }
}

void database::init_genesis_vesting_balances(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::vesting_balance_type>& vesting_balances = genesis_state.vesting_balances;

    for (auto& vesting_balance : vesting_balances)
    {

        FC_ASSERT(vesting_balance.balance > 0, "Deposit balance must be greater than 0");
        FC_ASSERT(vesting_balance.vesting_duration_seconds > 0, "Vesting duration must be longer than 0");
        FC_ASSERT(vesting_balance.vesting_duration_seconds > vesting_balance.vesting_cliff_seconds,
                "Vesting duration should be longer than vesting cliff");
        FC_ASSERT(vesting_balance.vesting_cliff_seconds > 0, "Vesting cliff must be longer than 0");
        FC_ASSERT(vesting_balance.period_duration_seconds > 0, "Withdraw period duration must be longer than 0");
        FC_ASSERT(vesting_balance.vesting_duration_seconds % vesting_balance.period_duration_seconds == 0,
                "Vesting duration should contain integer number of withdraw period");

        FC_ASSERT(!vesting_balance.owner.empty(), "Account 'name' should not be empty.");
        FC_ASSERT(is_valid_account_name(vesting_balance.owner), "Account name ${n} is invalid", ("n", vesting_balance.owner));

        create<vesting_balance_object>([&](vesting_balance_object& v) {
            v.id = vesting_balance.id;
            v.owner = vesting_balance.owner;
            v.balance = asset(vesting_balance.balance, DEIP_SYMBOL);
            v.withdrawn = asset(0, DEIP_SYMBOL);
            v.vesting_duration_seconds = vesting_balance.vesting_duration_seconds;
            v.vesting_cliff_seconds = vesting_balance.vesting_duration_seconds;
            v.period_duration_seconds = vesting_balance.period_duration_seconds;
            v.start_timestamp = get_genesis_time();
        });
    }
}


} // namespace chain
} // namespace deip
