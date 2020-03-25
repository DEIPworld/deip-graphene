#include <deip/chain/database/database.hpp>

#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/services/dbs_discipline_supply.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_vote.hpp>

#include <deip/chain/util/reward.hpp>
#include <tuple>

namespace deip {
namespace chain {

dbs_discipline_supply::dbs_discipline_supply(database& db)
    : _base_type(db)
{
}

dbs_discipline_supply::discipline_supply_refs_type dbs_discipline_supply::get_discipline_supplies() const
{
    discipline_supply_refs_type ret;

    auto idx = db_impl().get_index<discipline_supply_index>().indicies();
    auto it = idx.cbegin();
    const auto it_end = idx.cend();
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

std::set<string> dbs_discipline_supply::lookup_discipline_supply_owners(const string &lower_bound_owner_name,
                                                                        uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_LIMIT_DISCIPLINE_SUPPLIES_LIST_SIZE,
              "limit must be less or equal than ${1}, actual limit value == ${2}",
              ("1", DEIP_LIMIT_DISCIPLINE_SUPPLIES_LIST_SIZE)("2", limit));
    const auto& discipline_supplies_by_owner_name = db_impl().get_index<discipline_supply_index>().indices().get<by_owner_name>();
    set<string> result;

    for (auto itr = discipline_supplies_by_owner_name.lower_bound(lower_bound_owner_name);
         limit-- && itr != discipline_supplies_by_owner_name.end(); ++itr)
    {
        result.insert(itr->owner);
    }

    return result;
}

dbs_discipline_supply::discipline_supply_refs_type dbs_discipline_supply::get_discipline_supplies_by_owner(const account_name_type &owner) const
{
    discipline_supply_refs_type ret;

    auto it_pair = db_impl().get_index<discipline_supply_index>().indicies().get<by_owner_name>().equal_range(owner);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const discipline_supply_object& dbs_discipline_supply::get_discipline_supply(discipline_supply_id_type id) const
{
    try {
        return db_impl().get<discipline_supply_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const discipline_supply_object& dbs_discipline_supply::create_discipline_supply(const account_object &owner,
                                                                                const asset &balance,
                                                                                const uint32_t &start_block,
                                                                                const uint32_t &end_block,
                                                                                const discipline_id_type &target_discipline,
                                                                                const bool is_extendable,
                                                                                const std::string &content_hash)
{
    // clang-format off
    FC_ASSERT(balance.amount > 0, "invalid balance");
    FC_ASSERT(_get_discipline_supplies_count(owner.name) < DEIP_LIMIT_DISCIPLINE_SUPPLIES_PER_OWNER,
              "can't create more then ${1} DISCIPLINE_SUPPLIES per owner", ("1", DEIP_LIMIT_DISCIPLINE_SUPPLIES_PER_OWNER));
    // clang-format on

    auto& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    account_balance_service.check_existence_by_owner_and_asset(owner.name, balance.symbol);
    account_balance_service.adjust_balance(owner.name, -balance);

    const dynamic_global_property_object& props = db_impl().get_dynamic_global_properties();
    auto head_block_num = props.head_block_number;
    uint32_t start = start_block;
    if (start < head_block_num){
        start = head_block_num;
    }
    
    FC_ASSERT(start < end_block, "discipline_supply start block should be before end block");
    //Withdraw fund from account
    
    share_type per_block(balance.amount);
    per_block /= (end_block - start);

    FC_ASSERT(per_block >= DEIP_MIN_DISCIPLINE_SUPPLY_PER_BLOCK,
            "We can't proceed discipline_supply that spend less than ${1} per block", ("1", DEIP_LIMIT_DISCIPLINE_SUPPLIES_PER_OWNER));

    const discipline_supply_object& new_discipline_supply = db_impl().create<discipline_supply_object>([&](discipline_supply_object& discipline_supply) {
        discipline_supply.owner = owner.name;
        discipline_supply.target_discipline = target_discipline;
        discipline_supply.created = props.time;
        discipline_supply.start_block = start;
        discipline_supply.end_block = end_block;
        discipline_supply.balance = balance;
        discipline_supply.per_block = per_block;
        discipline_supply.is_extendable = is_extendable;
        fc::from_string(discipline_supply.content_hash, content_hash);
    });
    return new_discipline_supply;
}

asset dbs_discipline_supply::allocate_funds(const discipline_supply_object& discipline_supply)
{
    auto amount = asset(std::min(discipline_supply.per_block, discipline_supply.balance.amount), DEIP_SYMBOL);
    db_impl().modify(discipline_supply, [&](discipline_supply_object& b) {
        b.balance -= amount;
    });
    if (discipline_supply.balance.amount == 0){
        db_impl().remove(discipline_supply);
    }
    return amount;
}

void dbs_discipline_supply::clear_expired_discipline_supplies()
{
    auto& account_balance_service = db_impl().obtain_service<dbs_account_balance>();

    const auto& discipline_supply_expiration_index = db_impl().get_index<discipline_supply_index>().indices().get<by_end_block>();

    auto block_num = db_impl().head_block_num();
    auto discipline_supplies_itr = discipline_supply_expiration_index.upper_bound(block_num);

    while (discipline_supply_expiration_index.begin() != discipline_supplies_itr && is_expired(*discipline_supply_expiration_index.begin()))
    {
        auto& discipline_supply = *discipline_supply_expiration_index.begin();
        if(discipline_supply.balance.amount > 0)
        {
            auto& owner = db_impl().get_account(discipline_supply.owner);
            account_balance_service.adjust_balance(owner.name, discipline_supply.balance);
            db_impl().modify(discipline_supply, [&](discipline_supply_object& g) {
                g.balance = asset(0, DEIP_SYMBOL);
            });
        }
        db_impl().remove(*discipline_supply_expiration_index.begin());
    }
}

bool dbs_discipline_supply::is_expired(const discipline_supply_object& discipline_supply)
{
    return discipline_supply.end_block < db_impl().head_block_num();
}

share_type dbs_discipline_supply::supply_researches_in_discipline(const discipline_id_type &discipline_id, const share_type &grant)
{
    dbs_discipline& discipline_service = db_impl().obtain_service<dbs_discipline>();
    dbs_research_content& research_content_service = db_impl().obtain_service<dbs_research_content>();
    dbs_vote& vote_service = db_impl().obtain_service<dbs_vote>();
    dbs_research& research_service = db_impl().obtain_service<dbs_research>();
    dbs_research_group& research_group_service = db_impl().obtain_service<dbs_research_group>();

    const auto& discipline = discipline_service.get_discipline(discipline_id);
    if (discipline.total_active_weight == 0) return 0;

    share_type used_grant = 0;
    share_type total_research_weight = discipline.total_active_weight;

    std::map<research_group_id_type, share_type> grant_shares_per_research;

    // Exclude final results from share calculation and discipline_supply distribution
    const auto& final_results_idx = db_impl().get_index<total_votes_index>().indices().get<by_discipline_and_content_type>();
    auto final_results_itr_pair = final_results_idx.equal_range(std::make_tuple(discipline.id, research_content_type::final_result));
    auto& final_results_itr = final_results_itr_pair.first;
    const auto& final_results_itr_end = final_results_itr_pair.second;

    while (final_results_itr != final_results_itr_end)
    {
        const auto& final_result = research_content_service.get(final_results_itr->research_content_id);
        if (final_result.activity_state == research_content_activity_state::active) {
            total_research_weight -= final_results_itr->total_weight;
        }
        ++final_results_itr;
    }


    auto total_votes = vote_service.get_total_votes_by_discipline(discipline.id);

    for (auto tvw : total_votes)
    {
        auto& total_vote = tvw.get();
        const auto& research_content = research_content_service.get(total_vote.research_content_id);

        if (research_content.type != research_content_type::final_result
            && research_content.activity_state == research_content_activity_state::active
            && total_vote.total_weight != 0) {
                auto share = util::calculate_share(grant, total_vote.total_weight, total_research_weight);
                auto& research = research_service.get_research(total_vote.research_id);
                research_group_service.increase_research_group_balance(research.research_group_id, asset(share, DEIP_SYMBOL));
                used_grant += share;
            }
    }

    if (used_grant > grant)
        wlog("Attempt to allocate discipline_supply amount that is greater than discipline_supply (supply_researches_in_discipline): ${used_grant} > ${grant}",
             ("used_grant", used_grant)("grant", grant));

    //FC_ASSERT(used_grant <= grant, "Attempt to allocate discipline_supply amount that is greater than discipline_supply");

    return used_grant;
}

void dbs_discipline_supply::process_discipline_supplies()
{
    uint32_t block_num = db_impl().head_block_num();

    const auto& discipline_supplies_idx = db_impl().get_index<discipline_supply_index>().indices().get<by_start_block>();

    auto discipline_supplies_itr = discipline_supplies_idx.upper_bound(block_num);

    for (auto itr = discipline_supplies_idx.begin(); itr != discipline_supplies_itr; ++itr)
    {
        auto& discipline_supply = *itr;
        auto used_discipline_supply = supply_researches_in_discipline(discipline_supply.target_discipline, discipline_supply.per_block);

        if (used_discipline_supply == 0 && discipline_supply.is_extendable)
            db_impl().modify(discipline_supply, [&](discipline_supply_object& g_o) { g_o.end_block++;} );
        else if (used_discipline_supply != 0)
            allocate_funds(discipline_supply);
    }
}

uint64_t dbs_discipline_supply::_get_discipline_supplies_count(const account_name_type &owner) const
{
    return db_impl().get_index<discipline_supply_index>().indicies().get<by_owner_name>().count(owner);
}

} // namespace chain
} // namespace deip
