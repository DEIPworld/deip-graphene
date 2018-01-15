#include <deip/chain/dbs_expert_token.hpp>
#include <deip/chain/database.hpp>

#include <tuple>

namespace deip {
namespace chain {

dbs_expert_token::dbs_expert_token(database &db)
    : _base_type(db)
{
}

const expert_token_object& dbs_expert_token::get_expert_token(const expert_token_id_type id) const
{
    return db_impl().get<expert_token_object>(id);
}

std::vector<const expert_token_object> dbs_expert_token::get_expert_tokens_by_account_id(const account_id_type account_id) const
{
    std::vector<const expert_token_object> ret;

    auto it_pair = db_impl().get_index<expert_token_index>().indicies().get<by_account_id>().equal_range(account_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        // expert_token_object test = *it;
        ret.push_back(*it);
        ++it;
    }

    return ret;
}


// test
// dbs_expert_token::expert_token_refs_type dbs_expert_token::get_expert_tokens_by_account_id(const account_id_type account_id) const
// {
//     expert_token_refs_type ret;

//     auto it_pair = db_impl().get_index<expert_token_index>().indicies().get<by_account_id>().equal_range(account_id);
//     auto it = it_pair.first;
//     const auto it_end = it_pair.second;
//     while (it != it_end)
//     {
//         expert_token_object test = *it;
//         ret.push_back(std::cref(*it));
//         ++it;
//     }

//     return ret;
// }




// std::vector<const expert_token_object> dbs_expert_token::get_expert_tokens_by_discipline_id(const discipline_id_type discipline_id) const
// {
//     std::vector<const expert_token_object> ret;

//     auto it_pair = db_impl().get_index<expert_token_index>().indicies().get<by_discipline_id>().equal_range(discipline_id);
//     auto it = it_pair.first;
//     const auto it_end = it_pair.second;
//     while (it != it_end)
//     {
//         ret.push_back(*it);
//         ++it;
//     }

//     return ret;
// }

} //namespace chain
} //namespace deip