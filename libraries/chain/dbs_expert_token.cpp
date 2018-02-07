#include <deip/chain/dbs_expert_token.hpp>
#include <deip/chain/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_expert_token::dbs_expert_token(database &db)
    : _base_type(db)
{
}

const expert_token_object& dbs_expert_token::get_expert_token(const expert_token_id_type& id) const
{
    return db_impl().get<expert_token_object>(id);
}

const expert_token_object& dbs_expert_token::get_expert_token_by_account_and_discipline(
        const account_name_type &account, const discipline_id_type &discipline_id) const
{
    return db_impl().get<expert_token_object, by_account_and_discipline>(std::make_tuple(account, discipline_id));
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