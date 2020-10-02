#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/services/dbs_witness.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_dynamic_global_properties.hpp>

namespace deip {
namespace chain {

dbs_account::dbs_account(database& db)
    : _base_type(db)
{
}

const account_object& dbs_account::get_account(const account_name_type& name) const
{
    try
    {
        return db_impl().get<account_object, by_name>(name);
    }
    FC_CAPTURE_AND_RETHROW((name))
}

const dbs_account::account_optional_ref_type dbs_account::get_account_if_exists(const account_name_type& name) const
{
    account_optional_ref_type result;
    const auto& idx = db_impl()
      .get_index<account_index>()
      .indicies()
      .get<by_name>();

    auto itr = idx.find(name);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const account_authority_object& dbs_account::get_account_authority(const account_name_type& name) const
{
    try
    {
        return db_impl().get<account_authority_object, by_account>(name);
    }
    FC_CAPTURE_AND_RETHROW((name))
}

const bool dbs_account::account_exists(const account_name_type& name) const
{
    const auto& idx = db_impl()
      .get_index<account_index>()
      .indices()
      .get<by_name>();

    auto itr = idx.find(name);
    return itr != idx.end();
}

void dbs_account::check_account_existence(const account_name_type& name,
                                          const optional<const char*>& context_type_name) const
{
    auto acc = db_impl().find<account_object, by_name>(name);
    if (context_type_name.valid())
    {
        FC_ASSERT(acc != nullptr, "\"${1}\" \"${2}\" must exist.", ("1", *context_type_name)("2", name));
    }
    else
    {
        FC_ASSERT(acc != nullptr, "Account \"${1}\" must exist.", ("1", name));
    }
}

void dbs_account::check_account_existence(const authority::account_authority_map& names,
                                          const optional<const char*>& context_type_name) const
{
    for (const auto& a : names)
    {
        check_account_existence(a.first, context_type_name);
    }
}

const account_object& dbs_account::create_account_by_faucets(const account_name_type& account_name,
                                                             const account_name_type& creator_name,
                                                             const public_key_type& memo_key,
                                                             const fc::optional<string>& json_metadata,
                                                             const authority& owner,
                                                             const authority& active,
                                                             const flat_map<uint16_t, authority>& active_overrides,
                                                             const asset& fee,
                                                             const flat_set<account_trait>& traits,
                                                             const bool& is_user_account)
{
    auto& research_groups_service = db_impl().obtain_service<dbs_research_group>();
    auto& account_balance_service = db_impl().obtain_service<dbs_account_balance>();
    auto& dgp_service = db_impl().obtain_service<dbs_dynamic_global_properties>();

    FC_ASSERT(fee >= DEIP_MIN_ACCOUNT_CREATION_FEE, 
      "Insufficient fee ${1} for account creation. Min fee is ${1}", 
      ("1", fee)("2", DEIP_MIN_ACCOUNT_CREATION_FEE)
    );

    const auto& props = db_impl().get_dynamic_global_properties();
    account_balance_service.adjust_balance(creator_name, -fee);

    const auto& new_account = db_impl().create<account_object>([&](account_object& acc) {
        acc.name = account_name;
        acc.memo_key = memo_key;
        acc.created = props.time;
        acc.mined = false;
        acc.recovery_account = creator_name;
        acc.is_research_group = !is_user_account;
#ifndef IS_LOW_MEM
        if (json_metadata.valid())
        {
            const auto& metadata = *json_metadata;
            fc::from_string(acc.json_metadata, metadata);
        }
#endif
    });

    if (!account_balance_service.exists_by_owner_and_asset(account_name, DEIP_SYMBOL)) // check for genesis
        account_balance_service.create(account_name, DEIP_SYMBOL, 0);
    
    // Convert fee to Common tokens and increase account common tokens balance for throughput
    increase_common_tokens(get_account(account_name), fee.amount);

    db_impl().create<account_authority_object>([&](account_authority_object& auth) {
        auth.account = account_name;
        auth.owner = owner;
        auth.active = active;
        auth.last_owner_update = fc::time_point_sec::min();

        for (const auto& override : active_overrides)
        {
            auth.active_overrides.insert(std::pair<uint16_t, shared_authority>(override.first, shared_authority(override.second, shared_authority::allocator_type(db_impl().get_segment_manager()))));
        }
    });

    if (is_user_account) // user personal workspace
    {
        const auto& personal_rg = research_groups_service.create_personal_research_group(
          account_name
        );
        research_groups_service.add_member_to_research_group(
          account_name,
          personal_rg.id, 
          DEIP_100_PERCENT, 
          account_name_type()
        );
    }
    else // research group workspace
    {
        const account_trait trait = *traits.begin();
        const auto rg_trait = trait.get<research_group_trait>();

        const auto& shared_rg = research_groups_service.create_research_group(
          account_name,
          creator_name,
          rg_trait.name, 
          rg_trait.description
        );
        
        research_groups_service.add_member_to_research_group(
          creator_name,
          shared_rg.id,
          DEIP_100_PERCENT,
          account_name_type()
        );
    }

    dgp_service.create_recent_entity(account_name);

    return new_account;
}

void dbs_account::update_acount(const account_object& account,
                                const public_key_type& memo_key,
                                const string& json_metadata,
                                const optional<flat_set<deip::protocol::account_trait>>& traits,
                                const optional<time_point_sec>& now)
{
    dbs_research_group& research_groups_service = db_impl().obtain_service<dbs_research_group>();

    _time t = _get_now(now);

    db_impl().modify(account, [&](account_object& acc) {
        if (memo_key != public_key_type())
            acc.memo_key = memo_key;

            acc.last_account_update = t;

#ifndef IS_LOW_MEM
        if (json_metadata.size() > 0)
            fc::from_string(acc.json_metadata, json_metadata);
#endif
    });

    if (account.is_research_group && traits.valid() && traits->size() == 1) // research group workspace
    {
        const account_trait trait = *traits->begin();
        const auto rg_trait = trait.get<research_group_trait>();
        const auto& research_group = research_groups_service.get_research_group_by_account(account.name);

        FC_ASSERT(!research_group.is_personal, "Personal research group is preserved and can not be edited");

        research_groups_service.update_research_group(
          research_group,
          rg_trait.name, 
          rg_trait.description
        );
    }
}

void dbs_account::update_owner_authority(const account_object& account,
                                         const authority& owner_authority,
                                         const optional<time_point_sec>& now)
{
    _time t = _get_now(now);

    if (db_impl().head_block_num() >= DEIP_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM)
    {
        db_impl().create<owner_authority_history_object>([&](owner_authority_history_object& hist) {
            hist.account = account.name;
            hist.previous_owner_authority = db_impl().get<account_authority_object, by_account>(account.name).owner;
            hist.last_valid_time = t;
        });
    }

    db_impl().modify(db_impl().get<account_authority_object, by_account>(account.name),
        [&](account_authority_object& auth) {
            auth.owner = owner_authority;
            auth.last_owner_update = t;
        });
}


void dbs_account::update_active_authority(const account_object& account,
                                          const authority& active_authority)
{
    db_impl().modify(db_impl().get<account_authority_object, by_account>(account.name),
        [&](account_authority_object& auth) {
            auth.active = active_authority;
        });
}

void dbs_account::add_to_active_authority(const account_object& account,
                                          const account_name_type& member,
                                          const weight_type& weight)
{
    db_impl().modify(db_impl().get<account_authority_object, by_account>(account.name),
        [&](account_authority_object& auth) {
            if (auth.active.account_auths.find(member) == auth.active.account_auths.end()) {
                auth.active.account_auths.insert(std::make_pair(member, weight_type(weight)));
            }
        });
}

void dbs_account::remove_from_active_authority(const account_object& account,
                                               const account_name_type& member)
{
    db_impl().modify(db_impl().get<account_authority_object, by_account>(account.name),
        [&](account_authority_object& auth) {
            if (auth.active.account_auths.find(member) != auth.active.account_auths.end()) {
                auth.active.account_auths.erase(auth.active.account_auths.find(member));
            }
        });
}

void dbs_account::update_active_overrides_authorities(const account_object& account,
                                                      const flat_map<uint16_t, optional<authority>>& auth_overrides)
{
    db_impl().modify(db_impl().get<account_authority_object, by_account>(account.name),
        [&](account_authority_object& auth) {
            for (const auto& active_override : auth_overrides)
            {
                auto itr = auth.active_overrides.find(active_override.first);
                if (itr != auth.active_overrides.end())
                {
                    auth.active_overrides.erase(itr);
                }

                if (active_override.second.valid())
                {
                    const auto& auth_override = *active_override.second;
                    auth.active_overrides.insert(
                      std::pair<uint16_t, shared_authority>(
                        active_override.first,
                        shared_authority(auth_override, shared_authority::allocator_type(db_impl().get_segment_manager()))
                      )
                    );
                }
            }
        });
}


void dbs_account::update_withdraw(const account_object& account,
                                  const share_type& common_tokens_withdraw_rate,
                                  const time_point_sec& next_common_tokens_withdrawal,
                                  const share_type& to_withdrawn,
                                  const optional<share_type>& withdrawn)
{
    db_impl().modify(account, [&](account_object& a) {
        a.common_tokens_withdraw_rate = common_tokens_withdraw_rate;
        a.next_common_tokens_withdrawal = next_common_tokens_withdrawal;
        a.to_withdraw = to_withdrawn;
        a.withdrawn = (withdrawn.valid()) ? (*withdrawn) : 0;
    });
}

void dbs_account::increase_withdraw_routes(const account_object& account)
{
    db_impl().modify(account, [&](account_object& a) { a.withdraw_routes++; });
}

void dbs_account::decrease_withdraw_routes(const account_object& account)
{
    db_impl().modify(account, [&](account_object& a) { a.withdraw_routes--; });
}

void dbs_account::increase_witnesses_voted_for(const account_object& account)
{
    db_impl().modify(account, [&](account_object& a) { a.witnesses_voted_for++; });
}

void dbs_account::decrease_witnesses_voted_for(const account_object& account)
{
    db_impl().modify(account, [&](account_object& a) { a.witnesses_voted_for--; });
}

void dbs_account::create_account_recovery(const account_name_type& account_to_recover,
                                          const authority& new_owner_authority,
                                          const optional<time_point_sec>& now)
{
    _time t = _get_now(now);

    const auto& recovery_request_idx
        = db_impl().get_index<account_recovery_request_index>().indices().get<by_account>();
    auto request = recovery_request_idx.find(account_to_recover);

    if (request == recovery_request_idx.end()) // New Request
    {
        FC_ASSERT(!new_owner_authority.is_impossible(), "Cannot recover using an impossible authority.");
        FC_ASSERT(new_owner_authority.weight_threshold, "Cannot recover using an open authority.");

        check_account_existence(new_owner_authority.account_auths);

        db_impl().create<account_recovery_request_object>([&](account_recovery_request_object& req) {
            req.account_to_recover = account_to_recover;
            req.new_owner_authority = new_owner_authority;
            req.expires = t + DEIP_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
        });
    }
    else if (new_owner_authority.weight_threshold == 0) // Cancel Request if authority is open
    {
        db_impl().remove(*request);
    }
    else // Change Request
    {
        FC_ASSERT(!new_owner_authority.is_impossible(), "Cannot recover using an impossible authority.");

        check_account_existence(new_owner_authority.account_auths);

        db_impl().modify(*request, [&](account_recovery_request_object& req) {
            req.new_owner_authority = new_owner_authority;
            req.expires = t + DEIP_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD;
        });
    }
}

void dbs_account::submit_account_recovery(const account_object& account_to_recover,
                                          const authority& new_owner_authority,
                                          const authority& recent_owner_authority,
                                          const optional<time_point_sec>& now)
{
    _time t = _get_now(now);

    const auto& recovery_request_idx
        = db_impl().get_index<account_recovery_request_index>().indices().get<by_account>();
    auto request = recovery_request_idx.find(account_to_recover.name);

    FC_ASSERT(request != recovery_request_idx.end(), "There are no active recovery requests for this account.");
    FC_ASSERT(request->new_owner_authority == new_owner_authority,
              "New owner authority does not match recovery request.");

    const auto& recent_auth_idx = db_impl().get_index<owner_authority_history_index>().indices().get<by_account>();
    auto hist = recent_auth_idx.lower_bound(account_to_recover.name);
    bool found = false;

    while (hist != recent_auth_idx.end() && hist->account == account_to_recover.name && !found)
    {
        found = hist->previous_owner_authority == recent_owner_authority;
        if (found)
            break;
        ++hist;
    }

    FC_ASSERT(found, "Recent authority not found in authority history.");

    db_impl().remove(*request); // Remove first, update_owner_authority may invalidate iterator
    update_owner_authority(account_to_recover, new_owner_authority);
    db_impl().modify(account_to_recover, [&](account_object& a) { a.last_account_recovery = t; });
}

void dbs_account::change_recovery_account(const account_object& account_to_recover,
                                          const account_name_type& new_recovery_account_name,
                                          const optional<time_point_sec>& now)
{
    _time t = _get_now(now);

    const auto& change_recovery_idx
        = db_impl().get_index<change_recovery_account_request_index>().indices().get<by_account>();
    auto request = change_recovery_idx.find(account_to_recover.name);

    if (request == change_recovery_idx.end()) // New request
    {
        db_impl().create<change_recovery_account_request_object>([&](change_recovery_account_request_object& req) {
            req.account_to_recover = account_to_recover.name;
            req.recovery_account = new_recovery_account_name;
            req.effective_on = t + DEIP_OWNER_AUTH_RECOVERY_PERIOD;
        });
    }
    else if (account_to_recover.recovery_account != new_recovery_account_name) // Change existing request
    {
        db_impl().modify(*request, [&](change_recovery_account_request_object& req) {
            req.recovery_account = new_recovery_account_name;
            req.effective_on = t + DEIP_OWNER_AUTH_RECOVERY_PERIOD;
        });
    }
    else // Request exists and changing back to current recovery account
    {
        db_impl().remove(*request);
    }
}

void dbs_account::update_voting_proxy(const account_object& account, const optional<account_object>& proxy_account)
{
    /// remove all current votes
    std::array<share_type, DEIP_MAX_PROXY_RECURSION_DEPTH + 1> delta;

    delta[0] = -account.common_tokens_balance;

    for (int i = 0; i < DEIP_MAX_PROXY_RECURSION_DEPTH; ++i)
        delta[i + 1] = -account.proxied_vsf_votes[i];

    adjust_proxied_witness_votes(account, delta);

    if (proxy_account.valid())
    {
        flat_set<account_id_type> proxy_chain({ account.id, (*proxy_account).id });
        proxy_chain.reserve(DEIP_MAX_PROXY_RECURSION_DEPTH + 1);

        /// check for proxy loops and fail to update the proxy if it would create a loop
        auto cprox = &(*proxy_account);
        while (cprox->proxy.size() != 0)
        {
            const auto next_proxy = get_account(cprox->proxy);
            FC_ASSERT(proxy_chain.insert(next_proxy.id).second, "This proxy would create a proxy loop.");
            cprox = &next_proxy;
            FC_ASSERT(proxy_chain.size() <= DEIP_MAX_PROXY_RECURSION_DEPTH, "Proxy chain is too long.");
        }

        /// clear all individual vote records
        clear_witness_votes(account);

        db_impl().modify(account, [&](account_object& a) { a.proxy = (*proxy_account).name; });

        /// add all new votes
        for (int i = 0; i <= DEIP_MAX_PROXY_RECURSION_DEPTH; ++i)
            delta[i] = -delta[i];
        adjust_proxied_witness_votes(account, delta);
    }
    else
    { /// we are clearing the proxy which means we simply update the account
        db_impl().modify(account,
                         [&](account_object& a) { a.proxy = account_name_type(DEIP_PROXY_TO_SELF_ACCOUNT); });
    }
}

void dbs_account::clear_witness_votes(const account_object& account)
{
    const auto& vidx = db_impl().get_index<witness_vote_index>().indices().get<by_account_witness>();
    auto itr = vidx.lower_bound(boost::make_tuple(account.id, witness_id_type()));
    while (itr != vidx.end() && itr->account == account.id)
    {
        const auto& current = *itr;
        ++itr;
        db_impl().remove(current);
    }

    db_impl().modify(account, [&](account_object& acc) { acc.witnesses_voted_for = 0; });
}

void dbs_account::adjust_proxied_witness_votes(
    const account_object& account, const std::array<share_type, DEIP_MAX_PROXY_RECURSION_DEPTH + 1>& delta, int depth)
{
    dbs_witness& witness_service = db().obtain_service<dbs_witness>();

    if (account.proxy != DEIP_PROXY_TO_SELF_ACCOUNT)
    {
        /// nested proxies are not supported, vote will not propagate
        if (depth >= DEIP_MAX_PROXY_RECURSION_DEPTH)
            return;

        const auto& proxy = get_account(account.proxy);

        db_impl().modify(proxy, [&](account_object& a) {
            for (int i = DEIP_MAX_PROXY_RECURSION_DEPTH - depth - 1; i >= 0; --i)
            {
                a.proxied_vsf_votes[i + depth] += delta[i];
            }
        });

        adjust_proxied_witness_votes(proxy, delta, depth + 1);
    }
    else
    {
        share_type total_delta = 0;
        for (int i = DEIP_MAX_PROXY_RECURSION_DEPTH - depth; i >= 0; --i)
            total_delta += delta[i];
        witness_service.adjust_witness_votes(account, total_delta);
    }
}

void dbs_account::adjust_proxied_witness_votes(const account_object& account, share_type delta, int depth)
{
    dbs_witness& witness_service = db().obtain_service<dbs_witness>();

    if (account.proxy != DEIP_PROXY_TO_SELF_ACCOUNT)
    {
        /// nested proxies are not supported, vote will not propagate
        if (depth >= DEIP_MAX_PROXY_RECURSION_DEPTH)
            return;

        const auto& proxy = get_account(account.proxy);

        db_impl().modify(proxy, [&](account_object& a) { a.proxied_vsf_votes[depth] += delta; });

        adjust_proxied_witness_votes(proxy, delta, depth + 1);
    }
    else
    {
        witness_service.adjust_witness_votes(account, delta);
    }
}

const account_object& dbs_account::get_account(const account_id_type& account_id) const
{
    try {
        return db_impl().get<account_object, by_id>(account_id);
    }
    FC_CAPTURE_AND_RETHROW((account_id))
}

void dbs_account::increase_common_tokens(const account_object &account, const share_type &amount)
{
    FC_ASSERT(amount >= 0, "Amount cannot be < 0");

    auto& props = db_impl().get_dynamic_global_properties();
    db_impl().modify(account, [&](account_object& a) { a.common_tokens_balance += amount; });
    db_impl().modify(props, [&](dynamic_global_property_object& gpo) {
        gpo.total_common_tokens_amount += amount;
        gpo.common_tokens_fund += asset(amount, DEIP_SYMBOL);
    });
}

void dbs_account::decrease_common_tokens(const account_object &account, const share_type &amount)
{
    FC_ASSERT(amount >= 0, "Amount cannot be < 0");
    auto& props = db_impl().get_dynamic_global_properties();
    db_impl().modify(account, [&](account_object& a) { a.common_tokens_balance -= amount; });
    db_impl().modify(props, [&](dynamic_global_property_object& gpo) {
        gpo.total_common_tokens_amount -= amount;
        gpo.common_tokens_fund -= asset(amount, DEIP_SYMBOL);
    });
}

void dbs_account::adjust_expertise_tokens_throughput(const account_object& account, const share_type& delta)
{
    auto& props = db_impl().get_dynamic_global_properties();
    db_impl().modify(account, [&](account_object& a) {
        a.expertise_tokens_balance += delta;
        if (a.expertise_tokens_balance < 0)
        {
            a.expertise_tokens_balance = 0;
        }
    });
    db_impl().modify(props, [&](dynamic_global_property_object& gpo) {
        gpo.total_expert_tokens_amount += delta;
        if (gpo.total_expert_tokens_amount < 0)
        {
            gpo.total_expert_tokens_amount = 0;
        }
    });
}

dbs_account::accounts_refs_type dbs_account::get_accounts_by_expert_discipline(const discipline_id_type& discipline_id) const
{
    FC_ASSERT(discipline_id > 0, "Cannot use root discipline.");

    dbs_expert_token& expert_token_service = db_impl().obtain_service<dbs_expert_token>();

    accounts_refs_type ret;
    auto expert_tokens = expert_token_service.get_expert_tokens_by_discipline(discipline_id);

    for (auto expert_token : expert_tokens)
    {
        auto &token = expert_token.get();
        ret.push_back(std::cref(get_account(token.account_name)));
    }

    return ret;
}

void dbs_account::process_account_recovery()
{
    // Clear expired recovery requests
    const auto& rec_req_idx = db_impl().get_index<account_recovery_request_index>().indices().get<by_expiration>();
    auto rec_req = rec_req_idx.begin();

    while (rec_req != rec_req_idx.end() && rec_req->expires <= db_impl().head_block_time())
    {
        db_impl().remove(*rec_req);
        rec_req = rec_req_idx.begin();
    }

    // Clear invalid historical authorities
    const auto& hist_idx = db_impl().get_index<owner_authority_history_index>().indices(); // by id
    auto hist = hist_idx.begin();

    while (hist != hist_idx.end()
           && time_point_sec(hist->last_valid_time + DEIP_OWNER_AUTH_RECOVERY_PERIOD) < db_impl().head_block_time())
    {
        db_impl().remove(*hist);
        hist = hist_idx.begin();
    }

    // Apply effective recovery_account changes
    const auto& change_req_idx = db_impl().get_index<change_recovery_account_request_index>().indices().get<by_effective_date>();
    auto change_req = change_req_idx.begin();

    while (change_req != change_req_idx.end() && change_req->effective_on <= db_impl().head_block_time())
    {
        db_impl().modify(get_account(change_req->account_to_recover),
               [&](account_object& a) { a.recovery_account = change_req->recovery_account; });

        db_impl().remove(*change_req);
        change_req = change_req_idx.begin();
    }
}

const dbs_account::accounts_refs_type dbs_account::lookup_accounts(const string& lower_bound_name, uint32_t limit) const
{
    accounts_refs_type results;

    const auto& accounts_by_name = db_impl().get_index<account_index>().indices().get<by_name>();

    for (auto itr = accounts_by_name.lower_bound(lower_bound_name); limit-- && itr != accounts_by_name.end(); ++itr)
    {
        results.push_back(std::cref(*itr));
    }

    return results;
}

const dbs_account::accounts_refs_type dbs_account::lookup_user_accounts(const string& lower_bound_name, uint32_t limit) const
{
    accounts_refs_type results;

    const auto& accounts_by_name = db_impl().get_index<account_index>().indices().get<by_name>();

    for (auto itr = accounts_by_name.lower_bound(lower_bound_name); limit-- && itr != accounts_by_name.end(); ++itr)
    {
        if (!itr->is_research_group)
        {
            results.push_back(std::cref(*itr));
        }
    }

    return results;
}


const dbs_account::accounts_refs_type dbs_account::lookup_research_group_accounts(const string& lower_bound_name, uint32_t limit) const
{
    accounts_refs_type results;

    const auto& accounts_by_name = db_impl().get_index<account_index>().indices().get<by_name>();

    for (auto itr = accounts_by_name.lower_bound(lower_bound_name); limit-- && itr != accounts_by_name.end(); ++itr)
    {
        if (itr->is_research_group)
        {
            results.push_back(std::cref(*itr));
        }
    }

    return results;
}

}
}
