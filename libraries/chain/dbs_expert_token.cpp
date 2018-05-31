#include <deip/chain/dbs_expert_token.hpp>
#include <deip/chain/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_expert_token::dbs_expert_token(database &db)
    : _base_type(db)
{
}

const expert_token_object& dbs_expert_token::create(const account_name_type &account,
                                                    const discipline_id_type &discipline_id,
                                                    const share_type& amount)
{
    const auto& cprops = db_impl().get_dynamic_global_properties();
    const auto& to_account = db_impl().get<account_object, by_name>(account);

    if (discipline_id != 0)
    {
        db_impl().modify(to_account, [&](account_object& to) { to.total_expert_tokens_amount += amount; });
        db_impl().modify(cprops,
                         [&](dynamic_global_property_object& props) { props.total_expert_tokens_amount += amount; });
    }
    else
    {
        db_impl().modify(to_account, [&](account_object& to) { to.total_common_tokens_amount += amount; });
        db_impl().modify(cprops,
                         [&](dynamic_global_property_object& props) { props.total_common_tokens_amount += amount; });
    }

    auto& token = db_impl().create<expert_token_object>([&](expert_token_object& token) {
        token.account_name = account;
        token.discipline_id = discipline_id;
        token.amount = amount;
        token.last_vote_time = cprops.time;
    });

    return token;
}

const expert_token_object& dbs_expert_token::get_expert_token(const expert_token_id_type& id) const
{
    try {
        return db_impl().get<expert_token_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

const expert_token_object& dbs_expert_token::get_expert_token_by_account_and_discipline(
        const account_name_type &account, const discipline_id_type &discipline_id) const
{
    try {
        return db_impl().get<expert_token_object, by_account_and_discipline>(std::make_tuple(account, discipline_id));
    }
    FC_CAPTURE_AND_RETHROW((discipline_id))
}

dbs_expert_token::expert_token_refs_type dbs_expert_token::get_expert_tokens_by_account_name(const account_name_type& account_name) const
{
    expert_token_refs_type ret;

    auto it_pair = db_impl().get_index<expert_token_index>().indicies().get<by_account_name>().equal_range(account_name);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_expert_token::expert_token_refs_type dbs_expert_token::get_expert_tokens_by_discipline_id(const discipline_id_type& discipline_id) const
{
    expert_token_refs_type ret;

    auto it_pair = db_impl().get_index<expert_token_index>().indicies().get<by_discipline_id>().equal_range(discipline_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

void dbs_expert_token::check_expert_token_existence_by_account_and_discipline(const account_name_type &account,
                                                                              const discipline_id_type &discipline_id)
{
    const auto& idx = db_impl().get_index<expert_token_index>().indices().get<by_account_and_discipline>();

    FC_ASSERT(idx.find(std::make_tuple(account, discipline_id)) != idx.cend(), "Expert token for account \"${1}\" and discipline \"${2}\" does not exist", ("1", account)("2", discipline_id));
}


} //namespace chain
} //namespace deip