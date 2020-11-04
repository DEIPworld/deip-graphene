#include <deip/chain/database/database.hpp>

#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/services/dbs_discipline_supply.hpp>
#include <deip/chain/services/dbs_research.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/services/dbs_expertise_contribution.hpp>

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

std::set<string> dbs_discipline_supply::lookup_discipline_supply_grantors(const string &lower_bound_grantor_name,
                                                                        uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_LIMIT_DISCIPLINE_SUPPLIES_LIST_SIZE,
              "limit must be less or equal than ${1}, actual limit value == ${2}",
              ("1", DEIP_LIMIT_DISCIPLINE_SUPPLIES_LIST_SIZE)("2", limit));
    const auto& discipline_supplies_by_grantor_name = db_impl().get_index<discipline_supply_index>().indices().get<by_grantor_name>();
    set<string> result;

    for (auto itr = discipline_supplies_by_grantor_name.lower_bound(lower_bound_grantor_name);
         limit-- && itr != discipline_supplies_by_grantor_name.end(); ++itr)
    {
        result.insert(itr->grantor);
    }

    return result;
}

dbs_discipline_supply::discipline_supply_refs_type dbs_discipline_supply::get_discipline_supplies_by_grantor(const account_name_type &grantor) const
{
    discipline_supply_refs_type ret;

    auto it_pair = db_impl().get_index<discipline_supply_index>().indicies().get<by_grantor_name>().equal_range(grantor);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const discipline_supply_object& dbs_discipline_supply::get_discipline_supply(const discipline_supply_id_type id) const
{
    try {
        return db_impl().get<discipline_supply_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const dbs_discipline_supply::discipline_supply_optional_ref_type
dbs_discipline_supply::get_discipline_supply_if_exists(const discipline_supply_id_type& id) const
{
    discipline_supply_optional_ref_type result;
    const auto& idx = db_impl()
            .get_index<discipline_supply_index>()
            .indicies()
            .get<by_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const discipline_supply_object& dbs_discipline_supply::create_discipline_supply(const account_name_type &grantor,
                                                                                const asset &balance,
                                                                                const fc::time_point_sec& start_time,
                                                                                const fc::time_point_sec& end_time,
                                                                                const discipline_id_type &target_discipline,
                                                                                const bool is_extendable,
                                                                                const std::string &content_hash,
                                                                                const flat_map<string, string>& additional_info)
{
    // clang-format off
    FC_ASSERT(_get_discipline_supplies_count(grantor) < DEIP_LIMIT_DISCIPLINE_SUPPLIES_PER_GRANTOR,
              "can't create more then ${1} DISCIPLINE_SUPPLIES per grantor", ("1", DEIP_LIMIT_DISCIPLINE_SUPPLIES_PER_GRANTOR));
    // clang-format on

    auto& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    account_balance_service.adjust_account_balance(grantor, -balance);

    const dynamic_global_property_object& props = db_impl().get_dynamic_global_properties();
    auto head_block_time = db_impl().head_block_time();
    fc::time_point_sec start = head_block_time;
    if (start < head_block_time){
        start = head_block_time;
    }
    
    FC_ASSERT(start < end_time, "Invalid discipline supply duration.");
    //Withdraw fund from account
    
    share_type per_block(balance.amount);
    per_block = (per_block * DEIP_BLOCK_INTERVAL) / (end_time.sec_since_epoch() - start.sec_since_epoch());

    FC_ASSERT(per_block >= DEIP_MIN_DISCIPLINE_SUPPLY_PER_BLOCK,
            "We can't proceed discipline_supply that spend less than ${1} per block", ("1", DEIP_LIMIT_DISCIPLINE_SUPPLIES_PER_GRANTOR));

    const discipline_supply_object& new_discipline_supply = db_impl().create<discipline_supply_object>([&](discipline_supply_object& discipline_supply) {
        discipline_supply.grantor = grantor;
        discipline_supply.target_discipline = target_discipline;
        discipline_supply.created = props.time;
        discipline_supply.start_time = start;
        discipline_supply.end_time = end_time;
        discipline_supply.balance = balance;
        discipline_supply.per_block = per_block;
        discipline_supply.is_extendable = is_extendable;
        fc::from_string(discipline_supply.content_hash, content_hash);

        for (auto& pair : additional_info)
        {
            string key = pair.first;
            int key_length = key.length();
            char key_array[key_length + 1];
            strcpy(key_array, key.c_str());

            string val = pair.second;
            int val_length = val.length();
            char val_array[val_length + 1];
            strcpy(val_array, val.c_str());

            discipline_supply.additional_info.insert(std::pair<fc::shared_string, fc::shared_string>(
                fc::shared_string(key_array, basic_string_allocator(db_impl().get_segment_manager())),
                fc::shared_string(val_array, basic_string_allocator(db_impl().get_segment_manager())))
            );
        }
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

    const auto& discipline_supply_expiration_index = db_impl().get_index<discipline_supply_index>().indices().get<by_end_time>();

    auto head_block_time = db_impl().head_block_time();
    auto discipline_supplies_itr = discipline_supply_expiration_index.upper_bound(head_block_time);

    while (discipline_supply_expiration_index.begin() != discipline_supplies_itr && is_expired(*discipline_supply_expiration_index.begin()))
    {
        auto& discipline_supply = *discipline_supply_expiration_index.begin();
        if(discipline_supply.balance.amount > 0)
        {
            auto used_discipline_supply = supply_researches_in_discipline(discipline_supply.target_discipline, discipline_supply.balance.amount);

            if (used_discipline_supply > 0)
            {
                db_impl().modify(discipline_supply, [&](discipline_supply_object& ds_o) {
                    ds_o.balance -= asset(used_discipline_supply, DEIP_SYMBOL);
                });
            }
            if (discipline_supply.balance.amount != 0)
            {
                auto& grantor = db_impl().get_account(discipline_supply.grantor);
                account_balance_service.adjust_account_balance(grantor.name, discipline_supply.balance);
                db_impl().modify(discipline_supply, [&](discipline_supply_object& g) {
                    g.balance = asset(0, DEIP_SYMBOL);
                });
            }
        }
        db_impl().remove(*discipline_supply_expiration_index.begin());
    }
}

bool dbs_discipline_supply::is_expired(const discipline_supply_object& discipline_supply)
{
    return discipline_supply.end_time < db_impl().head_block_time();
}

share_type dbs_discipline_supply::supply_researches_in_discipline(const discipline_id_type& discipline_id,
                                                                  const share_type& grant)
{
    dbs_discipline& discipline_service = db_impl().obtain_service<dbs_discipline>();
    dbs_research_content& research_content_service = db_impl().obtain_service<dbs_research_content>();
    dbs_expertise_contribution& expertise_contribution_service = db_impl().obtain_service<dbs_expertise_contribution>();
    dbs_research& research_service = db_impl().obtain_service<dbs_research>();
    dbs_research_group& research_group_service = db_impl().obtain_service<dbs_research_group>();
    dbs_account_balance& account_balance_service = db_impl().obtain_service<dbs_account_balance>();

    auto expertise_contributions = expertise_contribution_service.get_expertise_contributions_by_discipline(discipline_id);
    share_type total_eci_amount = std::accumulate(expertise_contributions.begin(), expertise_contributions.end(), share_type(0),
      [&](share_type acc, const expertise_contribution_object& exp) { return acc + exp.eci; });

    const auto& discipline = discipline_service.get_discipline(discipline_id);
    if (total_eci_amount == 0)
        return 0;

    share_type used_grant = 0;
    share_type total_research_weight = total_eci_amount;

    std::map<research_group_id_type, share_type> grant_shares_per_research;

    // Exclude final results from share calculation and discipline_supply distribution
    const auto& final_results_idx
        = db_impl().get_index<expertise_contribution_index>().indices().get<by_discipline_id>();
    auto research_content_itr_pair = final_results_idx.equal_range(discipline.id);
    auto& research_content_itr = research_content_itr_pair.first;
    const auto& research_content_end = research_content_itr_pair.second;

    while (research_content_itr != research_content_end)
    {
        const auto& research_content = research_content_service.get_research_content(research_content_itr->research_content_id);
        if (research_content.type == research_content_type::final_result && research_content.activity_state == research_content_activity_state::active)
        {
            total_research_weight -= research_content_itr->eci;
        }
        ++research_content_itr;
    }

    for (auto& wrap : expertise_contributions)
    {
        auto& expertise_contribution = wrap.get();
        const auto& research_content = research_content_service.get_research_content(expertise_contribution.research_content_id);

        if (research_content.type != research_content_type::final_result && 
            research_content.activity_state == research_content_activity_state::active && 
            expertise_contribution.eci != 0)
        {
            const auto share = util::calculate_share(grant, expertise_contribution.eci, total_research_weight);
            const auto& research = research_service.get_research(expertise_contribution.research_id);
            const auto& research_group = research_group_service.get_research_group(research.research_group_id);
            account_balance_service.adjust_account_balance(research_group.account, asset(share, DEIP_SYMBOL));

            used_grant += share;
        }
    }

    if (used_grant > grant)
        wlog("Attempt to allocate discipline_supply amount that is greater than discipline_supply "
             "(supply_researches_in_discipline): ${used_grant} > ${grant}",
             ("used_grant", used_grant)("grant", grant));

    // FC_ASSERT(used_grant <= grant, "Attempt to allocate discipline_supply amount that is greater than
    // discipline_supply");

    return used_grant;
}

void dbs_discipline_supply::process_discipline_supplies()
{
    fc::time_point_sec head_block_time = db_impl().head_block_time();

    const auto& discipline_supplies_idx = db_impl().get_index<discipline_supply_index>().indices().get<by_start_time>();

    auto discipline_supplies_itr = discipline_supplies_idx.upper_bound(head_block_time);

    for (auto itr = discipline_supplies_idx.begin(); itr != discipline_supplies_itr; ++itr)
    {
        auto& discipline_supply = *itr;
        auto used_discipline_supply
            = supply_researches_in_discipline(discipline_supply.target_discipline, discipline_supply.per_block);

        if (used_discipline_supply == 0 && discipline_supply.is_extendable)
            db_impl().modify(discipline_supply,
                             [&](discipline_supply_object& g_o) { g_o.end_time = g_o.end_time + DEIP_BLOCK_INTERVAL; });
        else if (used_discipline_supply != 0)
            allocate_funds(discipline_supply);
    }
}

uint64_t dbs_discipline_supply::_get_discipline_supplies_count(const account_name_type& grantor) const
{
    return db_impl().get_index<discipline_supply_index>().indicies().get<by_grantor_name>().count(grantor);
}

} // namespace chain
} // namespace deip
