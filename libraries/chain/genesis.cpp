#include <deip/chain/database/database.hpp>
#include <deip/chain/genesis_state.hpp>
#include <fc/io/json.hpp>

#include <deip/chain/schema/account_object.hpp>
#include <deip/chain/schema/block_summary_object.hpp>
#include <deip/chain/schema/chain_property_object.hpp>
#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/schema/discipline_object.hpp>
#include <deip/chain/schema/expert_token_object.hpp>
#include <deip/chain/schema/proposal_object.hpp>
#include <deip/chain/schema/research_group_object.hpp>
#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/schema/research_content_object.hpp>
#include <deip/chain/schema/research_discipline_relation_object.hpp>
#include <deip/chain/schema/expertise_contribution_object.hpp>
#include <deip/chain/schema/vesting_balance_object.hpp>
#include <deip/chain/services/dbs_discipline_supply.hpp>
#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_research_discipline_relation.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_asset.hpp>

#define DEIP_DEFAULT_INIT_PUBLIC_KEY "STM5omawYzkrPdcEEcFiwLdEu7a3znoJDSmerNgf96J2zaHZMTpWs"
#define DEIP_DEFAULT_GENESIS_TIME fc::time_point_sec(1508331600);
#define DEIP_DEFAULT_INIT_SUPPLY (1000000u)

    namespace deip {
namespace chain {
namespace utils {

using namespace deip::protocol;

void generate_default_genesis_state(genesis_state_type& genesis)
{
    const public_key_type init_public_key(DEIP_DEFAULT_INIT_PUBLIC_KEY);
    std::string symbol = asset(0, DEIP_SYMBOL).symbol_name();

    genesis_state_type::account_balance_type ballance;
    ballance.owner = "initdelegate";
    ballance.amount = genesis.init_supply;

    genesis.init_supply = DEIP_DEFAULT_INIT_SUPPLY;
    genesis.initial_timestamp = DEIP_DEFAULT_GENESIS_TIME;
    genesis.accounts.push_back({ "initdelegate", "", init_public_key });
    genesis.account_balances.push_back(ballance);
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

        init_genesis_global_property_object(genesis_state);
        init_genesis_assets(genesis_state);
        init_genesis_account_balances(genesis_state);
        init_genesis_accounts(genesis_state);
        init_genesis_research_groups(genesis_state);
        init_genesis_witnesses(genesis_state);
        init_genesis_witness_schedule(genesis_state);
        init_genesis_disciplines(genesis_state);
        init_genesis_expert_tokens(genesis_state);
        init_genesis_research(genesis_state);
        init_genesis_research_content(genesis_state);
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

    const genesis_state_type::registrar_account_type& registrar = genesis_state.registrar_account;
    const vector<genesis_state_type::account_type>& accounts = genesis_state.accounts;

    FC_ASSERT(!registrar.name.empty(), "Registrar account 'name' should not be empty.");
    FC_ASSERT(is_valid_account_name(registrar.name), "Registrar account name ${n} is invalid", ("n", registrar.name));

    create<account_object>([&](account_object& a) {
        a.name = registrar.name;
        a.memo_key = registrar.public_key;
        a.json_metadata = "";
        a.recovery_account = registrar.recovery_account;
        a.common_tokens_balance = registrar.common_tokens_amount;
    });

    auto& props = get<dynamic_global_property_object>();
    modify(props, [&](dynamic_global_property_object& gpo) {
        gpo.total_common_tokens_amount += registrar.common_tokens_amount;
        gpo.common_tokens_fund += asset(registrar.common_tokens_amount, DEIP_SYMBOL);
    });

    create<account_authority_object>([&](account_authority_object& auth) {
        auth.account = registrar.name;
        auth.owner.add_authority(registrar.public_key, 1);
        auth.owner.weight_threshold = 1;
        auth.active = auth.owner;
    });

    for (auto& account : accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");
        FC_ASSERT(is_valid_account_name(account.name), "Account name ${n} is invalid", ("n", account.name));

        auto owner_authority = authority();
        owner_authority.add_authority(account.public_key, 1);
        owner_authority.weight_threshold = 1;

        fc::optional<std::string> json_metadata;
        flat_map<uint16_t, authority> active_authority_overrides;
        flat_set<account_trait> traits;

        account_service.create_account_by_faucets(
          account.name,
          registrar.name,
          account.public_key,
          json_metadata,
          owner_authority,
          owner_authority,
          active_authority_overrides,
          DEIP_MIN_ACCOUNT_CREATION_FEE,
          traits,
          true
        );
    }
}


void database::init_genesis_assets(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::asset_type>& assets = genesis_state.assets;
    const vector<genesis_state_type::account_balance_type>& account_balances = genesis_state.account_balances;
    const genesis_state_type::registrar_account_type& registrar = genesis_state.registrar_account;

    const share_type liquid_total_supply = std::accumulate(
      account_balances.begin(), account_balances.end(), share_type(0),
      [&](share_type acc, const genesis_state_type::account_balance_type& account_balance) {
        return account_balance.symbol == asset(0, DEIP_SYMBOL).symbol_name() 
          ? share_type(account_balance.amount) + acc
          : acc;
      });

    FC_ASSERT(liquid_total_supply.value == genesis_state.init_supply - registrar.common_tokens_amount,
      "Total supply (${total}) is not equal to inited supply (${inited}) for ${s} asset",
      ("total", liquid_total_supply.value)("inited", genesis_state.init_supply)("s", asset(0, DEIP_SYMBOL).symbol_name()));

    create<asset_object>([&](asset_object& a_o) {
        const protocol::asset a = asset(0, DEIP_SYMBOL);

        a_o.symbol = a.symbol;
        fc::from_string(a_o.string_symbol, a.symbol_name());
        a_o.precision = a.decimals();
        a_o.current_supply = genesis_state.init_supply;
    });

    for (auto& asset : assets)
    {
        const int p = std::pow(10, asset.precision);
        const std::string string_asset = "0." + fc::to_string(p).erase(0, 1) + " " + asset.symbol;
        const protocol::asset a = asset::from_string(string_asset);

        const share_type asset_total_supply = std::accumulate(
          account_balances.begin(), account_balances.end(), share_type(0),
          [&](share_type acc, const genesis_state_type::account_balance_type& account_balance) {
              return account_balance.symbol == a.symbol_name()
                ? share_type(account_balance.amount) + acc
                : acc;
          });

        FC_ASSERT(asset_total_supply.value == asset.current_supply,
          "Total supply (${total}) is not equal to inited supply (${inited}) for ${s} asset",
          ("total", asset_total_supply.value)("inited", asset.current_supply)("s", a.symbol_name()));

        create<asset_object>([&](asset_object& a_o) {
          a_o.symbol = a.symbol;
          fc::from_string(a_o.string_symbol, a.symbol_name());
          a_o.precision = a.decimals();
          a_o.current_supply = asset.current_supply;
        });
    }
}

void database::init_genesis_account_balances(const genesis_state_type& genesis_state)
{
    const auto& assets_service = obtain_service<dbs_asset>();
    auto& account_balances_service = obtain_service<dbs_account_balance>();

    const vector<genesis_state_type::account_balance_type>& account_balances = genesis_state.account_balances;

    for (auto& account_balance : account_balances)
    {
        const auto& asset = assets_service.get_by_string_symbol(account_balance.symbol);
        account_balances_service.adjust_balance(account_balance.owner, deip::protocol::asset(account_balance.amount, asset.symbol));
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
    dbs_discipline& disciplines_service = obtain_service<dbs_discipline>();
    const vector<genesis_state_type::discipline_type>& disciplines = genesis_state.disciplines;

    for (auto& discipline : disciplines)
    {
        FC_ASSERT(!discipline.name.empty(), "Discipline 'name' should not be empty.");

        if (discipline.parent_external_id != "")
        {
            const auto& parent_discipline = disciplines_service.get_discipline(discipline.parent_external_id); // verify that discipline exists

            create<discipline_object>([&](discipline_object& d_o) {
                d_o.external_id = discipline.external_id;
                fc::from_string(d_o.name, discipline.name);
                d_o.parent_external_id = parent_discipline.external_id;
                d_o.parent_id = parent_discipline.id;
            });
        } 
        else 
        {
            create<discipline_object>([&](discipline_object& d_o) {
                d_o.external_id = discipline.external_id;
                fc::from_string(d_o.name, discipline.name);
            });
        }
    }
}


void database::init_genesis_expert_tokens(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::expert_token_type>& expert_tokens = genesis_state.expert_tokens;

    dbs_expert_token& expert_token_service = obtain_service<dbs_expert_token>();
    dbs_discipline& discipline_service = obtain_service<dbs_discipline>();
    dbs_account& accounts_service = obtain_service<dbs_account>();

    for (auto& expert_token : expert_tokens)
    {
        FC_ASSERT(!expert_token.account.empty(), "Expertise token 'account' must not be empty ${1}.", ("1", expert_token.discipline_external_id));
        FC_ASSERT(expert_token.amount != 0,  "Expertise token 'amount' must not be equal to 0 for genesis state.");

        const auto& account = accounts_service.get_account(expert_token.account); // verify that account exists
        const auto& discipline = discipline_service.get_discipline(expert_token.discipline_external_id);

        expert_token_service.create_expert_token(
          expert_token.account, 
          discipline.id,
          expert_token.amount, 
          true);
    }


#ifdef IS_TEST_NET
    if (find<account_object, by_name>("hermes") != nullptr) {
        // Init 'hermes' user with tokens in every discipline
        const auto& disciplines = discipline_service.lookup_disciplines(discipline_id_type(1), 10000);

        for (const discipline_object& discipline : disciplines)
        {
            if (discipline.id != 0) {
                expert_token_service.create_expert_token(
                  "hermes", 
                  discipline.id, 
                  10000, 
                  true);
            }
        }
    }
#endif
}



void database::init_genesis_research(const genesis_state_type& genesis_state)
{
    dbs_research& research_service = obtain_service<dbs_research>();
    dbs_research_group& research_groups_service = obtain_service<dbs_research_group>();
    dbs_discipline& disciplines_service = obtain_service<dbs_discipline>();

    const vector<genesis_state_type::research_type>& researches = genesis_state.researches;
    const vector<genesis_state_type::research_content_type>& research_contents = genesis_state.research_contents;

    for (auto& research : researches)
    {
        FC_ASSERT(!research.title.empty(), "Research 'title' is required");

        std::set<discipline_id_type> disciplines;
        for (auto& discipline_external_id : research.disciplines)
        {
            const auto& discipline = disciplines_service.get_discipline(discipline_external_id);
            disciplines.insert(discipline_id_type(discipline.id));
        }

        const time_point_sec genesis_time = get_genesis_time();
        const bool& is_private = false;
        
        const optional<percent>& review_share = optional<percent>(percent(0));
        const optional<percent>& compensation_share = optional<percent>(percent(0));

        const auto& research_group = research_groups_service.get_research_group_by_account(research.account);

        const auto& authors = std::accumulate(
            research_contents.begin(), research_contents.end(), std::set<account_name_type>(),
            [&](std::set<account_name_type> acc, const genesis_state_type::research_content_type& research_content) {
                if (research_content.research_external_id == research.external_id)
                {
                    acc.insert(research_content.authors.begin(), research_content.authors.end());
                }
                return acc;
            });

        flat_set<account_name_type> members;
        members.insert(authors.begin(), authors.end());

        research_service.create_research(
          research_group, 
          research.external_id,
          research.title, 
          research.abstract, 
          disciplines, 
          review_share,
          compensation_share,
          is_private,
          research.is_finished,
          percent(DEIP_100_PERCENT),
          members,
          genesis_time
        );
    }
}

void database::init_genesis_research_content(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::research_content_type>& research_contents = genesis_state.research_contents;
    auto& research_groups_service = obtain_service<dbs_research_group>();
    auto& research_service = obtain_service<dbs_research>();
    auto& research_content_service = obtain_service<dbs_research_content>();
    auto& accounts_service = obtain_service<dbs_account>();
    auto& research_discipline_relation_service = obtain_service<dbs_research_discipline_relation>();

    for (auto& research_content : research_contents)
    {
        FC_ASSERT(!research_content.title.empty(), "Research content 'title' is required");
        FC_ASSERT(!research_content.content.empty(), "Research content payload is require");
        FC_ASSERT(research_content.authors.size() > 0, "Research group should contain at least 1 member");

        for (auto& author : research_content.authors)
        {
            FC_ASSERT(accounts_service.account_exists(author), "Research content 'author' does not exist");
        }

        const auto& research = research_service.get_research(research_content.research_external_id);
        const auto& research_group = research_groups_service.get_research_group(research.research_group_id);

        const time_point_sec timestamp = get_genesis_time();
        flat_set<external_id_type> references;
        references.insert(research_content.references.begin(), research_content.references.end());

        const auto& created_research_content = research_content_service.create_research_content(
          research_group,
          research,
          research_content.external_id,
          research_content.title,
          research_content.content,
          static_cast<deip::chain::research_content_type>(research_content.type),
          research_content.authors,
          references,
          timestamp
        );

        for (auto& reference_external_id : research_content.references)
        {
            const auto& reference = research_content_service.get_research_content(reference_external_id);
            push_virtual_operation(research_content_reference_history_operation(
                created_research_content.id._id, 
                created_research_content.research_id._id,
                fc::to_string(created_research_content.content),
                reference.id._id, 
                reference.research_id._id,
                fc::to_string(reference.content)
            ));
        }

        const std::map<discipline_id_type, share_type> previous_research_content_eci = research_content_service.get_eci_evaluation(created_research_content.id);
        const std::map<discipline_id_type, share_type> previous_research_eci = research_service.get_eci_evaluation(research.id);

        const research_content_object& updated_research_content = research_content_service.update_eci_evaluation(created_research_content.id);
        const research_object& updated_research = research_service.update_eci_evaluation(research.id);

        const auto& relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research.id);

        for (auto& wrap : relations)
        {
            const auto& rel = wrap.get();

            flat_map<uint16_t, uint16_t> assessment_criterias;

            const eci_diff research_content_eci_diff = eci_diff(
                previous_research_content_eci.at(rel.discipline_id),
                updated_research_content.eci_per_discipline.at(rel.discipline_id),
                timestamp,
                static_cast<uint16_t>(expertise_contribution_type::publication),
                updated_research_content.id._id,
                assessment_criterias
            );

            push_virtual_operation(research_content_eci_history_operation(
                updated_research_content.id._id, 
                rel.discipline_id._id, 
                research_content_eci_diff
            ));

            const eci_diff research_eci_diff = eci_diff(
                previous_research_eci.at(rel.discipline_id), 
                updated_research.eci_per_discipline.at(rel.discipline_id),
                timestamp, 
                static_cast<uint16_t>(expertise_contribution_type::publication),
                updated_research_content.id._id,
                assessment_criterias
            );

            push_virtual_operation(research_eci_history_operation(
              updated_research.id._id, 
              rel.discipline_id._id, 
              research_eci_diff
            ));
        }
    }
}

void database::init_genesis_research_groups(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::research_group_type>& research_groups = genesis_state.research_groups;
    for (auto& research_group : research_groups)
    {
        init_genesis_research_group(research_group);
    }
}

void database::init_genesis_research_group(const genesis_state_type::research_group_type& research_group)
{
    auto& account_service = obtain_service<dbs_account>();
    auto& research_groups_service = obtain_service<dbs_research_group>();

    FC_ASSERT(!research_group.name.empty(), "Research group 'name' must be specified");
    FC_ASSERT(research_group.members.size() > 0, "Research group must contain at least 1 member");
    
    const auto& creator = account_service.get_account(research_group.creator);

    auto owner_authority = authority();
    owner_authority.weight_threshold = 1;

    auto active_authority = authority();
    active_authority.weight_threshold = 1;

    for (auto& member_name : research_group.members)
    {
        const auto& member = account_service.get_account(member_name);
        owner_authority.add_authority(account_name_type(member.name), 1);
        active_authority.add_authority(account_name_type(member.name), 1);
    }

    research_group_trait rg_trait;
    rg_trait.name = research_group.name;
    rg_trait.description = research_group.description;

    flat_set<account_trait> traits;
    traits.insert(rg_trait);

    fc::optional<std::string> json_metadata;
    flat_map<uint16_t, authority> active_authority_overrides;

    account_service.create_account_by_faucets(
      research_group.account,
      creator.name,
      creator.memo_key,
      json_metadata,
      owner_authority,
      active_authority,
      active_authority_overrides,
      DEIP_MIN_ACCOUNT_CREATION_FEE,
      traits,
      false
    );

    const auto& rg = research_groups_service.get_research_group_by_account(research_group.account);

    for (auto& member_name : research_group.members)
    {
        if (member_name != creator.name)
        {
            const auto& member = account_service.get_account(member_name);
            const share_type rgt = share_type(DEIP_100_PERCENT / research_group.members.size());
            research_groups_service.add_member_to_research_group(member.name, rg.id, rgt, creator.name);
        }
    }

    for (auto& subgroup : research_group.subgroups)
    {
        init_genesis_research_group(subgroup);
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
