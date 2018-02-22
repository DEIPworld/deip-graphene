#include <deip/app/api_context.hpp>
#include <deip/app/application.hpp>
#include <deip/app/database_api.hpp>

#include <deip/protocol/get_config.hpp>

#include <deip/chain/util/reward.hpp>

#include <fc/bloom_filter.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/crypto/hex.hpp>

#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string.hpp>

#include <cctype>

#include <cfenv>
#include <iostream>

#include <deip/chain/dbs_budget.hpp>
#include <deip/chain/dbs_discipline.hpp>
#include <deip/chain/dbs_research.hpp>
#include <deip/chain/dbs_research_content.hpp>
#include <deip/chain/dbs_expert_token.hpp>
#include <deip/chain/dbs_research_token_sale.hpp>
#include <deip/chain/dbs_research_group.hpp>

#define GET_REQUIRED_FEES_MAX_RECURSION 4

namespace deip {
namespace app {

class database_api_impl;

class database_api_impl : public std::enable_shared_from_this<database_api_impl>
{
public:
    database_api_impl(const deip::app::api_context& ctx);
    ~database_api_impl();

    // Subscriptions
    void set_block_applied_callback(std::function<void(const variant& block_id)> cb);

    // Blocks and transactions
    optional<block_header> get_block_header(uint32_t block_num) const;
    optional<signed_block_api_obj> get_block(uint32_t block_num) const;
    vector<applied_operation> get_ops_in_block(uint32_t block_num, bool only_virtual) const;

    // Globals
    fc::variant_object get_config() const;
    dynamic_global_property_api_obj get_dynamic_global_properties() const;
    chain_id_type get_chain_id() const;

    // Keys
    vector<set<string>> get_key_references(vector<public_key_type> key) const;

    // Accounts
    vector<extended_account> get_accounts(const vector<string>& names) const;
    vector<account_id_type> get_account_references(account_id_type account_id) const;
    vector<optional<account_api_obj>> lookup_account_names(const vector<string>& account_names) const;
    set<string> lookup_accounts(const string& lower_bound_name, uint32_t limit) const;
    uint64_t get_account_count() const;

    // Budgets
    vector<budget_api_obj> get_budgets(const set<string>& names) const;
    set<string> lookup_budget_owners(const string& lower_bound_name, uint32_t limit) const;

    // Witnesses
    vector<optional<witness_api_obj>> get_witnesses(const vector<witness_id_type>& witness_ids) const;
    fc::optional<witness_api_obj> get_witness_by_account(const string& account_name) const;
    set<account_name_type> lookup_witness_accounts(const string& lower_bound_name, uint32_t limit) const;
    uint64_t get_witness_count() const;

    // Authority / validation
    std::string get_transaction_hex(const signed_transaction& trx) const;
    set<public_key_type> get_required_signatures(const signed_transaction& trx,
                                                 const flat_set<public_key_type>& available_keys) const;
    set<public_key_type> get_potential_signatures(const signed_transaction& trx) const;
    bool verify_authority(const signed_transaction& trx) const;
    bool verify_account_authority(const string& name_or_id, const flat_set<public_key_type>& signers) const;

    // signal handlers
    void on_applied_block(const chain::signed_block& b);

    std::function<void(const fc::variant&)> _block_applied_callback;

    deip::chain::database& _db;

    boost::signals2::scoped_connection _block_applied_connection;

    bool _disable_get_block = false;
};

applied_operation::applied_operation()
{
}

applied_operation::applied_operation(const operation_object& op_obj)
    : trx_id(op_obj.trx_id)
    , block(op_obj.block)
    , trx_in_block(op_obj.trx_in_block)
    , op_in_trx(op_obj.op_in_trx)
    , virtual_op(op_obj.virtual_op)
    , timestamp(op_obj.timestamp)
{
    // fc::raw::unpack( op_obj.serialized_op, op );     // g++ refuses to compile this as ambiguous
    op = fc::raw::unpack<operation>(op_obj.serialized_op);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Subscriptions                                                    //
//                                                                  //
//////////////////////////////////////////////////////////////////////

void database_api::set_block_applied_callback(std::function<void(const variant& block_id)> cb)
{
    my->_db.with_read_lock([&]() { my->set_block_applied_callback(cb); });
}

void database_api_impl::on_applied_block(const chain::signed_block& b)
{
    try
    {
        _block_applied_callback(fc::variant(signed_block_header(b)));
    }
    catch (...)
    {
        _block_applied_connection.release();
    }
}

void database_api_impl::set_block_applied_callback(std::function<void(const variant& block_header)> cb)
{
    _block_applied_callback = cb;
    _block_applied_connection = connect_signal(_db.applied_block, *this, &database_api_impl::on_applied_block);
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Constructors                                                     //
//                                                                  //
//////////////////////////////////////////////////////////////////////

database_api::database_api(const deip::app::api_context& ctx)
    : my(new database_api_impl(ctx))
{
}

database_api::~database_api()
{
}

database_api_impl::database_api_impl(const deip::app::api_context& ctx)
    : _db(*ctx.app.chain_database())
{
    wlog("creating database api ${x}", ("x", int64_t(this)));

    _disable_get_block = ctx.app._disable_get_block;
}

database_api_impl::~database_api_impl()
{
    elog("freeing database api ${x}", ("x", int64_t(this)));
}

void database_api::on_api_startup()
{
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Blocks and transactions                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////

optional<block_header> database_api::get_block_header(uint32_t block_num) const
{
    FC_ASSERT(!my->_disable_get_block, "get_block_header is disabled on this node.");

    return my->_db.with_read_lock([&]() { return my->get_block_header(block_num); });
}

optional<block_header> database_api_impl::get_block_header(uint32_t block_num) const
{
    auto result = _db.fetch_block_by_number(block_num);
    if (result)
        return *result;
    return {};
}

optional<signed_block_api_obj> database_api::get_block(uint32_t block_num) const
{
    FC_ASSERT(!my->_disable_get_block, "get_block is disabled on this node.");

    return my->_db.with_read_lock([&]() { return my->get_block(block_num); });
}

optional<signed_block_api_obj> database_api_impl::get_block(uint32_t block_num) const
{
    return _db.fetch_block_by_number(block_num);
}

vector<applied_operation> database_api::get_ops_in_block(uint32_t block_num, bool only_virtual) const
{
    return my->_db.with_read_lock([&]() { return my->get_ops_in_block(block_num, only_virtual); });
}

vector<applied_operation> database_api_impl::get_ops_in_block(uint32_t block_num, bool only_virtual) const
{
    const auto& idx = _db.get_index<operation_index>().indices().get<by_location>();
    auto itr = idx.lower_bound(block_num);
    vector<applied_operation> result;
    applied_operation temp;
    while (itr != idx.end() && itr->block == block_num)
    {
        temp = *itr;
        if (!only_virtual || is_virtual_operation(temp.op))
            result.push_back(temp);
        ++itr;
    }
    return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Globals                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////=

fc::variant_object database_api::get_config() const
{
    return my->_db.with_read_lock([&]() { return my->get_config(); });
}

fc::variant_object database_api_impl::get_config() const
{
    return deip::protocol::get_config();
}

dynamic_global_property_api_obj database_api::get_dynamic_global_properties() const
{
    return my->_db.with_read_lock([&]() { return my->get_dynamic_global_properties(); });
}

chain_properties database_api::get_chain_properties() const
{
    return my->_db.with_read_lock([&]() { return my->_db.get_witness_schedule_object().median_props; });
}

dynamic_global_property_api_obj database_api_impl::get_dynamic_global_properties() const
{
    return dynamic_global_property_api_obj(_db.get(dynamic_global_property_id_type()), _db);
}

chain_id_type database_api_impl::get_chain_id() const
{
    return _db.get_chain_id();
}

witness_schedule_api_obj database_api::get_witness_schedule() const
{
    return my->_db.with_read_lock([&]() { return my->_db.get(witness_schedule_id_type()); });
}

hardfork_version database_api::get_hardfork_version() const
{
    return my->_db.with_read_lock([&]() { return my->_db.get(hardfork_property_id_type()).current_hardfork_version; });
}

scheduled_hardfork database_api::get_next_scheduled_hardfork() const
{
    return my->_db.with_read_lock([&]() {
        scheduled_hardfork shf;
        const auto& hpo = my->_db.get(hardfork_property_id_type());
        shf.hf_version = hpo.next_hardfork;
        shf.live_time = hpo.next_hardfork_time;
        return shf;
    });
}

reward_fund_api_obj database_api::get_reward_fund(const string& name) const
{
    return my->_db.with_read_lock([&]() {
        auto fund = my->_db.find<reward_fund_object, by_name>(name);
        FC_ASSERT(fund != nullptr, "Invalid reward fund name");

        return *fund;
    });
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Keys                                                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<set<string>> database_api::get_key_references(vector<public_key_type> key) const
{
    return my->_db.with_read_lock([&]() { return my->get_key_references(key); });
}

/**
 *  @return all accounts that referr to the key or account id in their owner or active authorities.
 */
vector<set<string>> database_api_impl::get_key_references(vector<public_key_type> keys) const
{
    FC_ASSERT(false,
              "database_api::get_key_references has been deprecated. Please use "
              "account_by_key_api::get_key_references instead.");
    vector<set<string>> final_result;
    return final_result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Accounts                                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<extended_account> database_api::get_accounts(const vector<string>& names) const
{
    return my->_db.with_read_lock([&]() { return my->get_accounts(names); });
}

vector<extended_account> database_api_impl::get_accounts(const vector<string>& names) const
{
    const auto& idx = _db.get_index<account_index>().indices().get<by_name>();
    const auto& vidx = _db.get_index<witness_vote_index>().indices().get<by_account_witness>();
    vector<extended_account> results;

    for (auto name : names)
    {
        auto itr = idx.find(name);
        if (itr != idx.end())
        {
            results.push_back(extended_account(*itr, _db));

            auto vitr = vidx.lower_bound(boost::make_tuple(itr->id, witness_id_type()));
            while (vitr != vidx.end() && vitr->account == itr->id)
            {
                results.back().witness_votes.insert(_db.get(vitr->witness).owner);
                ++vitr;
            }
        }
    }

    return results;
}

vector<account_id_type> database_api::get_account_references(account_id_type account_id) const
{
    return my->_db.with_read_lock([&]() { return my->get_account_references(account_id); });
}

vector<account_id_type> database_api_impl::get_account_references(account_id_type account_id) const
{
    /*const auto& idx = _db.get_index<account_index>();
    const auto& aidx = dynamic_cast<const primary_index<account_index>&>(idx);
    const auto& refs = aidx.get_secondary_index<deip::chain::account_member_index>();
    auto itr = refs.account_to_account_memberships.find(account_id);
    vector<account_id_type> result;

    if( itr != refs.account_to_account_memberships.end() )
    {
       result.reserve( itr->second.size() );
       for( auto item : itr->second ) result.push_back(item);
    }
    return result;*/
    FC_ASSERT(false, "database_api::get_account_references --- Needs to be refactored for deip.");
}

vector<optional<account_api_obj>> database_api::lookup_account_names(const vector<string>& account_names) const
{
    return my->_db.with_read_lock([&]() { return my->lookup_account_names(account_names); });
}

vector<optional<account_api_obj>> database_api_impl::lookup_account_names(const vector<string>& account_names) const
{
    vector<optional<account_api_obj>> result;
    result.reserve(account_names.size());

    for (auto& name : account_names)
    {
        auto itr = _db.find<account_object, by_name>(name);

        if (itr)
        {
            result.push_back(account_api_obj(*itr, _db));
        }
        else
        {
            result.push_back(optional<account_api_obj>());
        }
    }

    return result;
}

set<string> database_api::lookup_accounts(const string& lower_bound_name, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() { return my->lookup_accounts(lower_bound_name, limit); });
}

set<string> database_api_impl::lookup_accounts(const string& lower_bound_name, uint32_t limit) const
{
    FC_ASSERT(limit <= 1000);
    const auto& accounts_by_name = _db.get_index<account_index>().indices().get<by_name>();
    set<string> result;

    for (auto itr = accounts_by_name.lower_bound(lower_bound_name); limit-- && itr != accounts_by_name.end(); ++itr)
    {
        result.insert(itr->name);
    }

    return result;
}

uint64_t database_api::get_account_count() const
{
    return my->_db.with_read_lock([&]() { return my->get_account_count(); });
}

uint64_t database_api_impl::get_account_count() const
{
    return _db.get_index<account_index>().indices().size();
}

vector<owner_authority_history_api_obj> database_api::get_owner_history(string account) const
{
    return my->_db.with_read_lock([&]() {
        vector<owner_authority_history_api_obj> results;

        const auto& hist_idx = my->_db.get_index<owner_authority_history_index>().indices().get<by_account>();
        auto itr = hist_idx.lower_bound(account);

        while (itr != hist_idx.end() && itr->account == account)
        {
            results.push_back(owner_authority_history_api_obj(*itr));
            ++itr;
        }

        return results;
    });
}

optional<account_recovery_request_api_obj> database_api::get_recovery_request(string account) const
{
    return my->_db.with_read_lock([&]() {
        optional<account_recovery_request_api_obj> result;

        const auto& rec_idx = my->_db.get_index<account_recovery_request_index>().indices().get<by_account>();
        auto req = rec_idx.find(account);

        if (req != rec_idx.end())
            result = account_recovery_request_api_obj(*req);

        return result;
    });
}

optional<escrow_api_obj> database_api::get_escrow(string from, uint32_t escrow_id) const
{
    return my->_db.with_read_lock([&]() {
        optional<escrow_api_obj> result;

        try
        {
            result = my->_db.get_escrow(from, escrow_id);
        }
        catch (...)
        {
        }

        return result;
    });
}

vector<withdraw_route> database_api::get_withdraw_routes(string account, withdraw_route_type type) const
{
    return my->_db.with_read_lock([&]() {
        vector<withdraw_route> result;

        const auto& acc = my->_db.get_account(account);

        if (type == outgoing || type == all)
        {
            const auto& by_route = my->_db.get_index<withdraw_vesting_route_index>().indices().get<by_withdraw_route>();
            auto route = by_route.lower_bound(acc.id);

            while (route != by_route.end() && route->from_account == acc.id)
            {
                withdraw_route r;
                r.from_account = account;
                r.to_account = my->_db.get(route->to_account).name;
                r.percent = route->percent;
                r.auto_vest = route->auto_vest;

                result.push_back(r);

                ++route;
            }
        }

        if (type == incoming || type == all)
        {
            const auto& by_dest = my->_db.get_index<withdraw_vesting_route_index>().indices().get<by_destination>();
            auto route = by_dest.lower_bound(acc.id);

            while (route != by_dest.end() && route->to_account == acc.id)
            {
                withdraw_route r;
                r.from_account = my->_db.get(route->from_account).name;
                r.to_account = account;
                r.percent = route->percent;
                r.auto_vest = route->auto_vest;

                result.push_back(r);

                ++route;
            }
        }

        return result;
    });
}

optional<account_bandwidth_api_obj> database_api::get_account_bandwidth(string account,
                                                                        witness::bandwidth_type type) const
{
    optional<account_bandwidth_api_obj> result;

    if (my->_db.has_index<witness::account_bandwidth_index>())
    {
        auto band = my->_db.find<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
            boost::make_tuple(account, type));
        if (band != nullptr)
            result = *band;
    }

    return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Witnesses                                                        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

vector<optional<witness_api_obj>> database_api::get_witnesses(const vector<witness_id_type>& witness_ids) const
{
    return my->_db.with_read_lock([&]() { return my->get_witnesses(witness_ids); });
}

vector<optional<witness_api_obj>> database_api_impl::get_witnesses(const vector<witness_id_type>& witness_ids) const
{
    vector<optional<witness_api_obj>> result;
    result.reserve(witness_ids.size());
    std::transform(witness_ids.begin(), witness_ids.end(), std::back_inserter(result),
                   [this](witness_id_type id) -> optional<witness_api_obj> {
                       if (auto o = _db.find(id))
                           return *o;
                       return {};
                   });
    return result;
}

fc::optional<witness_api_obj> database_api::get_witness_by_account(string account_name) const
{
    return my->_db.with_read_lock([&]() { return my->get_witness_by_account(account_name); });
}

vector<witness_api_obj> database_api::get_witnesses_by_vote(string from, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() {
        // idump((from)(limit));
        FC_ASSERT(limit <= 100);

        vector<witness_api_obj> result;
        result.reserve(limit);

        const auto& name_idx = my->_db.get_index<witness_index>().indices().get<by_name>();
        const auto& vote_idx = my->_db.get_index<witness_index>().indices().get<by_vote_name>();

        auto itr = vote_idx.begin();
        if (from.size())
        {
            auto nameitr = name_idx.find(from);
            FC_ASSERT(nameitr != name_idx.end(), "invalid witness name ${n}", ("n", from));
            itr = vote_idx.iterator_to(*nameitr);
        }

        while (itr != vote_idx.end() && result.size() < limit && itr->votes > 0)
        {
            result.push_back(witness_api_obj(*itr));
            ++itr;
        }
        return result;
    });
}

fc::optional<witness_api_obj> database_api_impl::get_witness_by_account(const string& account_name) const
{
    const auto& idx = _db.get_index<witness_index>().indices().get<by_name>();
    auto itr = idx.find(account_name);
    if (itr != idx.end())
        return witness_api_obj(*itr);
    return {};
}

set<account_name_type> database_api::lookup_witness_accounts(const string& lower_bound_name, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() { return my->lookup_witness_accounts(lower_bound_name, limit); });
}

set<account_name_type> database_api_impl::lookup_witness_accounts(const string& lower_bound_name, uint32_t limit) const
{
    FC_ASSERT(limit <= 1000);
    const auto& witnesses_by_id = _db.get_index<witness_index>().indices().get<by_id>();

    // get all the names and look them all up, sort them, then figure out what
    // records to return.  This could be optimized, but we expect the
    // number of witnesses to be few and the frequency of calls to be rare
    set<account_name_type> witnesses_by_account_name;
    for (const witness_api_obj& witness : witnesses_by_id)
        if (witness.owner >= lower_bound_name) // we can ignore anything below lower_bound_name
            witnesses_by_account_name.insert(witness.owner);

    auto end_iter = witnesses_by_account_name.begin();
    while (end_iter != witnesses_by_account_name.end() && limit--)
        ++end_iter;
    witnesses_by_account_name.erase(end_iter, witnesses_by_account_name.end());
    return witnesses_by_account_name;
}

uint64_t database_api::get_witness_count() const
{
    return my->_db.with_read_lock([&]() { return my->get_witness_count(); });
}

uint64_t database_api_impl::get_witness_count() const
{
    return _db.get_index<witness_index>().indices().size();
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Authority / validation                                           //
//                                                                  //
//////////////////////////////////////////////////////////////////////

std::string database_api::get_transaction_hex(const signed_transaction& trx) const
{
    return my->_db.with_read_lock([&]() { return my->get_transaction_hex(trx); });
}

std::string database_api_impl::get_transaction_hex(const signed_transaction& trx) const
{
    return fc::to_hex(fc::raw::pack(trx));
}

set<public_key_type> database_api::get_required_signatures(const signed_transaction& trx,
                                                           const flat_set<public_key_type>& available_keys) const
{
    return my->_db.with_read_lock([&]() { return my->get_required_signatures(trx, available_keys); });
}

set<public_key_type> database_api_impl::get_required_signatures(const signed_transaction& trx,
                                                                const flat_set<public_key_type>& available_keys) const
{
    //   wdump((trx)(available_keys));
    auto result = trx.get_required_signatures(
        get_chain_id(), available_keys,
        [&](string account_name) {
            return authority(_db.get<account_authority_object, by_account>(account_name).active);
        },
        [&](string account_name) {
            return authority(_db.get<account_authority_object, by_account>(account_name).owner);
        },
        [&](string account_name) {
            return authority(_db.get<account_authority_object, by_account>(account_name).posting);
        },
        DEIP_MAX_SIG_CHECK_DEPTH);
    //   wdump((result));
    return result;
}

set<public_key_type> database_api::get_potential_signatures(const signed_transaction& trx) const
{
    return my->_db.with_read_lock([&]() { return my->get_potential_signatures(trx); });
}

set<public_key_type> database_api_impl::get_potential_signatures(const signed_transaction& trx) const
{
    //   wdump((trx));
    set<public_key_type> result;
    trx.get_required_signatures(
        get_chain_id(), flat_set<public_key_type>(),
        [&](account_name_type account_name) {
            const auto& auth = _db.get<account_authority_object, by_account>(account_name).active;
            for (const auto& k : auth.get_keys())
                result.insert(k);
            return authority(auth);
        },
        [&](account_name_type account_name) {
            const auto& auth = _db.get<account_authority_object, by_account>(account_name).owner;
            for (const auto& k : auth.get_keys())
                result.insert(k);
            return authority(auth);
        },
        [&](account_name_type account_name) {
            const auto& auth = _db.get<account_authority_object, by_account>(account_name).posting;
            for (const auto& k : auth.get_keys())
                result.insert(k);
            return authority(auth);
        },
        DEIP_MAX_SIG_CHECK_DEPTH);

    //   wdump((result));
    return result;
}

bool database_api::verify_authority(const signed_transaction& trx) const
{
    return my->_db.with_read_lock([&]() { return my->verify_authority(trx); });
}

bool database_api_impl::verify_authority(const signed_transaction& trx) const
{
    trx.verify_authority(get_chain_id(),
                         [&](string account_name) {
                             return authority(_db.get<account_authority_object, by_account>(account_name).active);
                         },
                         [&](string account_name) {
                             return authority(_db.get<account_authority_object, by_account>(account_name).owner);
                         },
                         [&](string account_name) {
                             return authority(_db.get<account_authority_object, by_account>(account_name).posting);
                         },
                         DEIP_MAX_SIG_CHECK_DEPTH);
    return true;
}

bool database_api::verify_account_authority(const string& name_or_id, const flat_set<public_key_type>& signers) const
{
    return my->_db.with_read_lock([&]() { return my->verify_account_authority(name_or_id, signers); });
}

bool database_api_impl::verify_account_authority(const string& name, const flat_set<public_key_type>& keys) const
{
    FC_ASSERT(name.size() > 0);
    auto account = _db.find<account_object, by_name>(name);
    FC_ASSERT(account, "no such account");

    /// reuse trx.verify_authority by creating a dummy transfer
    signed_transaction trx;
    transfer_operation op;
    op.from = account->name;
    trx.operations.emplace_back(op);

    return verify_authority(trx);
}

vector<vote_state> database_api::get_active_votes(string author, string permlink) const
{
    return my->_db.with_read_lock([&]() {
        vector<vote_state> result;
        const auto& comment = my->_db.get_comment(author, permlink);
        const auto& idx = my->_db.get_index<comment_vote_index>().indices().get<by_comment_voter>();
        comment_id_type cid(comment.id);
        auto itr = idx.lower_bound(cid);
        while (itr != idx.end() && itr->comment == cid)
        {
            const auto& vo = my->_db.get(itr->voter);
            vote_state vstate;
            vstate.voter = vo.name;
            vstate.weight = itr->weight;
            vstate.rshares = itr->rshares;
            vstate.percent = itr->vote_percent;
            vstate.time = itr->last_update;

            result.push_back(vstate);
            ++itr;
        }
        return result;
    });
}

vector<account_vote> database_api::get_account_votes(string voter) const
{
    return my->_db.with_read_lock([&]() {
        vector<account_vote> result;

        const auto& voter_acnt = my->_db.get_account(voter);
        const auto& idx = my->_db.get_index<comment_vote_index>().indices().get<by_voter_comment>();

        account_id_type aid(voter_acnt.id);
        auto itr = idx.lower_bound(aid);
        auto end = idx.upper_bound(aid);
        while (itr != end)
        {
            const auto& vo = my->_db.get(itr->comment);
            account_vote avote;
            avote.authorperm = vo.author + "/" + fc::to_string(vo.permlink);
            avote.weight = itr->weight;
            avote.rshares = itr->rshares;
            avote.percent = itr->vote_percent;
            avote.time = itr->last_update;
            result.push_back(avote);
            ++itr;
        }
        return result;
    });
}

u256 to256(const fc::uint128& t)
{
    u256 result(t.high_bits());
    result <<= 65;
    result += t.low_bits();
    return result;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// Budgets                                                          //
//                                                                  //
//////////////////////////////////////////////////////////////////////
vector<budget_api_obj> database_api::get_budgets(const set<string>& names) const
{
    return my->_db.with_read_lock([&]() { return my->get_budgets(names); });
}

vector<budget_api_obj> database_api_impl::get_budgets(const set<string>& names) const
{
    FC_ASSERT(names.size() <= DEIP_LIMIT_API_BUDGETS_LIST_SIZE, "names size must be less or equal than ${1}",
              ("1", DEIP_LIMIT_API_BUDGETS_LIST_SIZE));

    vector<budget_api_obj> results;

    chain::dbs_budget& budget_service = _db.obtain_service<chain::dbs_budget>();

    for (const auto& name : names)
    {
        auto budgets = budget_service.get_budgets(name);
        if (results.size() + budgets.size() > DEIP_LIMIT_API_BUDGETS_LIST_SIZE)
        {
            break;
        }

        for (const chain::budget_object& budget : budgets)
        {
            results.push_back(budget_api_obj(budget));
        }
    }

    return results;
}

set<string> database_api::lookup_budget_owners(const string& lower_bound_name, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() { return my->lookup_budget_owners(lower_bound_name, limit); });
}

set<string> database_api_impl::lookup_budget_owners(const string& lower_bound_name, uint32_t limit) const
{
    FC_ASSERT(limit <= DEIP_LIMIT_API_BUDGETS_LIST_SIZE, "limit must be less or equal than ${1}",
              ("1", DEIP_LIMIT_API_BUDGETS_LIST_SIZE));

    chain::dbs_budget& budget_service = _db.obtain_service<chain::dbs_budget>();

    return budget_service.lookup_budget_owners(lower_bound_name, limit);
}

/**
 *  This method can be used to fetch replies to an account.
 *
 *  The first call should be (account_to_retrieve replies, "", limit)
 *  Subsequent calls should be (last_author, last_permlink, limit)
 */

map<uint32_t, applied_operation> database_api::get_account_history(string account, uint64_t from, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() {
        FC_ASSERT(limit <= 10000, "Limit of ${l} is greater than maxmimum allowed", ("l", limit));
        FC_ASSERT(from >= limit, "From must be greater than limit");
        //   idump((account)(from)(limit));
        const auto& idx = my->_db.get_index<account_history_index>().indices().get<by_account>();
        auto itr = idx.lower_bound(boost::make_tuple(account, from));
        //   if( itr != idx.end() ) idump((*itr));
        auto end = idx.upper_bound(boost::make_tuple(account, std::max(int64_t(0), int64_t(itr->sequence) - limit)));
        //   if( end != idx.end() ) idump((*end));

        map<uint32_t, applied_operation> result;
        while (itr != end)
        {
            result[itr->sequence] = my->_db.get(itr->op);
            ++itr;
        }
        return result;
    });
}

vector<pair<string, uint32_t>> database_api::get_tags_used_by_author(const string& author) const
{
    return my->_db.with_read_lock([&]() {
        const auto* acnt = my->_db.find_account(author);
        FC_ASSERT(acnt != nullptr);
        const auto& tidx = my->_db.get_index<tags::author_tag_stats_index>().indices().get<tags::by_author_posts_tag>();
        auto itr = tidx.lower_bound(boost::make_tuple(acnt->id, 0));
        vector<pair<string, uint32_t>> result;
        while (itr != tidx.end() && itr->author == acnt->id && result.size() < 1000)
        {
            result.push_back(std::make_pair(itr->tag, itr->total_posts));
            ++itr;
        }
        return result;
    });
}

vector<tag_api_obj> database_api::get_trending_tags(string after, uint32_t limit) const
{
    return my->_db.with_read_lock([&]() {
        limit = std::min(limit, uint32_t(1000));
        vector<tag_api_obj> result;
        result.reserve(limit);

        const auto& nidx = my->_db.get_index<tags::tag_stats_index>().indices().get<tags::by_tag>();

        const auto& ridx = my->_db.get_index<tags::tag_stats_index>().indices().get<tags::by_trending>();
        auto itr = ridx.begin();
        if (after != "" && nidx.size())
        {
            auto nitr = nidx.lower_bound(after);
            if (nitr == nidx.end())
                itr = ridx.end();
            else
                itr = ridx.iterator_to(*nitr);
        }

        while (itr != ridx.end() && result.size() < limit)
        {
            result.push_back(tag_api_obj(*itr));
            ++itr;
        }
        return result;
    });
}

/**
 *  This call assumes root already stored as part of state, it will
 *  modify root.replies to contain links to the reply posts and then
 *  add the reply discussions to the state. This method also fetches
 *  any accounts referenced by authors.
 *
 */

vector<account_name_type> database_api::get_active_witnesses() const
{
    return my->_db.with_read_lock([&]() {
        const auto& wso = my->_db.get_witness_schedule_object();
        size_t n = wso.current_shuffled_witnesses.size();
        vector<account_name_type> result;
        result.reserve(n);
        for (size_t i = 0; i < n; i++)
            result.push_back(wso.current_shuffled_witnesses[i]);
        return result;
    });
}

vector<vesting_delegation_api_obj>
database_api::get_vesting_delegations(string account, string from, uint32_t limit) const
{
    FC_ASSERT(limit <= 1000);

    return my->_db.with_read_lock([&]() {
        vector<vesting_delegation_api_obj> result;
        result.reserve(limit);

        const auto& delegation_idx = my->_db.get_index<vesting_delegation_index, by_delegation>();
        auto itr = delegation_idx.lower_bound(boost::make_tuple(account, from));
        while (result.size() < limit && itr != delegation_idx.end() && itr->delegator == account)
        {
            result.push_back(*itr);
            ++itr;
        }

        return result;
    });
}

vector<vesting_delegation_expiration_api_obj>
database_api::get_expiring_vesting_delegations(string account, time_point_sec from, uint32_t limit) const
{
    FC_ASSERT(limit <= 1000);

    return my->_db.with_read_lock([&]() {
        vector<vesting_delegation_expiration_api_obj> result;
        result.reserve(limit);

        const auto& exp_idx = my->_db.get_index<vesting_delegation_expiration_index, by_account_expiration>();
        auto itr = exp_idx.lower_bound(boost::make_tuple(account, from));
        while (result.size() < limit && itr != exp_idx.end() && itr->delegator == account)
        {
            result.push_back(*itr);
            ++itr;
        }

        return result;
    });
}

state database_api::get_state(string path) const
{
    return my->_db.with_read_lock([&]() {
        state _state;
        _state.props = get_dynamic_global_properties();
        _state.current_route = path;

        try
        {
            if (path.size() && path[0] == '/')
                path = path.substr(1); /// remove '/' from front

            if (!path.size())
                path = "trending";

            /// FETCH CATEGORY STATE
            auto trending_tags = get_trending_tags(std::string(), 50);
            for (const auto& t : trending_tags)
            {
                _state.tag_idx.trending.push_back(string(t.name));
            }
            /// END FETCH CATEGORY STATE

            set<string> accounts;

            vector<string> part;
            part.reserve(4);
            boost::split(part, path, boost::is_any_of("/"));
            part.resize(std::max(part.size(), size_t(4))); // at least 4

            auto tag = fc::to_lower(part[1]);

            if (part[0].size() && part[0][0] == '@')
            {
                auto acnt = part[0].substr(1);
                _state.accounts[acnt] = extended_account(my->_db.get_account(acnt), my->_db);
                _state.accounts[acnt].tags_usage = get_tags_used_by_author(acnt);

                auto& eacnt = _state.accounts[acnt];
                if (part[1] == "transfers")
                {
                    auto history = get_account_history(acnt, uint64_t(-1), 10000);
                    for (auto& item : history)
                    {
                        switch (item.second.op.which())
                        {
                        case operation::tag<transfer_to_vesting_operation>::value:
                        case operation::tag<withdraw_vesting_operation>::value:
                        case operation::tag<transfer_operation>::value:
                        case operation::tag<author_reward_operation>::value:
                        case operation::tag<curation_reward_operation>::value:
                        case operation::tag<comment_benefactor_reward_operation>::value:
                        case operation::tag<escrow_transfer_operation>::value:
                        case operation::tag<escrow_approve_operation>::value:
                        case operation::tag<escrow_dispute_operation>::value:
                        case operation::tag<escrow_release_operation>::value:
                            eacnt.transfer_history[item.first] = item.second;
                            break;
                        case operation::tag<comment_operation>::value:
                            //   eacnt.post_history[item.first] =  item.second;
                            break;
                        case operation::tag<vote_operation>::value:
                        case operation::tag<account_witness_vote_operation>::value:
                        case operation::tag<account_witness_proxy_operation>::value:
                            //   eacnt.vote_history[item.first] =  item.second;
                            break;
                        case operation::tag<account_create_operation>::value:
                        case operation::tag<account_update_operation>::value:
                        case operation::tag<witness_update_operation>::value:
                        case operation::tag<producer_reward_operation>::value:
                        default:
                            eacnt.other_history[item.first] = item.second;
                        }
                    }
                }
            }
            /// pull a complete discussion

            else if (part[0] == "witnesses" || part[0] == "~witnesses")
            {
                auto wits = get_witnesses_by_vote("", 50);
                for (const auto& w : wits)
                {
                    _state.witnesses[w.owner] = w;
                }
            }

            else if (part[0] == "tags")
            {
                _state.tag_idx.trending.clear();
                auto trending_tags = get_trending_tags(std::string(), 250);
                for (const auto& t : trending_tags)
                {
                    string name = t.name;
                    _state.tag_idx.trending.push_back(name);
                    _state.tags[name] = t;
                }
            }
            else
            {
                elog("What... no matches");
            }

            for (const auto& a : accounts)
            {
                _state.accounts.erase("");
                _state.accounts[a] = extended_account(my->_db.get_account(a), my->_db);
            }
            for (auto& d : _state.content)
            {
                d.second.active_votes = get_active_votes(d.second.author, d.second.permlink);
            }

            _state.witness_schedule = my->_db.get_witness_schedule_object();
        }
        catch (const fc::exception& e)
        {
            _state.error = e.to_detail_string();
        }
        return _state;
    });
}

annotated_signed_transaction database_api::get_transaction(transaction_id_type id) const
{
#ifdef SKIP_BY_TX_ID
    FC_ASSERT(false, "This node's operator has disabled operation indexing by transaction_id");
#else
    return my->_db.with_read_lock([&]() {
        const auto& idx = my->_db.get_index<operation_index>().indices().get<by_transaction_id>();
        auto itr = idx.lower_bound(id);
        if (itr != idx.end() && itr->trx_id == id)
        {
            auto blk = my->_db.fetch_block_by_number(itr->block);
            FC_ASSERT(blk.valid());
            FC_ASSERT(blk->transactions.size() > itr->trx_in_block);
            annotated_signed_transaction result = blk->transactions[itr->trx_in_block];
            result.block_num = itr->block;
            result.transaction_num = itr->trx_in_block;
            return result;
        }
        FC_ASSERT(false, "Unknown Transaction ${t}", ("t", id));
    });
#endif
}

vector<discipline_api_obj> database_api::get_all_disciplines() const
{
    return my->_db.with_read_lock([&]() {
        vector<discipline_api_obj> results;

        chain::dbs_discipline &discipline_service = my->_db.obtain_service<chain::dbs_discipline>();

        auto disciplines = discipline_service.get_disciplines();

        for (const chain::discipline_object &discipline : disciplines) {
            results.push_back(discipline_api_obj(discipline));
        }

        return results;
    });
}

discipline_api_obj database_api::get_discipline(const discipline_id_type id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_discipline &discipline_service = my->_db.obtain_service<chain::dbs_discipline>();

        return discipline_service.get_discipline(id);
    });
}

discipline_api_obj database_api::get_discipline_by_name(const discipline_name_type name) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_discipline &discipline_service = my->_db.obtain_service<chain::dbs_discipline>();

        return discipline_service.get_discipline_by_name(name);
    });
}

vector<discipline_api_obj> database_api::get_disciplines_by_parent_id(const discipline_id_type parent_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<discipline_api_obj> results;

        chain::dbs_discipline &discipline_service = my->_db.obtain_service<chain::dbs_discipline>();

        auto disciplines = discipline_service.get_disciplines_by_parent_id(parent_id);

        for (const chain::discipline_object &discipline : disciplines) {
            results.push_back(discipline_api_obj(discipline));
        }

        return results;
    });
}


research_api_obj database_api::get_research_by_id(const research_id_type& id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_research &research_service = my->_db.obtain_service<chain::dbs_research>();
        return research_service.get_research(id);
    });
}

research_api_obj database_api::get_research_by_permlink(const string& permlink) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_research &research_service = my->_db.obtain_service<chain::dbs_research>();
        return research_service.get_research_by_permlink(permlink);
    });
}

vector<research_api_obj> database_api::get_researches() const
{
    return my->_db.with_read_lock([&]() {
        vector<research_api_obj> results;

        chain::dbs_research &research_service = my->_db.obtain_service<chain::dbs_research>();
        auto researches = research_service.get_researches();

        for (const chain::research_object &research : researches) {
            results.push_back(research_api_obj(research));
        }

        return results;
    });
}

research_content_api_obj database_api::get_research_content_by_id(const research_content_id_type& id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_research_content &research_content_service = my->_db.obtain_service<chain::dbs_research_content>();
        return research_content_service.get_content_by_id(id);
    });
}

vector<research_content_api_obj> database_api::get_all_research_content(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_content_api_obj> results;
        chain::dbs_research_content &research_content_service = my->_db.obtain_service<chain::dbs_research_content>();
        auto contents = research_content_service.get_content_by_research_id(research_id);

        for (const chain::research_content_object &content : contents) {
            results.push_back(research_content_api_obj(content));
        }

        return results;
    });
}

vector<research_content_api_obj> database_api::get_research_content_by_type(const research_id_type& research_id, const research_content_type& type) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_content_api_obj> results;
        chain::dbs_research_content &research_content_service = my->_db.obtain_service<chain::dbs_research_content>();
        auto contents = research_content_service.get_content_by_research_id_and_content_type(research_id, type);

        for (const chain::research_content_object &content : contents) {
            results.push_back(research_content_api_obj(content));
        }
        return results;
    });
}


expert_token_api_obj database_api::get_expert_token(const expert_token_id_type id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_expert_token &expert_token_service = my->_db.obtain_service<chain::dbs_expert_token>();
        auto expert_token = expert_token_service.get_expert_token(id);
        return expert_token_api_obj(expert_token);
    });
}

vector<expert_token_api_obj> database_api::get_expert_tokens_by_account_name(const account_name_type account_name) const
{
    return my->_db.with_read_lock([&]() {
        vector<expert_token_api_obj> results;
        chain::dbs_expert_token &expert_token_service = my->_db.obtain_service<chain::dbs_expert_token>();
        auto expert_tokens = expert_token_service.get_expert_tokens_by_account_name(account_name);

        for (const chain::expert_token_object &expert_token : expert_tokens) {
            results.push_back(expert_token_api_obj(expert_token));
        }
        return results;
    });
}

vector<expert_token_api_obj> database_api::get_expert_tokens_by_discipline_id(const discipline_id_type discipline_id) const
{
      return my->_db.with_read_lock([&]() {
        vector<expert_token_api_obj> results;

        chain::dbs_expert_token &expert_token_service = my->_db.obtain_service<chain::dbs_expert_token>();
        auto expert_tokens = expert_token_service.get_expert_tokens_by_discipline_id(discipline_id);

        for (const chain::expert_token_object &expert_token : expert_tokens) {
            results.push_back(expert_token_api_obj(expert_token));
        }

        return results;
    });
}

vector<proposal_api_obj>
database_api::get_proposals_by_research_group_id(const research_group_id_type research_group_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<proposal_api_obj> results;

        chain::dbs_proposal& proposal_service = my->_db.obtain_service<chain::dbs_proposal>();
        auto proposals = proposal_service.get_proposals_by_research_group_id(research_group_id);

        for (const chain::proposal_object& proposal : proposals)
        {
            results.push_back(proposal_api_obj(proposal));
        }

        return results;
    });
}

proposal_api_obj database_api::get_proposal(const proposal_id_type id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_proposal &proposal_service = my->_db.obtain_service<chain::dbs_proposal>();

        return proposal_service.get_proposal(id);
    });
}

vector<research_group_token_api_obj>
database_api::get_research_group_tokens_by_account(const account_name_type account) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_group_token_api_obj> results;

        chain::dbs_research_group& research_group_service = my->_db.obtain_service<chain::dbs_research_group>();
        auto research_group_tokens = research_group_service.get_research_group_tokens_by_account_name(account);

        for (const chain::research_group_token_object& research_group_token : research_group_tokens)
        {
            results.push_back(research_group_token_api_obj(research_group_token));
        }

        return results;
    });
}

research_group_token_api_obj database_api::get_research_group_token_by_account_and_research_group_id(
    const account_name_type account, const research_group_id_type research_group_id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_research_group& research_group_service = my->_db.obtain_service<chain::dbs_research_group>();

        return research_group_service.get_research_group_token_by_account_and_research_group_id(account, research_group_id);
    });
}

research_group_api_obj database_api::get_research_group_by_id(const research_group_id_type research_group_id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_research_group &research_group_service = my->_db.obtain_service<chain::dbs_research_group>();

        research_group_service.check_research_group_existence(research_group_id);
        return research_group_service.get_research_group(research_group_id);
    });
}

research_group_api_obj database_api::get_research_group_by_permlink(const string& permlink) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_research_group &research_group_service = my->_db.obtain_service<chain::dbs_research_group>();

        research_group_service.check_research_group_existence_by_permlink(permlink);
        return research_group_service.get_research_group_by_permlink(permlink);
    });
}

research_token_sale_api_obj
database_api::get_research_token_sale_by_id(const research_token_sale_id_type research_token_sale_id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_research_token_sale& research_token_sale_service
            = my->_db.obtain_service<chain::dbs_research_token_sale>();
        return research_token_sale_service.get_research_token_sale_by_id(research_token_sale_id);
    });
}

research_token_sale_api_obj
database_api::get_research_token_sale_by_research_id(const research_id_type& research_id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_research_token_sale& research_token_sale_service
            = my->_db.obtain_service<chain::dbs_research_token_sale>();
        return research_token_sale_service.get_research_token_sale_by_research_id(research_id);
    });
}

vector<research_token_sale_api_obj>
database_api::get_research_token_sale_by_end_time(const time_point_sec end_time) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_token_sale_api_obj> results;
        chain::dbs_research_token_sale& research_token_sale_service
            = my->_db.obtain_service<chain::dbs_research_token_sale>();

        auto research_token_sales = research_token_sale_service.get_research_token_sale_by_end_time(end_time);

        for (const chain::research_token_sale_object& research_token_sale : research_token_sales)
        {
            results.push_back(research_token_sale);
        }

        return results;
    });
}

research_token_sale_contribution_api_obj
database_api::get_research_token_sale_contribution_by_id(const research_token_sale_contribution_id_type research_token_sale_contribution_id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_research_token_sale& research_token_sale_service
            = my->_db.obtain_service<chain::dbs_research_token_sale>();
        return research_token_sale_service.get_research_token_sale_contribution_by_id(research_token_sale_contribution_id);
    });
}

vector<research_token_sale_contribution_api_obj>
database_api::get_research_token_sale_contributions_by_research_token_sale_id(const research_token_sale_id_type research_token_sale_id) const
{
    return my->_db.with_read_lock([&]() {
        vector<research_token_sale_contribution_api_obj> results;
        chain::dbs_research_token_sale& research_token_sale_service
            = my->_db.obtain_service<chain::dbs_research_token_sale>();

        auto research_token_sale_contributions = research_token_sale_service.get_research_token_sale_contributions_by_research_token_sale_id(research_token_sale_id);

        for (const chain::research_token_sale_contribution_object& research_token_sale_contribution : research_token_sale_contributions)
        {
            results.push_back(research_token_sale_contribution);
        }

        return results;
    });
}

research_token_sale_contribution_api_obj
database_api::get_research_token_sale_contribution_by_account_name_and_research_token_sale_id(const account_name_type owner,
                                                                                              const research_token_sale_id_type research_token_sale_id) const
{
    return my->_db.with_read_lock([&]() {
        chain::dbs_research_token_sale& research_token_sale_service
            = my->_db.obtain_service<chain::dbs_research_token_sale>();
        return research_token_sale_service.get_research_token_sale_contribution_by_account_name_and_research_token_sale_id(owner, research_token_sale_id);
    });
}


} // namespace app
} // namespace deip
