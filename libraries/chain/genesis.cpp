#include <deip/chain/database/database.hpp>
#include <deip/chain/genesis_state.hpp>
#include <fc/io/json.hpp>

#include <deip/chain/schema/account_object.hpp>
#include <deip/chain/schema/block_summary_object.hpp>
#include <deip/chain/schema/chain_property_object.hpp>
#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/schema/discipline_object.hpp>
#include <deip/chain/schema/expert_token_object.hpp>
#include <deip/chain/schema/offer_research_tokens_object.hpp>
#include <deip/chain/schema/proposal_object.hpp>
#include <deip/chain/schema/proposal_vote_object.hpp>
#include <deip/chain/schema/research_group_object.hpp>
#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/schema/research_content_object.hpp>
#include <deip/chain/schema/research_discipline_relation_object.hpp>
#include <deip/chain/schema/expertise_contribution_object.hpp>
#include <deip/chain/schema/vesting_balance_object.hpp>
#include <deip/chain/services/dbs_discipline_supply.hpp>
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
        init_genesis_accounts(genesis_state);
        init_genesis_research_groups(genesis_state);
        init_genesis_account_balances(genesis_state);
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
        auth.posting = auth.active;
    });

    for (auto& account : accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");
        FC_ASSERT(is_valid_account_name(account.name), "Account name ${n} is invalid", ("n", account.name));

        auto owner_authority = authority();
        owner_authority.add_authority(account.public_key, 1);
        owner_authority.weight_threshold = 1;

        account_service.create_account_by_faucets(
          account.name,
          registrar.name,
          account.public_key,
          "",
          owner_authority,
          owner_authority,
          owner_authority,
          asset(0, DEIP_SYMBOL),
          {},
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
    const vector<genesis_state_type::discipline_type>& disciplines = genesis_state.disciplines;

    for (auto& discipline : disciplines)
    {
        FC_ASSERT(!discipline.name.empty(), "Discipline 'name' should not be empty.");

        create<discipline_object>([&](discipline_object& d) {
            d.id = discipline.id;
            fc::from_string(d.name, discipline.name);
            d.parent_id = discipline.parent_id;
        });
    }
}


void database::init_genesis_expert_tokens(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::expert_token_type>& expert_tokens = genesis_state.expert_tokens;

    dbs_expert_token& expert_token_service = obtain_service<dbs_expert_token>();
    for (auto& expert_token : expert_tokens)
    {
        FC_ASSERT(!expert_token.account_name.empty(), "Expertise token 'account_name' must not be empty ${1}.", ("1", expert_token.discipline_id));
        FC_ASSERT(expert_token.amount != 0,  "Expertise token 'amount' must not be equal to 0 for genesis state.");

        auto account = get<account_object, by_name>(expert_token.account_name);
        FC_ASSERT(account.name == expert_token.account_name); // verify that account exists

        auto& discipline = get<discipline_object, by_id>(expert_token.discipline_id); // verify that discipline exists
        FC_ASSERT(discipline.id._id == expert_token.discipline_id); // verify that discipline exists

        expert_token_service.create_expert_token(expert_token.account_name, expert_token.discipline_id, expert_token.amount, true);
    }


#ifdef IS_TEST_NET
    if (find<account_object, by_name>("hermes") != nullptr) {
        // Init 'hermes' user with tokens in every discipline
        const vector<genesis_state_type::discipline_type>& disciplines = genesis_state.disciplines;

        for (auto& discipline : disciplines)
        {
            if (discipline.id != 0) {
                expert_token_service.create_expert_token("hermes", discipline.id, 10000, true);
            }
        }
    }
#endif
}



void database::init_genesis_research(const genesis_state_type& genesis_state)
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
        std::vector<account_name_type> members;

        for (auto& research_content : research_contents) {
            FC_ASSERT(!research_content.title.empty(), "Research content 'title' must be specified");
            FC_ASSERT(!research_content.content.empty(), "Research content must be specified");
            FC_ASSERT(research_content.authors.size() > 0, "Research group must contain at least 1 member");

            for (auto& author : research_content.authors)
                auto account = get<account_object, by_name>(author);

            if (research.id == research_content.research_id) {
                for (auto& author : research_content.authors)
                    members.push_back(author);
                contents_amount++;
            }
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
            for (auto member : members)
                r.members.insert(member);
            r.is_private = research.is_private;
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

void database::init_genesis_research_content(const genesis_state_type& genesis_state)
{
    const vector<genesis_state_type::research_content_type>& research_contents = genesis_state.research_contents;
    dbs_research_content& research_content_service = obtain_service<dbs_research_content>();
    dbs_research& research_service = obtain_service<dbs_research>();
    dbs_research_discipline_relation& research_discipline_relation_service = obtain_service<dbs_research_discipline_relation>();

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
            rc.id = research_content.id;
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
            rc.activity_window_end = get_genesis_time() + DEIP_REGULAR_CONTENT_ACTIVITY_WINDOW_DURATION;
        });

        for (auto& reference : research_content.references)
        {
            auto& _content = get<research_content_object>(reference);
            push_virtual_operation(research_content_reference_history_operation(c.id._id,
                                                                       c.research_id._id,
                                                                       fc::to_string(c.content),
                                                                       _content.id._id,
                                                                       _content.research_id._id,
                                                                       fc::to_string(_content.content)));
        }

        const std::map<discipline_id_type, share_type> previous_research_content_eci = research_content_service.get_eci_evaluation(research_content.id);
        const std::map<discipline_id_type, share_type> previous_research_eci = research_service.get_eci_evaluation(research.id);

        const research_content_object& updated_research_content = research_content_service.update_eci_evaluation(c.id);
        const research_object& updated_research = research_service.update_eci_evaluation(research.id);

        auto relations = research_discipline_relation_service.get_research_discipline_relations_by_research(research_content.research_id);
        for (auto& wrap : relations)
        {
            const auto& rel = wrap.get();

            const eci_diff research_content_eci_diff = eci_diff(
              previous_research_content_eci.at(rel.discipline_id),
              updated_research_content.eci_per_discipline.at(rel.discipline_id),
              get_genesis_time(),
              static_cast<uint16_t>(expertise_contribution_type::publication),
              updated_research_content.id._id
            );

            push_virtual_operation(research_content_eci_history_operation(
              updated_research_content.id._id, 
              rel.discipline_id._id,
              research_content_eci_diff)
            );

            const eci_diff research_eci_diff = eci_diff(
              previous_research_eci.at(rel.discipline_id),
              updated_research.eci_per_discipline.at(rel.discipline_id),
              get_genesis_time(),
              static_cast<uint16_t>(expertise_contribution_type::publication),
              updated_research_content.id._id
            );

            push_virtual_operation(research_eci_history_operation(
              updated_research.id._id, 
              rel.discipline_id._id,
              research_eci_diff)
            );
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
    FC_ASSERT(!research_group.permlink.empty(), "Research group 'permlink' must be specified");
    FC_ASSERT(research_group.members.size() > 0, "Research group must contain at least 1 member");
    
    const auto& creator = account_service.get_account(research_group.creator);

    auto owner_authority = authority();
    owner_authority.add_authority(creator.name, 1);
    owner_authority.weight_threshold = 1;

    auto active_authority = authority();
    auto posting_authority = authority();

    for (auto& member_name : research_group.members)
    {
        const auto& member = account_service.get_account(member_name);

        active_authority.add_authority(account_name_type(member.name), 1);
        active_authority.weight_threshold += 1;

        posting_authority.add_authority(account_name_type(member.name), 1);
        posting_authority.weight_threshold += 1;
    }

    std::string id("");
    id.append(research_group.name);
    id.append(research_group.permlink);
    id.append(research_group.description);

    std::string rg_account = (std::string)fc::ripemd160::hash(id);

    research_group_v1_0_0_trait research_group_trait;
    research_group_trait.name = research_group.name;
    research_group_trait.permlink = research_group.permlink;
    research_group_trait.description = research_group.description;

    vector<account_trait> traits = { research_group_trait };

    account_service.create_account_by_faucets(
      rg_account,
      creator.name,
      creator.memo_key,
      "",
      owner_authority,
      active_authority,
      posting_authority,
      asset(0, DEIP_SYMBOL),
      traits,
      false
    );

    const auto& rg = research_groups_service.get_research_group_by_account(rg_account);

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
