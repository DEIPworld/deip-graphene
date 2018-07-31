#include <deip/chain/dbs_grant.hpp>
#include <deip/chain/database.hpp>
#include <deip/chain/dbs_account.hpp>
#include <deip/chain/account_object.hpp>

#include <deip/chain/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_grant::dbs_grant(database& db)
    : _base_type(db)
{
}

dbs_grant::grant_refs_type dbs_grant::get_grants() const
{
    grant_refs_type ret;

    auto idx = db_impl().get_index<grant_index>().indicies();
    auto it = idx.cbegin();
    const auto it_end = idx.cend();
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

std::set<string> dbs_grant::lookup_grant_owners(const string& lower_bound_owner_name, uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_LIMIT_GRANTS_LIST_SIZE,
              "limit must be less or equal than ${1}, actual limit value == ${2}",
              ("1", DEIP_LIMIT_GRANTS_LIST_SIZE)("2", limit));
    const auto& grants_by_owner_name = db_impl().get_index<grant_index>().indices().get<by_owner_name>();
    set<string> result;

    for (auto itr = grants_by_owner_name.lower_bound(lower_bound_owner_name);
         limit-- && itr != grants_by_owner_name.end(); ++itr)
    {
        result.insert(itr->owner);
    }

    return result;
}

dbs_grant::grant_refs_type dbs_grant::get_grants(const account_name_type& owner) const
{
    grant_refs_type ret;

    auto it_pair = db_impl().get_index<grant_index>().indicies().get<by_owner_name>().equal_range(owner);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const grant_object& dbs_grant::get_grant(grant_id_type id) const
{
    try {
        return db_impl().get<grant_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const grant_object& dbs_grant::create_grant(const account_object& owner,
                                            const asset& balance,
                                            const uint32_t& start_block,
                                            const uint32_t& end_block,
                                            const discipline_id_type& target_discipline,
                                            const bool is_extendable)
{
    // clang-format off
    FC_ASSERT(balance.symbol == DEIP_SYMBOL, "invalid asset type (symbol)");
    FC_ASSERT(balance.amount > 0, "invalid balance");
    FC_ASSERT(owner.balance >= balance, "insufficient funds");
    FC_ASSERT(_get_grants_count(owner.name) < DEIP_LIMIT_GRANTS_PER_OWNER,
              "can't create more then ${1} grants per owner", ("1", DEIP_LIMIT_GRANTS_PER_OWNER));
    // clang-format on

    const dynamic_global_property_object& props = db_impl().get_dynamic_global_properties();
    auto head_block_num = props.head_block_number;
    uint32_t start = start_block;
    if (start < head_block_num){
        start = head_block_num;
    }
    
    FC_ASSERT(start < end_block, "grant start block should be before end block");
    //Withdraw fund from account
    dbs_account& account_service = db().obtain_service<dbs_account>();
    account_service.decrease_balance(owner, balance);
    
    share_type per_block(balance.amount);
    per_block /= (end_block - start);

    FC_ASSERT(per_block >= DEIP_MIN_GRANT_PER_BLOCK,
            "We can't proceed grant that spend less than ${1} per block", ("1", DEIP_LIMIT_GRANTS_PER_OWNER));

    const grant_object& new_grant = db_impl().create<grant_object>([&](grant_object& grant) {
        grant.owner = owner.name;
        grant.target_discipline = target_discipline;
        grant.created = props.time;
        grant.start_block = start;
        grant.end_block = end_block;
        grant.balance = balance;
        grant.per_block = per_block;
        grant.is_extendable = is_extendable;
    });
    return new_grant;
}

asset dbs_grant::allocate_funds(const grant_object& grant)
{
    auto amount = asset(std::min(grant.per_block, grant.balance.amount), DEIP_SYMBOL);
    db_impl().modify(grant, [&](grant_object& b) {
        b.balance -= amount;
    });
    if (grant.balance.amount == 0){
        db_impl().remove(grant);
    }
    return amount;
}

uint64_t dbs_grant::_get_grants_count(const account_name_type& owner) const
{
    return db_impl().get_index<grant_index>().indicies().get<by_owner_name>().count(owner);
}

} // namespace chain
} // namespace deip
