#include <deip/chain/services/dbs_discipline_supply.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/schema/account_object.hpp>

#include <deip/chain/database/database.hpp>

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
    FC_ASSERT(balance.symbol == DEIP_SYMBOL, "invalid asset type (symbol)");
    FC_ASSERT(balance.amount > 0, "invalid balance");
    FC_ASSERT(owner.balance >= balance, "insufficient funds");
    FC_ASSERT(_get_discipline_supplies_count(owner.name) < DEIP_LIMIT_DISCIPLINE_SUPPLIES_PER_OWNER,
              "can't create more then ${1} DISCIPLINE_SUPPLIES per owner", ("1", DEIP_LIMIT_DISCIPLINE_SUPPLIES_PER_OWNER));
    // clang-format on

    const dynamic_global_property_object& props = db_impl().get_dynamic_global_properties();
    auto head_block_num = props.head_block_number;
    uint32_t start = start_block;
    if (start < head_block_num){
        start = head_block_num;
    }
    
    FC_ASSERT(start < end_block, "discipline_supply start block should be before end block");
    //Withdraw fund from account
    dbs_account& account_service = db().obtain_service<dbs_account>();
    account_service.adjust_balance(owner, -balance);
    
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
    const auto& discipline_supply_expiration_index = db_impl().get_index<discipline_supply_index>().indices().get<by_end_block>();

    auto block_num = db_impl().head_block_num();
    auto discipline_supplies_itr = discipline_supply_expiration_index.upper_bound(block_num);

    while (discipline_supply_expiration_index.begin() != discipline_supplies_itr && is_expired(*discipline_supply_expiration_index.begin()))
    {
        auto& discipline_supply = *discipline_supply_expiration_index.begin();
        if(discipline_supply.balance.amount > 0)
        {
            auto& owner = db_impl().get_account(discipline_supply.owner);
            db_impl().modify(owner, [&](account_object& a) {
                a.balance += discipline_supply.balance;
            });
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

uint64_t dbs_discipline_supply::_get_discipline_supplies_count(const account_name_type &owner) const
{
    return db_impl().get_index<discipline_supply_index>().indicies().get<by_owner_name>().count(owner);
}

} // namespace chain
} // namespace deip
