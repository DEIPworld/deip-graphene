
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/services/dbs_expertise_allocation_proposal.hpp>
#include <deip/chain/database/database.hpp>
#include <tuple>

namespace deip {
namespace chain {

dbs_expert_token::dbs_expert_token(database &db)
    : _base_type(db)
{
}

const expert_token_object& dbs_expert_token::create_expert_token(
  const account_name_type& name, 
  const discipline_id_type& discipline_id,
  const share_type& amount,
  const bool& create_parent = true)
{
    auto& account_service = db_impl().obtain_service<dbs_account>();
    const auto& props = db_impl().get_dynamic_global_properties();
    const auto& account = account_service.get_account(name);

    FC_ASSERT(discipline_id != 0, "Expertise token can not be created for the common discipline");

    const auto& exp = db_impl().create<expert_token_object>([&](expert_token_object& exp_o) {
        exp_o.account_name = name;
        exp_o.discipline_id = discipline_id;
        exp_o.amount = amount;
        if (exp_o.amount < 0) 
        {
            exp_o.amount = 0;
        }
        exp_o.last_vote_time = props.time;
    });

    account_service.adjust_expertise_tokens_throughput(account, amount);
    adjust_proxied_expertise(exp, amount);

    if (create_parent)
    {
        const auto& discipline = db_impl().get<discipline_object>(discipline_id);
        if (discipline.parent_id != 0 && !expert_token_exists_by_account_and_discipline(name, discipline.parent_id))
        {
            create_expert_token(name, discipline.parent_id, amount, true);
        }
    }

    return exp;
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

const bool dbs_expert_token::expert_token_exists_by_account_and_discipline(
  const account_name_type &account,
  const discipline_id_type &discipline_id) const
{
    const auto& idx = db_impl()
      .get_index<expert_token_index>()
      .indices()
      .get<by_account_and_discipline>();

    return idx.find(std::make_tuple(account, discipline_id)) != idx.end();
}

const std::tuple<share_type, share_type> dbs_expert_token::adjust_expert_token( 
  const account_name_type& name,
  const discipline_id_type& discipline_id,
  const share_type& delta) 
{
    dbs_account& accounts_service = db_impl().obtain_service<dbs_account>();
    const auto& account = accounts_service.get_account(name);

    if (expert_token_exists_by_account_and_discipline(name, discipline_id))
    {
        const expert_token_object& exp = get_expert_token_by_account_and_discipline(account.name, discipline_id);
        share_type previous = exp.amount;
        db_impl().modify(exp, [&](expert_token_object& exp_o) {
            exp_o.amount += delta;
            if (exp_o.amount < 0)
            {
                exp_o.amount = 0;
            }
        });

        accounts_service.adjust_expertise_tokens_throughput(account, delta);
        adjust_proxied_expertise(exp, delta);
        return std::make_tuple(previous, exp.amount);
    }
    else 
    {
        share_type previous = share_type(0);
        const expert_token_object& exp = create_expert_token(name, discipline_id, delta, true);
        return std::make_tuple(previous, exp.amount);
    }
}

void dbs_expert_token::update_expertise_proxy(const expert_token_object& expert_token, const optional<expert_token_object>& proxy_expert_token)
{
    /// remove all current votes
    std::array<share_type, DEIP_MAX_PROXY_RECURSION_DEPTH + 1> delta;

    delta[0] = -expert_token.amount;

    for (int i = 0; i < DEIP_MAX_PROXY_RECURSION_DEPTH; ++i)
        delta[i + 1] = -expert_token.proxied_expertise[i];

    adjust_proxied_expertise(expert_token, delta);

    if (proxy_expert_token.valid())
    {
        flat_set<expert_token_id_type> proxy_chain({expert_token.id, (*proxy_expert_token).id});

        proxy_chain.reserve(DEIP_MAX_PROXY_RECURSION_DEPTH + 1);

        /// check for proxy loops and fail to update the proxy if it would create a loop
        auto cprox = &(*proxy_expert_token);
        while (cprox->proxy.size() != 0) {
            const auto next_proxy = get_expert_token_by_account_and_discipline(cprox->proxy, cprox->discipline_id);
            FC_ASSERT(proxy_chain.insert(next_proxy.id).second, "This proxy would create a proxy loop.");
            cprox = &next_proxy;
            FC_ASSERT(proxy_chain.size() <= DEIP_MAX_PROXY_RECURSION_DEPTH, "Proxy chain is too long.");
        }

        db_impl().modify(expert_token, [&](expert_token_object &e) { e.proxy = (*proxy_expert_token).account_name; });

        for (int i = 0; i <= DEIP_MAX_PROXY_RECURSION_DEPTH; ++i)
            delta[i] = -delta[i];
        adjust_proxied_expertise(expert_token, delta);
    }

    else
    { /// we are clearing the proxy which means we simply update the expert token
        db_impl().modify(expert_token,
                         [&](expert_token_object& e) { e.proxy = account_name_type(DEIP_PROXY_TO_SELF_ACCOUNT); });
    }
}

void dbs_expert_token::adjust_proxied_expertise(const expert_token_object& expert_token,
                                                const std::array<share_type, DEIP_MAX_PROXY_RECURSION_DEPTH + 1>& delta,
                                                int depth)
{
    if (expert_token.proxy != DEIP_PROXY_TO_SELF_EXPERT_TOKEN)
    {
        /// nested proxies are not supported, vote will not propagate
        if (depth >= DEIP_MAX_PROXY_RECURSION_DEPTH)
            return;

        const auto& proxy = get_expert_token_by_account_and_discipline(expert_token.proxy, expert_token.discipline_id);

        db_impl().modify(proxy, [&](expert_token_object& e) {
            for (int i = DEIP_MAX_PROXY_RECURSION_DEPTH - depth - 1; i >= 0; --i)
            {
                e.proxied_expertise[i + depth] += delta[i];
            }
        });

        adjust_proxied_expertise(proxy, delta, depth + 1);
    }
}

void dbs_expert_token::adjust_proxied_expertise(const expert_token_object& expert_token,
                                                share_type delta,
                                                int depth)
{
    dbs_expertise_allocation_proposal& expertise_allocation_proposal_service = db_impl().obtain_service<dbs_expertise_allocation_proposal>();

    if (expert_token.proxy != DEIP_PROXY_TO_SELF_EXPERT_TOKEN)
    {
        /// nested proxies are not supported, vote will not propagate
        if (depth >= DEIP_MAX_PROXY_RECURSION_DEPTH)
            return;

        const auto& proxy = get_expert_token_by_account_and_discipline(expert_token.proxy, expert_token.discipline_id);

        db_impl().modify(proxy, [&](expert_token_object& e) { e.proxied_expertise[depth] += delta; });

        adjust_proxied_expertise(proxy, delta, depth + 1);
    }
    else
    {
        expertise_allocation_proposal_service.adjust_expert_token_vote(expert_token, delta);
    }
}

} //namespace chain
} //namespace deip