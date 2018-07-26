#include <deip/chain/dbs_expert_token.hpp>
#include <deip/chain/dbs_account.hpp>
#include <deip/chain/dbs_discipline.hpp>
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
    auto& account_service = db_impl().obtain_service<dbs_account>();

    const auto& props = db_impl().get_dynamic_global_properties();
    const auto& to_account = account_service.get_account(account);

    FC_ASSERT(discipline_id != 0, "You cannot create expert token with discipline 0");

    account_service.increase_expertise_tokens(to_account, amount);
    account_service.adjust_proxied_witness_votes(to_account, amount);

    auto& token = db_impl().create<expert_token_object>([&](expert_token_object& token) {
        token.account_name = account;
        token.discipline_id = discipline_id;
        token.amount = amount;
        token.last_vote_time = props.time;
    });

    auto& discipline = db_impl().get<discipline_object, by_id>(discipline_id);

    db_impl().modify(discipline, [&](discipline_object d) {
        d.total_expertise_amount += amount;
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

bool dbs_expert_token::is_expert_token_existence_by_account_and_discipline(const account_name_type &account,
                                                                              const discipline_id_type &discipline_id)
{
    const auto& idx = db_impl().get_index<expert_token_index>().indices().get<by_account_and_discipline>();
    return idx.find(boost::make_tuple(account, discipline_id)) != idx.cend();
}

void dbs_expert_token::increase_expertise_tokens(const account_object &account,
                                                 const discipline_id_type &discipline_id,
                                                 const share_type &amount) {
    FC_ASSERT(amount >= 0, "Amount cannot be < 0");

    dbs_account& account_service = db_impl().obtain_service<dbs_account>();

    auto& token = get_expert_token_by_account_and_discipline(account.name, discipline_id);
    db_impl().modify(token, [&](expert_token_object et) {
        et.amount += amount;
    });

    auto& discipline = db_impl().get<discipline_object, by_id>(discipline_id);

    db_impl().modify(discipline, [&](discipline_object d) {
        d.total_expertise_amount += amount;
    });

    account_service.increase_expertise_tokens(account, amount);
}

} //namespace chain
} //namespace deip