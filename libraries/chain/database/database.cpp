#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/schema/block_summary_object.hpp>
#include <deip/chain/database/database.hpp>
#include <deip/chain/database/database_exceptions.hpp>
#include <deip/chain/database/db_with.hpp>
#include <deip/chain/evaluator_registry.hpp>
#include <deip/chain/schema/global_property_object.hpp>
#include <deip/chain/schema/chain_property_object.hpp>
#include <deip/chain/schema/operation_object.hpp>
#include <deip/chain/deip_evaluator.hpp>
#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/schema/transaction_object.hpp>
#include <deip/chain/shared_db_merkle.hpp>
#include <deip/chain/operation_notification.hpp>
#include <deip/chain/schema/grant_objects.hpp>
#include <deip/chain/schema/proposal_object.hpp>
#include <deip/chain/genesis_state.hpp>
#include <deip/chain/schema/research_group_object.hpp>
#include <deip/chain/schema/discipline_object.hpp>
#include <deip/chain/schema/research_discipline_relation_object.hpp>
#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/schema/research_content_object.hpp>
#include <deip/chain/schema/research_token_sale_object.hpp>
#include <deip/chain/schema/expert_token_object.hpp>
#include <deip/chain/schema/research_token_object.hpp>
#include <deip/chain/schema/vote_object.hpp>
#include <deip/chain/schema/total_votes_object.hpp>
#include <deip/chain/schema/research_group_invite_object.hpp>
#include <deip/chain/schema/review_object.hpp>
#include <deip/chain/schema/vesting_balance_object.hpp>
#include <deip/chain/schema/expertise_stats_object.hpp>

#include <deip/chain/util/asset.hpp>
#include <deip/chain/util/reward.hpp>
#include <deip/chain/util/uint256.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>
#include <fc/container/deque.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>

#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>

#include <openssl/md5.h>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/uniqued.hpp>

#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_witness.hpp>
#include <deip/chain/services/dbs_proposal.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research_token_sale.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_dynamic_global_properties.hpp>
#include <deip/chain/services/dbs_vote.hpp>
#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/services/dbs_research_group_invite.hpp>
#include <deip/chain/services/dbs_grant.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_vesting_balance.hpp>
#include <deip/chain/services/dbs_proposal_execution.hpp>
#include <deip/chain/services/dbs_research_content_reward_pool.hpp>
#include <deip/chain/services/dbs_expertise_stats.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace deip {
namespace chain {

struct object_schema_repr
{
    std::pair<uint16_t, uint16_t> space_type;
    std::string type;
};

struct operation_schema_repr
{
    std::string id;
    std::string type;
};

struct db_schema
{
    std::map<std::string, std::string> types;
    std::vector<object_schema_repr> object_types;
    std::string operation_type;
    std::vector<operation_schema_repr> custom_operation_types;
};
} // namespace chain
} // namespace deip

FC_REFLECT(deip::chain::object_schema_repr, (space_type)(type))
FC_REFLECT(deip::chain::operation_schema_repr, (id)(type))
FC_REFLECT(deip::chain::db_schema, (types)(object_types)(operation_type)(custom_operation_types))

namespace deip {
namespace chain {

using boost::container::flat_set;

class database_impl
{
public:
    database_impl(database& self);

    database& _self;
    evaluator_registry<operation> _evaluator_registry;
};

database_impl::database_impl(database& self)
    : _self(self)
    , _evaluator_registry(self)
{
}

database::database()
    : chainbase::database()
    , dbservice(*this)
    , _my(new database_impl(*this))
{
}

database::~database()
{
    clear_pending();
}

void database::open(const fc::path& data_dir,
                    const fc::path& shared_mem_dir,
                    uint64_t shared_file_size,
                    uint32_t chainbase_flags,
                    const genesis_state_type& genesis_state)
{
    try
    {
        chainbase::database::open(shared_mem_dir, chainbase_flags, shared_file_size);

        initialize_indexes();
        initialize_evaluators();

        if (chainbase_flags & chainbase::database::read_write)
        {
            if (!find<dynamic_global_property_object>())
                with_write_lock([&]() { init_genesis(genesis_state); });

            if (!fc::exists(data_dir))
                fc::create_directories(data_dir);

            _block_log.open(data_dir / "block_log");

            auto log_head = _block_log.head();

            // Rewind all undo state. This should return us to the state at the last irreversible block.
            with_write_lock([&]() {
                undo_all();
                FC_ASSERT(revision() == head_block_num(), "Chainbase revision does not match head block num",
                          ("rev", revision())("head_block", head_block_num()));
                validate_invariants();
            });

            if (head_block_num())
            {
                auto head_block = _block_log.read_block_by_num(head_block_num());
                // This assertion should be caught and a reindex should occur
                FC_ASSERT(head_block.valid() && head_block->id() == head_block_id(),
                          "Chain state does not match block log. Please reindex blockchain.");

                _fork_db.start_block(*head_block);
            }
        }

        with_read_lock([&]() {
            init_hardforks(genesis_state.initial_timestamp); // Writes to local state, but reads from db
        });
    }
    FC_CAPTURE_LOG_AND_RETHROW((data_dir)(shared_mem_dir)(shared_file_size))
}

void database::reindex(const fc::path& data_dir,
                       const fc::path& shared_mem_dir,
                       uint64_t shared_file_size,
                       const genesis_state_type& genesis_state)
{
    try
    {
        ilog("Reindexing Blockchain");
        wipe(data_dir, shared_mem_dir, false);
        open(data_dir, shared_mem_dir, shared_file_size, chainbase::database::read_write, genesis_state);
        _fork_db.reset(); // override effect of _fork_db.start_block() call in open()

        auto start = fc::time_point::now();
        DEIP_ASSERT(_block_log.head(), block_log_exception, "No blocks in block log. Cannot reindex an empty chain.");

        ilog("Replaying blocks...");

        uint64_t skip_flags = skip_witness_signature | skip_transaction_signatures | skip_transaction_dupe_check
            | skip_tapos_check | skip_merkle_check | skip_witness_schedule_check | skip_authority_check | skip_validate
            | /// no need to validate operations
            skip_validate_invariants | skip_block_log;

        with_write_lock([&]() {
            auto itr = _block_log.read_block(0);
            auto last_block_num = _block_log.head()->block_num();

            while (itr.first.block_num() != last_block_num)
            {
                auto cur_block_num = itr.first.block_num();
                if (cur_block_num % 100000 == 0)
                    std::cerr << "   " << double(cur_block_num * 100) / last_block_num << "%   " << cur_block_num
                              << " of " << last_block_num << "   (" << (get_free_memory() / (1024 * 1024))
                              << "M free)\n";
                apply_block(itr.first, skip_flags);
                itr = _block_log.read_block(itr.second);
            }

            apply_block(itr.first, skip_flags);
            set_revision(head_block_num());
        });

        if (_block_log.head()->block_num())
            _fork_db.start_block(*_block_log.head());

        auto end = fc::time_point::now();
        ilog("Done reindexing, elapsed time: ${t} sec", ("t", double((end - start).count()) / 1000000.0));
    }
    FC_CAPTURE_AND_RETHROW((data_dir)(shared_mem_dir))
}

void database::wipe(const fc::path& data_dir, const fc::path& shared_mem_dir, bool include_blocks)
{
    close();
    chainbase::database::wipe(shared_mem_dir);
    if (include_blocks)
    {
        fc::remove_all(data_dir / "block_log");
        fc::remove_all(data_dir / "block_log.index");
    }
}

void database::close()
{
    try
    {
        // Since pop_block() will move tx's in the popped blocks into pending,
        // we have to clear_pending() after we're done popping to get a clean
        // DB state (issue #336).
        clear_pending();

        chainbase::database::flush();
        chainbase::database::close();

        _block_log.close();

        _fork_db.reset();
    }
    FC_CAPTURE_AND_RETHROW()
}

bool database::is_known_block(const block_id_type& id) const
{
    try
    {
        return fetch_block_by_id(id).valid();
    }
    FC_CAPTURE_AND_RETHROW()
}

/**
 * Only return true *if* the transaction has not expired or been invalidated. If this
 * method is called with a VERY old transaction we will return false, they should
 * query things by blocks if they are that old.
 */
bool database::is_known_transaction(const transaction_id_type& id) const
{
    try
    {
        const auto& trx_idx = get_index<transaction_index>().indices().get<by_trx_id>();
        return trx_idx.find(id) != trx_idx.end();
    }
    FC_CAPTURE_AND_RETHROW()
}

block_id_type database::find_block_id_for_num(uint32_t block_num) const
{
    try
    {
        if (block_num == 0)
            return block_id_type();

        // Reversible blocks are *usually* in the TAPOS buffer.  Since this
        // is the fastest check, we do it first.
        block_summary_id_type bsid = block_num & 0xFFFF;
        const block_summary_object* bs = find<block_summary_object, by_id>(bsid);
        if (bs != nullptr)
        {
            if (protocol::block_header::num_from_id(bs->block_id) == block_num)
                return bs->block_id;
        }

        // Next we query the block log.   Irreversible blocks are here.
        auto b = _block_log.read_block_by_num(block_num);
        if (b.valid())
            return b->id();

        // Finally we query the fork DB.
        shared_ptr<fork_item> fitem = _fork_db.fetch_block_on_main_branch_by_number(block_num);
        if (fitem)
            return fitem->id;

        return block_id_type();
    }
    FC_CAPTURE_AND_RETHROW((block_num))
}

block_id_type database::get_block_id_for_num(uint32_t block_num) const
{
    block_id_type bid = find_block_id_for_num(block_num);
    FC_ASSERT(bid != block_id_type());
    return bid;
}

optional<signed_block> database::fetch_block_by_id(const block_id_type& id) const
{
    try
    {
        auto b = _fork_db.fetch_block(id);
        if (!b)
        {
            auto tmp = _block_log.read_block_by_num(protocol::block_header::num_from_id(id));

            if (tmp && tmp->id() == id)
                return tmp;

            tmp.reset();
            return tmp;
        }

        return b->data;
    }
    FC_CAPTURE_AND_RETHROW()
}

optional<signed_block> database::fetch_block_by_number(uint32_t block_num) const
{
    try
    {
        optional<signed_block> b;

        auto results = _fork_db.fetch_block_by_number(block_num);
        if (results.size() == 1)
            b = results[0]->data;
        else
            b = _block_log.read_block_by_num(block_num);

        return b;
    }
    FC_LOG_AND_RETHROW()
}

const signed_transaction database::get_recent_transaction(const transaction_id_type& trx_id) const
{
    try
    {
        auto& index = get_index<transaction_index>().indices().get<by_trx_id>();
        auto itr = index.find(trx_id);
        FC_ASSERT(itr != index.end());
        signed_transaction trx;
        fc::raw::unpack(itr->packed_trx, trx);
        return trx;
    }
    FC_CAPTURE_AND_RETHROW()
}

std::vector<block_id_type> database::get_block_ids_on_fork(block_id_type head_of_fork) const
{
    try
    {
        pair<fork_database::branch_type, fork_database::branch_type> branches
            = _fork_db.fetch_branch_from(head_block_id(), head_of_fork);
        if (!((branches.first.back()->previous_id() == branches.second.back()->previous_id())))
        {
            edump((head_of_fork)(head_block_id())(branches.first.size())(branches.second.size()));
            assert(branches.first.back()->previous_id() == branches.second.back()->previous_id());
        }
        std::vector<block_id_type> result;
        for (const item_ptr& fork_block : branches.second)
            result.emplace_back(fork_block->id);
        result.emplace_back(branches.first.back()->previous_id());
        return result;
    }
    FC_CAPTURE_AND_RETHROW()
}

chain_id_type database::get_chain_id() const
{
    return get<chain_property_object>().chain_id;
}

const witness_object& database::get_witness(const account_name_type& name) const
{
    try
    {
        return get<witness_object, by_name>(name);
    }
    FC_CAPTURE_AND_RETHROW((name))
}

const witness_object* database::find_witness(const account_name_type& name) const
{
    return find<witness_object, by_name>(name);
}

const account_object& database::get_account(const account_name_type& name) const
{
    try
    {
        return get<account_object, by_name>(name);
    }
    FC_CAPTURE_AND_RETHROW((name))
}

const account_object* database::find_account(const account_name_type& name) const
{
    return find<account_object, by_name>(name);
}

const dynamic_global_property_object& database::get_dynamic_global_properties() const
{
    try
    {
        return get<dynamic_global_property_object>();
    }
    FC_CAPTURE_AND_RETHROW()
}

const expertise_stats_object& database::get_expertise_stats() const
{
    try
    {
        return get<expertise_stats_object>();
    }
    FC_CAPTURE_AND_RETHROW()
}

const node_property_object& database::get_node_properties() const
{
    return _node_property_object;
}

const witness_schedule_object& database::get_witness_schedule_object() const
{
    try
    {
        return get<witness_schedule_object>();
    }
    FC_CAPTURE_AND_RETHROW()
}

const hardfork_property_object& database::get_hardfork_property_object() const
{
    try
    {
        return get<hardfork_property_object>();
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::pay_fee(const account_object& account, asset fee)
{
    FC_ASSERT(fee.amount >= 0); /// NOTE if this fails then validate() on some operation is probably wrong
    if (fee.amount == 0)
        return;

    FC_ASSERT(account.balance >= fee);
    adjust_balance(account, -fee);
    adjust_supply(-fee);
}

uint32_t database::witness_participation_rate() const
{
    const dynamic_global_property_object& dpo = get_dynamic_global_properties();
    return uint64_t(DEIP_100_PERCENT) * dpo.recent_slots_filled.popcount() / 128;
}

void database::add_checkpoints(const flat_map<uint32_t, block_id_type>& checkpts)
{
    for (const auto& i : checkpts)
        _checkpoints[i.first] = i.second;
}

bool database::before_last_checkpoint() const
{
    return (_checkpoints.size() > 0) && (_checkpoints.rbegin()->first >= head_block_num());
}

/**
 * Push block "may fail" in which case every partial change is unwound.  After
 * push block is successful the block is appended to the chain database on disk.
 *
 * @return true if we switched forks as a result of this push.
 */
bool database::push_block(const signed_block& new_block, uint32_t skip)
{
    // fc::time_point begin_time = fc::time_point::now();

    bool result;
    detail::with_skip_flags(*this, skip, [&]() {
        with_write_lock([&]() {
            detail::without_pending_transactions(*this, std::move(_pending_tx), [&]() {
                try
                {
                    result = _push_block(new_block);
                }
                FC_CAPTURE_AND_RETHROW((new_block))
            });
        });
    });

    // fc::time_point end_time = fc::time_point::now();
    // fc::microseconds dt = end_time - begin_time;
    // if( ( new_block.block_num() % 10000 ) == 0 )
    //   ilog( "push_block ${b} took ${t} microseconds", ("b", new_block.block_num())("t", dt.count()) );
    return result;
}

void database::_maybe_warn_multiple_production(uint32_t height) const
{
    auto blocks = _fork_db.fetch_block_by_number(height);
    if (blocks.size() > 1)
    {
        vector<std::pair<account_name_type, fc::time_point_sec>> witness_time_pairs;
        for (const auto& b : blocks)
        {
            witness_time_pairs.push_back(std::make_pair(b->data.witness, b->data.timestamp));
        }

        ilog("Encountered block num collision at block ${n} due to a fork, witnesses are: ${w}",
             ("n", height)("w", witness_time_pairs));
    }
    return;
}

bool database::_push_block(const signed_block& new_block)
{
    try
    {
        uint32_t skip = get_node_properties().skip_flags;
        // uint32_t skip_undo_db = skip & skip_undo_block;

        if (!(skip & skip_fork_db))
        {
            shared_ptr<fork_item> new_head = _fork_db.push_block(new_block);
            _maybe_warn_multiple_production(new_head->num);

            // If the head block from the longest chain does not build off of the current head, we need to switch forks.
            if (new_head->data.previous != head_block_id())
            {
                // If the newly pushed block is the same height as head, we get head back in new_head
                // Only switch forks if new_head is actually higher than head
                if (new_head->data.block_num() > head_block_num())
                {
                    // wlog( "Switching to fork: ${id}", ("id",new_head->data.id()) );
                    auto branches = _fork_db.fetch_branch_from(new_head->data.id(), head_block_id());

                    // pop blocks until we hit the forked block
                    while (head_block_id() != branches.second.back()->data.previous)
                        pop_block();

                    // push all blocks on the new fork
                    for (auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr)
                    {
                        // ilog( "pushing blocks from fork ${n} ${id}",
                        // ("n",(*ritr)->data.block_num())("id",(*ritr)->data.id()) );
                        optional<fc::exception> except;
                        try
                        {
                            auto session = start_undo_session(true);
                            apply_block((*ritr)->data, skip);
                            session.push();
                        }
                        catch (const fc::exception& e)
                        {
                            except = e;
                        }
                        if (except)
                        {
                            // wlog( "exception thrown while switching forks ${e}", ("e",except->to_detail_string() ) );
                            // remove the rest of branches.first from the fork_db, those blocks are invalid
                            while (ritr != branches.first.rend())
                            {
                                _fork_db.remove((*ritr)->data.id());
                                ++ritr;
                            }
                            _fork_db.set_head(branches.second.front());

                            // pop all blocks from the bad fork
                            while (head_block_id() != branches.second.back()->data.previous)
                                pop_block();

                            // restore all blocks from the good fork
                            for (auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr)
                            {
                                auto session = start_undo_session(true);
                                apply_block((*ritr)->data, skip);
                                session.push();
                            }
                            throw *except;
                        }
                    }
                    return true;
                }
                else
                    return false;
            }
        }

        try
        {
            auto session = start_undo_session(true);
            apply_block(new_block, skip);
            session.push();
        }
        catch (const fc::exception& e)
        {
            elog("Failed to push new block:\n${e}", ("e", e.to_detail_string()));
            _fork_db.remove(new_block.id());
            throw;
        }

        return false;
    }
    FC_CAPTURE_AND_RETHROW()
}

/**
 * Attempts to push the transaction into the pending queue
 *
 * When called to push a locally generated transaction, set the skip_block_size_check bit on the skip argument. This
 * will allow the transaction to be pushed even if it causes the pending block size to exceed the maximum block size.
 * Although the transaction will probably not propagate further now, as the peers are likely to have their pending
 * queues full as well, it will be kept in the queue to be propagated later when a new block flushes out the pending
 * queues.
 */
void database::push_transaction(const signed_transaction& trx, uint32_t skip)
{
    try
    {
        try
        {
            size_t trx_size = fc::raw::pack_size(trx);
            FC_ASSERT(trx_size <= (get_dynamic_global_properties().maximum_block_size - 256));
            set_producing(true);
            detail::with_skip_flags(*this, skip, [&]() { with_write_lock([&]() { _push_transaction(trx); }); });
            set_producing(false);
        }
        catch (...)
        {
            set_producing(false);
            throw;
        }
    }
    FC_CAPTURE_AND_RETHROW((trx))
}

void database::_push_transaction(const signed_transaction& trx)
{
    // If this is the first transaction pushed after applying a block, start a new undo session.
    // This allows us to quickly rewind to the clean state of the head block, in case a new block arrives.
    if (!_pending_tx_session.valid())
        _pending_tx_session = start_undo_session(true);

    // Create a temporary undo session as a child of _pending_tx_session.
    // The temporary session will be discarded by the destructor if
    // _apply_transaction fails.  If we make it to merge(), we
    // apply the changes.

    auto temp_session = start_undo_session(true);
    _apply_transaction(trx);
    _pending_tx.push_back(trx);

    notify_changed_objects();
    // The transaction applied successfully. Merge its changes into the pending block session.
    temp_session.squash();

    // notify anyone listening to pending transactions
    notify_on_pending_transaction(trx);
}

signed_block database::generate_block(fc::time_point_sec when,
                                      const account_name_type& witness_owner,
                                      const fc::ecc::private_key& block_signing_private_key,
                                      uint32_t skip /* = 0 */
)
{
    signed_block result;
    detail::with_skip_flags(*this, skip, [&]() {
        try
        {
            result = _generate_block(when, witness_owner, block_signing_private_key);
        }
        FC_CAPTURE_AND_RETHROW((witness_owner))
    });
    return result;
}

signed_block database::_generate_block(fc::time_point_sec when,
                                       const account_name_type& witness_owner,
                                       const fc::ecc::private_key& block_signing_private_key)
{
    uint32_t skip = get_node_properties().skip_flags;
    uint32_t slot_num = get_slot_at_time(when);
    FC_ASSERT(slot_num > 0);
    string scheduled_witness = get_scheduled_witness(slot_num);
    FC_ASSERT(scheduled_witness == witness_owner);

    const auto& witness_obj = get_witness(witness_owner);

    if (!(skip & skip_witness_signature))
        FC_ASSERT(witness_obj.signing_key == block_signing_private_key.get_public_key());

    static const size_t max_block_header_size = fc::raw::pack_size(signed_block_header()) + 4;
    auto maximum_block_size = get_dynamic_global_properties().maximum_block_size; // DEIP_MAX_BLOCK_SIZE;
    size_t total_block_size = max_block_header_size;

    signed_block pending_block;

    with_write_lock([&]() {
        //
        // The following code throws away existing pending_tx_session and
        // rebuilds it by re-applying pending transactions.
        //
        // This rebuild is necessary because pending transactions' validity
        // and semantics may have changed since they were received, because
        // time-based semantics are evaluated based on the current block
        // time.  These changes can only be reflected in the database when
        // the value of the "when" variable is known, which means we need to
        // re-apply pending transactions in this method.
        //
        _pending_tx_session.reset();
        _pending_tx_session = start_undo_session(true);

        uint64_t postponed_tx_count = 0;
        // pop pending state (reset to head block state)
        for (const signed_transaction& tx : _pending_tx)
        {
            // Only include transactions that have not expired yet for currently generating block,
            // this should clear problem transactions and allow block production to continue

            if (tx.expiration < when)
                continue;

            uint64_t new_total_size = total_block_size + fc::raw::pack_size(tx);

            // postpone transaction if it would make block too big
            if (new_total_size >= maximum_block_size)
            {
                postponed_tx_count++;
                continue;
            }

            try
            {
                auto temp_session = start_undo_session(true);
                _apply_transaction(tx);
                temp_session.squash();

                total_block_size += fc::raw::pack_size(tx);
                pending_block.transactions.push_back(tx);
            }
            catch (const fc::exception& e)
            {
                // Do nothing, transaction will not be re-applied
                // wlog( "Transaction was not processed while generating block due to ${e}", ("e", e) );
                // wlog( "The transaction was ${t}", ("t", tx) );
            }
        }
        if (postponed_tx_count > 0)
        {
            wlog("Postponed ${n} transactions due to block size limit", ("n", postponed_tx_count));
        }

        _pending_tx_session.reset();
    });

    // We have temporarily broken the invariant that
    // _pending_tx_session is the result of applying _pending_tx, as
    // _pending_tx now consists of the set of postponed transactions.
    // However, the push_block() call below will re-create the
    // _pending_tx_session.

    pending_block.previous = head_block_id();
    pending_block.timestamp = when;
    pending_block.transaction_merkle_root = pending_block.calculate_merkle_root();
    pending_block.witness = witness_owner;

    const auto& witness = get_witness(witness_owner);

    if (witness.running_version != DEIP_BLOCKCHAIN_VERSION)
        pending_block.extensions.insert(block_header_extensions(DEIP_BLOCKCHAIN_VERSION));

    const auto& hfp = get_hardfork_property_object();

    if (hfp.current_hardfork_version
            < DEIP_BLOCKCHAIN_HARDFORK_VERSION // Binary is newer hardfork than has been applied
        && (witness.hardfork_version_vote != _hardfork_versions[hfp.last_hardfork + 1]
            || witness.hardfork_time_vote
                != _hardfork_times[hfp.last_hardfork + 1])) // Witness vote does not match binary configuration
    {
        // Make vote match binary configuration
        pending_block.extensions.insert(block_header_extensions(
            hardfork_version_vote(_hardfork_versions[hfp.last_hardfork + 1], _hardfork_times[hfp.last_hardfork + 1])));
    }
    else if (hfp.current_hardfork_version
                 == DEIP_BLOCKCHAIN_HARDFORK_VERSION // Binary does not know of a new hardfork
             && witness.hardfork_version_vote
                 > DEIP_BLOCKCHAIN_HARDFORK_VERSION) // Voting for hardfork in the future, that we do not know of...
    {
        // Make vote match binary configuration. This is vote to not apply the new hardfork.
        pending_block.extensions.insert(block_header_extensions(
            hardfork_version_vote(_hardfork_versions[hfp.last_hardfork], _hardfork_times[hfp.last_hardfork])));
    }

    if (!(skip & skip_witness_signature))
        pending_block.sign(block_signing_private_key);

    // TODO:  Move this to _push_block() so session is restored.
    if (!(skip & skip_block_size_check))
    {
        FC_ASSERT(fc::raw::pack_size(pending_block) <= DEIP_MAX_BLOCK_SIZE);
    }

    push_block(pending_block, skip);

    return pending_block;
}

/**
 * Removes the most recent block from the database and
 * undoes any changes it made.
 */
void database::pop_block()
{
    try
    {
        _pending_tx_session.reset();
        auto head_id = head_block_id();

        /// save the head block so we can recover its transactions
        optional<signed_block> head_block = fetch_block_by_id(head_id);
        DEIP_ASSERT(head_block.valid(), pop_empty_chain, "there are no blocks to pop");

        _fork_db.pop_block();
        undo();

        _popped_tx.insert(_popped_tx.begin(), head_block->transactions.begin(), head_block->transactions.end());
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::clear_pending()
{
    try
    {
        assert((_pending_tx.size() == 0) || _pending_tx_session.valid());
        _pending_tx.clear();
        _pending_tx_session.reset();
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::notify_pre_apply_operation(operation_notification& note)
{
    note.trx_id = _current_trx_id;
    note.block = _current_block_num;
    note.trx_in_block = _current_trx_in_block;
    note.op_in_trx = _current_op_in_trx;

    DEIP_TRY_NOTIFY(pre_apply_operation, note)
}

void database::notify_post_apply_operation(const operation_notification& note)
{
    DEIP_TRY_NOTIFY(post_apply_operation, note)
}

inline void database::push_virtual_operation(const operation& op)
{
#if defined(IS_LOW_MEM) && !defined(IS_TEST_NET)
    return;
#endif

    FC_ASSERT(is_virtual_operation(op));
    operation_notification note(op);
    notify_pre_apply_operation(note);
    notify_post_apply_operation(note);
}

inline void database::push_hf_operation(const operation& op)
{
    FC_ASSERT(is_virtual_operation(op));
    operation_notification note(op);
    notify_pre_apply_operation(note);
    notify_post_apply_operation(note);
}

void database::notify_applied_block(const signed_block& block)
{
    DEIP_TRY_NOTIFY(applied_block, block)
}

void database::notify_on_pending_transaction(const signed_transaction& tx)
{
    DEIP_TRY_NOTIFY(on_pending_transaction, tx)
}

void database::notify_on_pre_apply_transaction(const signed_transaction& tx)
{
    DEIP_TRY_NOTIFY(on_pre_apply_transaction, tx)
}

void database::notify_on_applied_transaction(const signed_transaction& tx)
{
    DEIP_TRY_NOTIFY(on_applied_transaction, tx);
}

account_name_type database::get_scheduled_witness(uint32_t slot_num) const
{
    const dynamic_global_property_object& dpo = get_dynamic_global_properties();
    const witness_schedule_object& wso = get_witness_schedule_object();
    uint64_t current_aslot = dpo.current_aslot + slot_num;
    return wso.current_shuffled_witnesses[current_aslot % wso.num_scheduled_witnesses];
}

fc::time_point_sec database::get_slot_time(uint32_t slot_num) const
{
    if (slot_num == 0)
        return fc::time_point_sec();

    auto interval = DEIP_BLOCK_INTERVAL;
    const dynamic_global_property_object& dpo = get_dynamic_global_properties();

    if (head_block_num() == 0)
    {
        // n.b. first block is at genesis_time plus one block interval
        fc::time_point_sec genesis_time = dpo.time;
        return genesis_time + slot_num * interval;
    }

    int64_t head_block_abs_slot = head_block_time().sec_since_epoch() / interval;
    fc::time_point_sec head_slot_time(head_block_abs_slot * interval);

    // "slot 0" is head_slot_time
    // "slot 1" is head_slot_time,
    //   plus maint interval if head block is a maint block
    //   plus block interval if head block is not a maint block
    return head_slot_time + (slot_num * interval);
}

uint32_t database::get_slot_at_time(fc::time_point_sec when) const
{
    fc::time_point_sec first_slot_time = get_slot_time(1);
    if (when < first_slot_time)
        return 0;
    return (when - first_slot_time).to_seconds() / DEIP_BLOCK_INTERVAL + 1;
}

void database::process_common_tokens_withdrawals()
{
    dbs_account& account_service = obtain_service<dbs_account>();

    const auto& widx = get_index<account_index>().indices().get<by_next_common_tokens_withdrawal>();
    const auto& didx = get_index<withdraw_common_tokens_route_index>().indices().get<by_withdraw_route>();
    auto current = widx.begin();

    const auto& cprops = get_dynamic_global_properties();

    while (current != widx.end() && current->next_common_tokens_withdrawal <= head_block_time())
    {
        const auto& from_account = *current;
        ++current;

        /**
         *  Let T = total tokens in common_tokens fund
         *  Let V = total common_tokens
         *  Let v = total common_tokens being cashed out
         *
         *  The user may withdraw  vT / V tokens
         */
        share_type to_withdraw;
        if (from_account.to_withdraw - from_account.withdrawn < from_account.common_tokens_withdraw_rate)
            to_withdraw = std::min(from_account.common_tokens_balance,
                                   from_account.to_withdraw % from_account.common_tokens_withdraw_rate)
                              .value;
        else
            to_withdraw = std::min(from_account.common_tokens_balance, from_account.common_tokens_withdraw_rate).value;

        share_type common_tokens_deposited_as_deip = 0;
        share_type common_tokens_deposited_as_common_tokens = 0;
        asset total_deip_converted = asset(0, DEIP_SYMBOL);

        // Do two passes, the first for common tokens, the second for deip. Try to maintain as much accuracy for common tokens as
        // possible.
        for (auto itr = didx.upper_bound(boost::make_tuple(from_account.id, account_id_type()));
             itr != didx.end() && itr->from_account == from_account.id; ++itr)
        {
            if (itr->auto_common_token)
            {
                share_type to_deposit
                    = ((fc::uint128_t(to_withdraw.value) * itr->percent) / DEIP_100_PERCENT).to_uint64();
                common_tokens_deposited_as_common_tokens += to_deposit;

                if (to_deposit > 0)
                {
                    const auto& to_account = get(itr->to_account);

                    modify(to_account, [&](account_object& a) { a.common_tokens_balance += to_deposit; });

                    account_service.adjust_proxied_witness_votes(to_account, to_deposit);

                    push_virtual_operation(fill_common_tokens_withdraw_operation(from_account.name, to_account.name,
                                                                           to_deposit,
                                                                           to_deposit,
                                                                           true));
                }
            }
        }

        for (auto itr = didx.upper_bound(boost::make_tuple(from_account.id, account_id_type()));
             itr != didx.end() && itr->from_account == from_account.id; ++itr)
        {
            if (!itr->auto_common_token)
            {
                const auto& to_account = get(itr->to_account);

                share_type to_deposit
                    = ((fc::uint128_t(to_withdraw.value) * itr->percent) / DEIP_100_PERCENT).to_uint64();
                common_tokens_deposited_as_deip += to_deposit;
                share_type converted_deip = to_deposit;
                total_deip_converted += converted_deip;

                if (to_deposit > 0)
                {
                    modify(to_account, [&](account_object& a) { a.balance += converted_deip; });

                    modify(cprops, [&](dynamic_global_property_object& o) {
                        o.common_tokens_fund -= converted_deip;
                        o.total_common_tokens_amount -= to_deposit;
                    });

                    push_virtual_operation(fill_common_tokens_withdraw_operation(
                        from_account.name, to_account.name, to_deposit, converted_deip, false));
                }
            }
        }

        share_type to_convert = to_withdraw - common_tokens_deposited_as_deip - common_tokens_deposited_as_common_tokens;
        FC_ASSERT(to_convert >= 0, "Deposited more common_tokens than were supposed to be withdrawn");

        share_type converted_deip = to_convert;

        modify(from_account, [&](account_object& a) {
            a.common_tokens_balance -= to_withdraw;
            a.balance += converted_deip;
            a.withdrawn += to_withdraw;

            if (a.withdrawn >= a.to_withdraw || a.common_tokens_balance == 0)
            {
                a.common_tokens_withdraw_rate= 0;
                a.next_common_tokens_withdrawal = fc::time_point_sec::maximum();
            }
            else
            {
                a.next_common_tokens_withdrawal += fc::seconds(DEIP_COMMON_TOKENS_WITHDRAW_INTERVAL_SECONDS);
            }
        });

        modify(cprops, [&](dynamic_global_property_object& o) {
            o.common_tokens_fund -= converted_deip;
            o.total_common_tokens_amount -= to_convert;
        });

        if (to_withdraw > 0)
            account_service.adjust_proxied_witness_votes(from_account, -to_withdraw);

        push_virtual_operation(fill_common_tokens_withdraw_operation(from_account.name, from_account.name,
                                                                                        to_withdraw,
                                                                                        converted_deip,
                                                                                        false));
    }
}

// TODO: Update information without VESTS
/**
 *  Overall the network has an inflation rate of 102% of virtual deip per year
 *  90% of inflation is directed to vesting shares
 *  10% of inflation is directed to subjective proof of work voting
 *  1% of inflation is directed to liquidity providers
 *  1% of inflation is directed to block producers
 *
 *  This method pays out vesting and reward shares every block, and liquidity shares once per day.
 *  This method does not pay out witnesses.
 */
void database::process_funds()
{
    dbs_account& account_service = obtain_service<dbs_account>();

    const auto& props = get_dynamic_global_properties();
    const auto& wso = get_witness_schedule_object();

    // DEIP: compare to our inflation rate strategy

    /**
     * At block 7,000,000 have a 9.5% instantaneous inflation rate, decreasing to 0.95% at a rate of 0.01%
     * every 250k blocks. This narrowing will take approximately 20.5 years and will complete on block 220,750,000
     */
    int64_t start_inflation_rate = int64_t(DEIP_INFLATION_RATE_START_PERCENT);
    int64_t inflation_rate_adjustment = int64_t(head_block_num() / DEIP_INFLATION_NARROWING_PERIOD);
    int64_t inflation_rate_floor = int64_t(DEIP_INFLATION_RATE_STOP_PERCENT);

    // below subtraction cannot underflow int64_t because inflation_rate_adjustment is <2^32
    int64_t current_inflation_rate = std::max(start_inflation_rate - inflation_rate_adjustment, inflation_rate_floor);

    auto new_deip = (props.current_supply.amount * current_inflation_rate) / (int64_t(DEIP_100_PERCENT) * int64_t(DEIP_BLOCKS_PER_YEAR));
    // TODO: Expertise adaptive emission model
    auto new_expertise = new_deip;

    auto contribution_reward = util::calculate_share(new_deip, DEIP_CONTRIBUTION_REWARD_PERCENT); /// 97% to contribution rewards
    auto witness_reward = new_deip - contribution_reward; /// Remaining 3% to witness pay

    const auto& cwit = get_witness(props.current_witness);
    witness_reward *= DEIP_MAX_WITNESSES;

    if (cwit.schedule == witness_object::timeshare)
        witness_reward *= wso.timeshare_weight;
    else if (cwit.schedule == witness_object::top20)
        witness_reward *= wso.top20_weight;
    else
        wlog("Encountered unknown witness type for witness: ${w}", ("w", cwit.owner));

    witness_reward /= wso.witness_pay_normalization_factor;

    contribution_reward = distribute_reward(contribution_reward, new_expertise);

    new_deip = contribution_reward + witness_reward;

    modify(props, [&](dynamic_global_property_object& p) {
        p.current_supply += asset(new_deip, DEIP_SYMBOL);
    });

    account_service.increase_common_tokens(get_account(cwit.owner), witness_reward);

    // witness_reward = producer_reward because 1 DEIP = 1 Common Token. Add producer_reward as separate value if 1 DEIP != 1 Common Token
    push_virtual_operation(producer_reward_operation(cwit.owner, witness_reward));
}

void database::account_recovery_processing()
{
    // Clear expired recovery requests
    const auto& rec_req_idx = get_index<account_recovery_request_index>().indices().get<by_expiration>();
    auto rec_req = rec_req_idx.begin();

    while (rec_req != rec_req_idx.end() && rec_req->expires <= head_block_time())
    {
        remove(*rec_req);
        rec_req = rec_req_idx.begin();
    }

    // Clear invalid historical authorities
    const auto& hist_idx = get_index<owner_authority_history_index>().indices(); // by id
    auto hist = hist_idx.begin();

    while (hist != hist_idx.end()
           && time_point_sec(hist->last_valid_time + DEIP_OWNER_AUTH_RECOVERY_PERIOD) < head_block_time())
    {
        remove(*hist);
        hist = hist_idx.begin();
    }

    // Apply effective recovery_account changes
    const auto& change_req_idx = get_index<change_recovery_account_request_index>().indices().get<by_effective_date>();
    auto change_req = change_req_idx.begin();

    while (change_req != change_req_idx.end() && change_req->effective_on <= head_block_time())
    {
        modify(get_account(change_req->account_to_recover),
               [&](account_object& a) { a.recovery_account = change_req->recovery_account; });

        remove(*change_req);
        change_req = change_req_idx.begin();
    }
}

void database::distribute_research_tokens(const research_token_sale_id_type& research_token_sale_id)
{
    dbs_research_token_sale& research_token_sale_service = obtain_service<dbs_research_token_sale>();
    dbs_research_token& research_token_service = obtain_service<dbs_research_token>();

    auto& research_token_sale = research_token_sale_service.get_by_id(research_token_sale_id);

    const auto& idx = get_index<research_token_sale_contribution_index>().indicies().get<by_research_token_sale_id>();
    auto it = idx.find(research_token_sale_id);

    while (it != idx.end())
    {
        auto transfer_amount = (it->amount.amount * research_token_sale.balance_tokens) / research_token_sale.total_amount.amount;

        if (research_token_service.exists_by_owner_and_research(it->owner, research_token_sale.research_id))
        {
            auto& research_token = research_token_service.get_by_owner_and_research(it->owner, research_token_sale.research_id);
            research_token_service.increase_research_token_amount(research_token, transfer_amount);
        }
        else
        {
            research_token_service.create_research_token(it->owner, transfer_amount, research_token_sale.research_id);
        }
        remove(*it);
        ++it;
    }
}

void database::refund_research_tokens(const research_token_sale_id_type research_token_sale_id)
{
    dbs_account& account_service = obtain_service<dbs_account>();
    dbs_research& research_service = obtain_service<dbs_research>();
    dbs_research_token_sale& research_token_sale_service = obtain_service<dbs_research_token_sale>();

    auto& research_token_sale = research_token_sale_service.get_by_id(research_token_sale_id);

    const auto& idx = get_index<research_token_sale_contribution_index>().indicies().
            get<by_research_token_sale_id>().equal_range(research_token_sale_id);

    auto it = idx.first;
    const auto it_end = idx.second;

    while (it != it_end)
    {
        account_service.adjust_balance(account_service.get_account(it->owner), it->amount);
        remove(*it);
        it = idx.first;
    }

    auto& research = research_service.get_research(research_token_sale.research_id);
    modify(research, [&](research_object& r_o) { r_o.owned_tokens += research_token_sale.balance_tokens; });
}

share_type database::distribute_reward(const share_type &reward, const share_type &expertise)
{
    auto& discipline_service = obtain_service<dbs_discipline>();
    auto disciplines = discipline_service.get_disciplines();

    share_type total_disciplines_weight = share_type(0);

    for (uint32_t i = 0; i < disciplines.size(); i++) {
        auto& discipline = disciplines.at(i).get();
        total_disciplines_weight += discipline.total_active_weight;
    }

    // Distribute among all disciplines pools
    share_type used_reward = 0;

    for (auto& discipline_ref : disciplines) {
        const auto& discipline = discipline_ref.get();
        if (discipline.total_active_weight != 0)
        {
            // Distribute among disciplines in all disciplines pool
            auto discipline_reward_share = util::calculate_share(reward, discipline.total_active_weight, total_disciplines_weight);
            auto discipline_expertise_share = util::calculate_share(expertise, discipline.total_active_weight, total_disciplines_weight);
            // TODO: Adjustable review reward pool share
            auto discipline_review_reward_pool_share = util::calculate_share(discipline_reward_share, DEIP_REVIEW_REWARD_POOL_SHARE_PERCENT);
            discipline_reward_share -= discipline_review_reward_pool_share;
            used_reward += reward_researches_in_discipline(discipline, discipline_reward_share, discipline_expertise_share);
            used_reward += fund_review_pool(discipline, discipline_review_reward_pool_share);
        }
    }

    FC_ASSERT(used_reward <= reward, "Attempt to distribute amount that is greater than reward amount");

    return used_reward;
}

share_type database::reward_researches_in_discipline(const discipline_object &discipline,
                                                     const share_type &reward,
                                                     const share_type &expertise)
{
    FC_ASSERT(discipline.total_active_weight != 0, "Attempt to reward research in inactive discipline");

    auto& content_service = obtain_service<dbs_research_content>();
    dbs_research_content_reward_pool& research_reward_pool_service = obtain_service<dbs_research_content_reward_pool>();

    const auto& total_votes_idx = get_index<total_votes_index>().indices().get<by_discipline_id>();
    auto total_votes_itr = total_votes_idx.find(discipline.id);

    share_type used_reward = 0;

    while (total_votes_itr != total_votes_idx.end())
    {
        auto content_id = total_votes_itr->research_content_id;
        auto& content = content_service.get(content_id);
        if (content.activity_state == research_content_activity_state::active)
        {
            auto weight = std::max(int64_t(0), total_votes_itr->total_weight.value);
            auto reward_share = util::calculate_share(reward, weight, discipline.total_active_weight);
            auto expertise_share = util::calculate_share(expertise, weight,discipline.total_active_weight);

            if(research_reward_pool_service.is_research_reward_pool_exists_by_research_content_id_and_discipline_id(content_id, discipline.id))
            {
                auto& research_reward_pool = research_reward_pool_service.get_by_research_content_id_and_discipline_id(content_id, discipline.id);

                research_reward_pool_service.increase_reward_pool(research_reward_pool, reward_share);
                research_reward_pool_service.increase_expertise_pool(research_reward_pool, expertise_share);

                used_reward += reward_share;
            }
            else
            {
                research_reward_pool_service.create(content_id, discipline.id, reward_share, expertise_share);
            }
        }
        ++total_votes_itr;
    }

    FC_ASSERT(used_reward <= reward, "Attempt to allocate funds amount that is greater than reward amount");

    return used_reward;
}

share_type database::reward_research_content(const research_content_id_type &research_content_id,
                                             const discipline_id_type &discipline_id,
                                             const share_type &reward,
                                             const share_type &expertise)
{
    auto& research_content_service = obtain_service<dbs_research_content>();
    auto& research_service = obtain_service<dbs_research>();

    auto& research_content = research_content_service.get(research_content_id);
    auto& research = research_service.get_research(research_content.research_id);

    auto references_share = util::calculate_share(reward, DEIP_REFERENCES_REWARD_SHARE_PERCENT);
    auto review_share = util::calculate_share(reward, research.review_share_in_percent);
    auto token_holders_share = reward - references_share - review_share;

    auto review_expertise_share = util::calculate_share(expertise, research.review_share_in_percent);
    auto authors_expertise_share = expertise - review_expertise_share;

    FC_ASSERT(token_holders_share + review_share + references_share <= reward,
              "Attempt to allocate funds amount that is greater than reward amount");

    share_type used_reward = 0;

    reward_research_authors_with_expertise(research, research_content, discipline_id, authors_expertise_share);

    used_reward += reward_research_token_holders(research, discipline_id, token_holders_share);
    used_reward += reward_references(research_content_id, discipline_id, references_share);
    used_reward += reward_reviews(research_content_id, discipline_id, review_share, review_expertise_share);

    FC_ASSERT(used_reward <= reward, "Attempt to allocate funds amount that is greater than reward amount");

    return used_reward;
}

share_type database::reward_research_token_holders(const research_object& research,
                                            const discipline_id_type& discipline_id,
                                            const share_type& reward)
{
    dbs_account& account_service = obtain_service<dbs_account>();

    auto research_group_reward = (research.owned_tokens * reward) / DEIP_100_PERCENT;
    share_type used_reward = 0;

    if(research_group_reward > 0)
    {
        dbs_research_group& research_group_service = obtain_service<dbs_research_group>();
        research_group_service.increase_balance(research.research_group_id, asset(research_group_reward, DEIP_SYMBOL));
        used_reward += research_group_reward;
    }

    const auto& idx = get_index<research_token_index>().indicies().get<by_research_id>().equal_range(research.id);

    auto it = idx.first;
    const auto it_end = idx.second;

    while (it != it_end)
    {
        auto reward_amount = (it->amount * reward) / DEIP_100_PERCENT;
        auto& account = account_service.get_account(it->account_name);
        account_service.adjust_balance(account, asset(reward_amount, DEIP_SYMBOL));
        used_reward += reward_amount;
        ++it;
    }

    FC_ASSERT(used_reward <= reward, "Attempt to allocate funds amount that is greater than reward amount");

    return used_reward;
}

share_type database::reward_references(const research_content_id_type &research_content_id,
                                       const discipline_id_type &discipline_id, const share_type &reward)
{
    dbs_research& research_service = obtain_service<dbs_research>();
    dbs_research_content& research_content_service = obtain_service<dbs_research_content>();
    auto& research_content = research_content_service.get(research_content_id);

    share_type used_reward = 0;
    share_type total_votes_amount = 0;

    for (auto content_id : research_content.references)
    {
        const auto& idx = get_index<total_votes_index>().indicies().get<by_content_and_discipline>();
        auto total_votes_itr = idx.find(std::make_tuple(content_id, discipline_id));
        if (total_votes_itr != idx.end()) {
            total_votes_amount += total_votes_itr->total_weight;
        }
    }

    for (auto content_id : research_content.references)
    {
        const auto& idx = get_index<total_votes_index>().indicies().get<by_content_and_discipline>();
        auto total_votes_itr = idx.find(std::make_tuple(content_id, discipline_id));
        if (total_votes_itr != idx.end()) {
            auto& research = research_service.get_research(total_votes_itr->research_id);

            auto reward_share = util::calculate_share(reward, total_votes_itr->total_weight, total_votes_amount);
            used_reward += reward_research_token_holders(research, discipline_id, reward_share);
        }
    }

    FC_ASSERT(used_reward <= reward, "Attempt to allocate funds amount that is greater than reward amount");

    return used_reward;
}

share_type database::reward_reviews(const research_content_id_type &research_content_id,
                                    const discipline_id_type &discipline_id,
                                    const share_type &reward,
                                    const share_type &expertise_reward)
{
    std::vector<review_object> reviews;

    share_type used_reward = 0;

    auto it_pair = get_index<review_index>().indicies().get<by_research_content>().equal_range(research_content_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        reviews.push_back(*it);
        ++it;
    }

    used_reward = allocate_rewards_to_reviews(reviews, discipline_id, reward, expertise_reward);

    FC_ASSERT(used_reward <= reward, "Attempt to allocate funds amount that is greater than reward amount");

    return used_reward;
}

share_type database::reward_review_voters(const review_object &review,
                                          const discipline_id_type &discipline_id,
                                          const share_type &reward)
{
    dbs_account& account_service = obtain_service<dbs_account>();
    dbs_vote& vote_service = obtain_service<dbs_vote>();

    auto votes_wrapper = vote_service.get_review_votes_by_review_and_discipline(review.id, discipline_id);

    share_type used_reward = 0;
    share_type total_weight = review.weights_per_discipline.at(discipline_id);

    if (total_weight == 0) return 0;

    for (auto& vote_ref : votes_wrapper) {
        auto vote = vote_ref.get();
        auto& voter = account_service.get_account(vote.voter);
        auto reward_amount = util::calculate_share(reward, vote.weight, total_weight);
        account_service.adjust_balance(voter, asset(reward_amount, DEIP_SYMBOL));
        used_reward += reward_amount;
    }

    FC_ASSERT(used_reward <= reward, "Attempt to allocate funds amount that is greater than reward amount");

    return used_reward;
}

void database::reward_account_with_expertise(const account_name_type &account, const discipline_id_type &discipline_id,
                                             const share_type &reward)
{
    if (reward > 0)
    {
        const auto &expert_tokens_idx = get_index<expert_token_index>().indices().get<by_account_and_discipline>();
        auto expert_tokens_itr = expert_tokens_idx.find(std::make_tuple(account, discipline_id));
        if (expert_tokens_itr != expert_tokens_idx.end()) {
            auto &expert_token = *expert_tokens_itr;
            modify(expert_token, [&](expert_token_object &t) {
                t.amount += reward;
            });
        } else {
            dbs_expert_token &expert_token_service = obtain_service<dbs_expert_token>();
            expert_token_service.create(account, discipline_id, reward);
        }
    }
}

void database::reward_research_authors_with_expertise(const research_object &research,
                                                      const research_content_object &research_content,
                                                      const discipline_id_type &discipline_id,
                                                      const share_type &expertise_reward)
{
    const auto& research_group_service = obtain_service<dbs_research_group>();

    share_type used_reward = 0;

    flat_set<account_name_type> authors;

    switch (research_content.type) {
        case research_content_type::announcement:
        case research_content_type::final_result:
            authors = research_group_service.get_members(research.research_group_id);
            break;
        default:
            authors.insert(research_content.authors.begin(), research_content.authors.end());
            break;
    }

    share_type total_tokens_amount = share_type(0);
    std::vector<research_group_token_object> tokens;
    for (auto& author : authors) {
        auto& token = research_group_service.get_token_by_account_and_research_group(author, research.research_group_id);
        tokens.push_back(token);

        total_tokens_amount += token.amount;
    }

    for (auto& token : tokens) {
        auto expertise_amount = (expertise_reward * token.amount) / total_tokens_amount;
        reward_account_with_expertise(token.owner, discipline_id, expertise_amount);
        used_reward += expertise_amount;
    }

    FC_ASSERT(used_reward <= expertise_reward, "Attempt to allocate expertise reward amount that is greater than reward amount");
}


share_type database::fund_review_pool(const discipline_object& discipline, const share_type& amount)
{
    auto& review_service = obtain_service<dbs_review>();

    flat_set<review_id_type> reviews_ids;
    std::vector<review_object> reviews;

    share_type used_reward = 0;

    auto it_pair = get_index<review_vote_index>().indicies().get<by_discipline_id>().equal_range(discipline.id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        auto id = it->review_id;
        if (reviews_ids.count(id) == 0)
        {
            reviews_ids.insert(id);

            auto& review = review_service.get(id);
            reviews.push_back(review);
        }
        ++it;
    }

    used_reward = allocate_rewards_to_reviews(reviews, discipline.id, amount, 0);

    return used_reward;
}

share_type database::allocate_rewards_to_reviews(const std::vector<review_object> &reviews, const discipline_id_type &discipline_id,
                                                 const share_type &reward, const share_type &expertise_reward)
{
    dbs_account& account_service = obtain_service<dbs_account>();

    share_type total_reviews_weight = share_type(0);

    for (uint32_t i = 0; i < reviews.size(); i++) {
        total_reviews_weight += reviews.at(i).weights_per_discipline.at(discipline_id);
    }

    if (total_reviews_weight == 0) return 0;

    share_type used_reward = 0;

    for (auto& review : reviews) {
        auto review_reward_share = util::calculate_share(reward, review.weights_per_discipline.at(discipline_id),
                                                         total_reviews_weight);
        auto author_name = review.author;
        auto review_curators_reward_share = util::calculate_share(review_reward_share,
                                                                      DEIP_CURATORS_REWARD_SHARE_PERCENT);

        auto author_reward = review_reward_share - review_curators_reward_share;

        // Reward author
        auto& author = account_service.get_account(author_name);
        account_service.adjust_balance(author, asset(author_reward, DEIP_SYMBOL));
        used_reward += author_reward;

        used_reward += reward_review_voters(review, discipline_id, review_curators_reward_share);

        // reward expertise
        if (expertise_reward != 0) {
            auto author_expertise = util::calculate_share(expertise_reward, review.weights_per_discipline.at(discipline_id),
                                                          total_reviews_weight);
            reward_account_with_expertise(author_name, discipline_id, author_expertise);
        }
    }

    return used_reward;
}

share_type database::grant_researches_in_discipline(const discipline_id_type& discipline_id, const share_type &grant)
{
    dbs_discipline& discipline_service = obtain_service<dbs_discipline>();
    dbs_research_content& research_content_service = obtain_service<dbs_research_content>();
    auto& research_service = obtain_service<dbs_research>();
    auto& research_group_service = obtain_service<dbs_research_group>();

    const auto& discipline = discipline_service.get_discipline(discipline_id);
    if(discipline.total_active_weight == 0) return 0;

    share_type used_grant = 0;
    share_type total_research_weight = discipline.total_active_weight;

    std::map<research_group_id_type, share_type> grant_shares_per_research;

    // Exclude final results from share calculation and grant distribution
    const auto& final_results_idx = get_index<total_votes_index>().indices().get<by_discipline_and_content_type>();
    auto final_results_itr = final_results_idx.find(std::make_tuple(discipline.id, research_content_type::final_result));
    while (final_results_itr != final_results_idx.end())
    {
        const auto& final_result = research_content_service.get(final_results_itr->research_content_id);
        if (final_result.activity_state == research_content_activity_state::active) {
            total_research_weight -= final_results_itr->total_weight;
        }
        ++final_results_itr;
    }

    const auto& total_votes_idx = get_index<total_votes_index>().indices().get<by_discipline_id>();
    auto total_votes_itr = total_votes_idx.find(discipline.id);
    while (total_votes_itr != total_votes_idx.end())
    {
        const auto& research_content = research_content_service.get(total_votes_itr->research_content_id);

        if (research_content.type != research_content_type::final_result
            && research_content.activity_state == research_content_activity_state::active
            && total_votes_itr->total_weight != 0) {
                auto share = util::calculate_share(grant, total_votes_itr->total_weight, total_research_weight);
                auto& research = research_service.get_research(total_votes_itr->research_id);
                research_group_service.increase_balance(research.research_group_id, asset(share, DEIP_SYMBOL));
                used_grant += share;
            }


        ++total_votes_itr;

    }

    FC_ASSERT(used_grant <= grant, "Attempt to allocate grant amount that is greater than grant");

    return used_grant;
}

void database::process_grants()
{
    uint32_t block_num = head_block_num();

    dbs_grant& grant_service = obtain_service<dbs_grant>();

    const auto& grants_idx = get_index<grant_index>().indices().get<by_start_block>();
    auto grants_itr = grants_idx.lower_bound(block_num);

    while (grants_itr != grants_idx.end())
    {
        auto& grant = *grants_itr;
        auto used_grant = grant_researches_in_discipline(grant.target_discipline, grant.per_block);

        if (used_grant == 0 && grant.is_extendable)
            modify(grant, [&](grant_object& g_o) { g_o.end_block++;} );
        else if (used_grant != 0)
            grant_service.allocate_funds(grant);

        ++grants_itr;
    }
}


void database::process_research_token_sales()
{
    dbs_research_token_sale& research_token_sale_service = obtain_service<dbs_research_token_sale>();
    const auto& idx = get_index<research_token_sale_index>().indices().get<by_end_time>();
    auto itr = idx.begin();
    auto _head_block_time = head_block_time();

    while (itr != idx.end())
    {
        if (itr->end_time <= _head_block_time && itr->status == research_token_sale_status::token_sale_active) {
            if (itr->total_amount < itr->soft_cap) {
                research_token_sale_service.update_status(itr->id, research_token_sale_status::token_sale_expired);
                refund_research_tokens(itr->id);
            } else if (itr->total_amount >= itr->soft_cap) {
                research_token_sale_service.update_status(itr->id, research_token_sale_status::token_sale_finished);
                distribute_research_tokens(itr->id);
            }
        } else if (itr->end_time > _head_block_time) {
            if (_head_block_time >= itr->start_time && itr->status == research_token_sale_status::token_sale_inactive) {
                research_token_sale_service.update_status(itr->id, research_token_sale_status::token_sale_active);
            }
        }
        itr++;
    }
}

time_point_sec database::head_block_time() const
{
    return get_dynamic_global_properties().time;
}

uint32_t database::head_block_num() const
{
    return get_dynamic_global_properties().head_block_number;
}

block_id_type database::head_block_id() const
{
    return get_dynamic_global_properties().head_block_id;
}

node_property_object& database::node_properties()
{
    return _node_property_object;
}

uint32_t database::last_non_undoable_block_num() const
{
    return get_dynamic_global_properties().last_irreversible_block_num;
}

void database::initialize_evaluators()
{
    _my->_evaluator_registry.register_evaluator<transfer_evaluator>();
    _my->_evaluator_registry.register_evaluator<transfer_to_common_tokens_evaluator>();
    _my->_evaluator_registry.register_evaluator<withdraw_common_tokens_evaluator>();
    _my->_evaluator_registry.register_evaluator<set_withdraw_common_tokens_route_evaluator>();
    _my->_evaluator_registry.register_evaluator<account_create_evaluator>();
    _my->_evaluator_registry.register_evaluator<account_update_evaluator>();
    _my->_evaluator_registry.register_evaluator<witness_update_evaluator>();
    _my->_evaluator_registry.register_evaluator<account_witness_vote_evaluator>();
    _my->_evaluator_registry.register_evaluator<account_witness_proxy_evaluator>();
    _my->_evaluator_registry.register_evaluator<request_account_recovery_evaluator>();
    _my->_evaluator_registry.register_evaluator<recover_account_evaluator>();
    _my->_evaluator_registry.register_evaluator<change_recovery_account_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_research_group_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_proposal_evaluator>();
    _my->_evaluator_registry.register_evaluator<make_review_evaluator>();
    _my->_evaluator_registry.register_evaluator<contribute_to_token_sale_evaluator>();
    _my->_evaluator_registry.register_evaluator<approve_research_group_invite_evaluator>();
    _my->_evaluator_registry.register_evaluator<reject_research_group_invite_evaluator>();
    _my->_evaluator_registry.register_evaluator<vote_for_review_evaluator>();
    _my->_evaluator_registry.register_evaluator<transfer_research_tokens_to_research_group_evaluator>();
    _my->_evaluator_registry.register_evaluator<set_expertise_tokens_evaluator>();
    _my->_evaluator_registry.register_evaluator<research_update_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_vesting_balance_evaluator>();
    _my->_evaluator_registry.register_evaluator<withdraw_vesting_balance_evaluator>();
    _my->_evaluator_registry.register_evaluator<vote_proposal_evaluator>();
    _my->_evaluator_registry.register_evaluator<transfer_research_tokens_evaluator>();
    _my->_evaluator_registry.register_evaluator<delegate_expertise_evaluator>();
    _my->_evaluator_registry.register_evaluator<revoke_expertise_delegation_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_grant_evaluator>();
}

void database::initialize_indexes()
{
    add_index<dynamic_global_property_index>();
    add_index<chain_property_index>();
    add_index<account_index>();
    add_index<account_authority_index>();
    add_index<witness_index>();
    add_index<transaction_index>();
    add_index<block_summary_index>();
    add_index<witness_schedule_index>();
    add_index<witness_vote_index>();
    add_index<operation_index>();
    add_index<hardfork_property_index>();
    add_index<withdraw_common_tokens_route_index>();
    add_index<owner_authority_history_index>();
    add_index<account_recovery_request_index>();
    add_index<change_recovery_account_request_index>();
    add_index<reward_fund_index>();
    add_index<grant_index>();
    add_index<proposal_index>();
    add_index<proposal_vote_index>();
    add_index<research_group_index>();
    add_index<research_group_token_index>();
    add_index<discipline_index>();
    add_index<research_discipline_relation_index>();
    add_index<research_index>();
    add_index<research_content_index>();
    add_index<expert_token_index>();
    add_index<research_token_index>();
    add_index<research_token_sale_index>();
    add_index<research_token_sale_contribution_index>();
    add_index<vote_index>();
    add_index<total_votes_index>();
    add_index<research_group_invite_index>();
    add_index<review_index>();
    add_index<review_vote_index>();
    add_index<vesting_balance_index>();
    add_index<research_content_reward_pool_index>();
    add_index<expertise_stats_index>();

    _plugin_index_signal();
}

void database::validate_transaction(const signed_transaction& trx)
{
    database::with_write_lock([&]() {
        auto session = start_undo_session(true);
        _apply_transaction(trx);
        session.undo();
    });
}

void database::notify_changed_objects()
{
    try
    {
        /*vector< graphene::chainbase::generic_id > ids;
        get_changed_ids( ids );
        DEIP_TRY_NOTIFY( changed_objects, ids )*/
        /*
        if( _undo_db.enabled() )
        {
           const auto& head_undo = _undo_db.head();
           vector<object_id_type> changed_ids;  changed_ids.reserve(head_undo.old_values.size());
           for( const auto& item : head_undo.old_values ) changed_ids.push_back(item.first);
           for( const auto& item : head_undo.new_ids ) changed_ids.push_back(item);
           vector<const object*> removed;
           removed.reserve( head_undo.removed.size() );
           for( const auto& item : head_undo.removed )
           {
              changed_ids.push_back( item.first );
              removed.emplace_back( item.second.get() );
           }
           DEIP_TRY_NOTIFY( changed_objects, changed_ids )
        }
        */
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::set_flush_interval(uint32_t flush_blocks)
{
    _flush_blocks = flush_blocks;
    _next_flush_block = 0;
}

//////////////////// private methods ////////////////////

void database::apply_block(const signed_block& next_block, uint32_t skip)
{
    try
    {
        // fc::time_point begin_time = fc::time_point::now();

        auto block_num = next_block.block_num();
        if (_checkpoints.size() && _checkpoints.rbegin()->second != block_id_type())
        {
            auto itr = _checkpoints.find(block_num);
            if (itr != _checkpoints.end())
                FC_ASSERT(next_block.id() == itr->second, "Block did not match checkpoint",
                          ("checkpoint", *itr)("block_id", next_block.id()));

            if (_checkpoints.rbegin()->first >= block_num)
                skip = skip_witness_signature | skip_transaction_signatures | skip_transaction_dupe_check | skip_fork_db
                    | skip_block_size_check | skip_tapos_check
                    | skip_authority_check
                    /* | skip_merkle_check While blockchain is being downloaded, txs need to be validated against block
                       headers */
                    | skip_undo_history_check | skip_witness_schedule_check | skip_validate | skip_validate_invariants;
        }

        detail::with_skip_flags(*this, skip, [&]() { _apply_block(next_block); });

        /*try
        {
        /// check invariants
        if( is_producing() || !( skip & skip_validate_invariants ) )
           validate_invariants();
        }
        FC_CAPTURE_AND_RETHROW( (next_block) );*/

        // fc::time_point end_time = fc::time_point::now();
        // fc::microseconds dt = end_time - begin_time;
        if (_flush_blocks != 0)
        {
            if (_next_flush_block == 0)
            {
                uint32_t lep = block_num + 1 + _flush_blocks * 9 / 10;
                uint32_t rep = block_num + 1 + _flush_blocks;

                // use time_point::now() as RNG source to pick block randomly between lep and rep
                uint32_t span = rep - lep;
                uint32_t x = lep;
                if (span > 0)
                {
                    uint64_t now = uint64_t(fc::time_point::now().time_since_epoch().count());
                    x += now % span;
                }
                _next_flush_block = x;
                // ilog( "Next flush scheduled at block ${b}", ("b", x) );
            }

            if (_next_flush_block == block_num)
            {
                _next_flush_block = 0;
                // ilog( "Flushing database shared memory at block ${b}", ("b", block_num) );
                chainbase::database::flush();
            }
        }

        show_free_memory(false);
    }
    FC_CAPTURE_AND_RETHROW((next_block))
}

void database::show_free_memory(bool force)
{
#ifdef IS_TEST_NET
    boost::ignore_unused(force);
#else
    uint32_t free_gb = uint32_t(get_free_memory() / (1024 * 1024 * 1024));
    if (force || (free_gb < _last_free_gb_printed) || (free_gb > _last_free_gb_printed + 1))
    {
        ilog("Free memory is now ${n}G", ("n", free_gb));
        _last_free_gb_printed = free_gb;
    }

    if (free_gb == 0)
    {
        uint32_t free_mb = uint32_t(get_free_memory() / (1024 * 1024));

        if (free_mb <= 100 && head_block_num() % 10 == 0)
            elog("Free memory is now ${n}M. Increase shared file size immediately!", ("n", free_mb));
    }
#endif
}

void database::_apply_block(const signed_block& next_block)
{
    try
    {
        uint32_t next_block_num = next_block.block_num();
        // block_id_type next_block_id = next_block.id();

        uint32_t skip = get_node_properties().skip_flags;

        if (!(skip & skip_merkle_check))
        {
            auto merkle_root = next_block.calculate_merkle_root();

            try
            {
                FC_ASSERT(next_block.transaction_merkle_root == merkle_root, "Merkle check failed",
                          ("next_block.transaction_merkle_root", next_block.transaction_merkle_root)(
                              "calc", merkle_root)("next_block", next_block)("id", next_block.id()));
            }
            catch (fc::assert_exception& e)
            {
                const auto& merkle_map = get_shared_db_merkle();
                auto itr = merkle_map.find(next_block_num);

                if (itr == merkle_map.end() || itr->second != merkle_root)
                    throw e;
            }
        }
        auto& expertise_stats_service = obtain_service<dbs_expertise_stats>();

        const witness_object& signing_witness = validate_block_header(skip, next_block);

        _current_block_num = next_block_num;
        _current_trx_in_block = 0;

        const auto& gprops = get_dynamic_global_properties();
        auto block_size = fc::raw::pack_size(next_block);
        FC_ASSERT(block_size <= gprops.maximum_block_size, "Block Size is too Big",
                  ("next_block_num", next_block_num)("block_size", block_size)("max", gprops.maximum_block_size));

        /// modify current witness so transaction evaluators can know who included the transaction,
        /// this is mostly for POW operations which must pay the current_witness
        modify(gprops, [&](dynamic_global_property_object& dgp) { dgp.current_witness = next_block.witness; });

        /// modify expertise stats to correctly calculate emission
        auto& stats = get_expertise_stats();
        modify(stats, [&](expertise_stats_object& s) {
            s.used_expertise_last_week.push_front(s.used_expertise_per_block);
            if (s.used_expertise_last_week.size() > DEIP_BLOCKS_PER_WEEK) {
                s.used_expertise_last_week.pop_back();
            }
        });

        /// parse witness version reporting
        process_header_extensions(next_block);

        const auto& witness = get_witness(next_block.witness);
        const auto& hardfork_state = get_hardfork_property_object();
        FC_ASSERT(witness.running_version >= hardfork_state.current_hardfork_version,
                  "Block produced by witness that is not running current hardfork",
                  ("witness", witness)("next_block.witness", next_block.witness)("hardfork_state", hardfork_state));

        for (const auto& trx : next_block.transactions)
        {
            /* We do not need to push the undo state for each transaction
             * because they either all apply and are valid or the
             * entire block fails to apply.  We only need an "undo" state
             * for transactions when validating broadcast transactions or
             * when building a block.
             */
            apply_transaction(trx, skip);
            ++_current_trx_in_block;
        }

        update_global_dynamic_data(next_block);
        update_signing_witness(signing_witness, next_block);

        update_last_irreversible_block();

        create_block_summary(next_block);
        clear_expired_transactions();
        clear_expired_proposals();
        clear_expired_invites();
        clear_expired_grants();

        // in dbs_database_witness_schedule.cpp
        update_witness_schedule();
        process_research_token_sales();
        process_funds();
        process_common_tokens_withdrawals();
        account_recovery_processing();
        process_content_activity_windows();
        process_hardforks();
        process_grants();

        expertise_stats_service.reset_used_expertise_per_block();

        // notify observers that the block has been applied
        notify_applied_block(next_block);

        notify_changed_objects();
    } // FC_CAPTURE_AND_RETHROW( (next_block.block_num()) )  }
    FC_CAPTURE_LOG_AND_RETHROW((next_block.block_num()))
}

void database::process_header_extensions(const signed_block& next_block)
{
    auto itr = next_block.extensions.begin();

    while (itr != next_block.extensions.end())
    {
        switch (itr->which())
        {
        case 0: // void_t
            break;
        case 1: // version
        {
            auto reported_version = itr->get<version>();
            const auto& signing_witness = get_witness(next_block.witness);
            // idump( (next_block.witness)(signing_witness.running_version)(reported_version) );

            if (reported_version != signing_witness.running_version)
            {
                modify(signing_witness, [&](witness_object& wo) { wo.running_version = reported_version; });
            }
            break;
        }
        case 2: // hardfork_version vote
        {
            auto hfv = itr->get<hardfork_version_vote>();
            const auto& signing_witness = get_witness(next_block.witness);
            // idump( (next_block.witness)(signing_witness.running_version)(hfv) );

            if (hfv.hf_version != signing_witness.hardfork_version_vote
                || hfv.hf_time != signing_witness.hardfork_time_vote)
                modify(signing_witness, [&](witness_object& wo) {
                    wo.hardfork_version_vote = hfv.hf_version;
                    wo.hardfork_time_vote = hfv.hf_time;
                });

            break;
        }
        default:
            FC_ASSERT(false, "Unknown extension in block header");
        }

        ++itr;
    }
}

void database::apply_transaction(const signed_transaction& trx, uint32_t skip)
{
    detail::with_skip_flags(*this, skip, [&]() { _apply_transaction(trx); });
    notify_on_applied_transaction(trx);
}

void database::_apply_transaction(const signed_transaction& trx)
{
    try
    {
        _current_trx_id = trx.id();
        uint32_t skip = get_node_properties().skip_flags;

        if (!(skip & skip_validate)) /* issue #505 explains why this skip_flag is disabled */
            trx.validate();

        auto& trx_idx = get_index<transaction_index>();
        auto trx_id = trx.id();
        // idump((trx_id)(skip&skip_transaction_dupe_check));
        FC_ASSERT((skip & skip_transaction_dupe_check)
                      || trx_idx.indices().get<by_trx_id>().find(trx_id) == trx_idx.indices().get<by_trx_id>().end(),
                  "Duplicate transaction check failed", ("trx_ix", trx_id));

        if (!(skip & (skip_transaction_signatures | skip_authority_check)))
        {
            auto get_active
                = [&](const string& name) { return authority(get<account_authority_object, by_account>(name).active); };
            auto get_owner
                = [&](const string& name) { return authority(get<account_authority_object, by_account>(name).owner); };
            auto get_posting = [&](const string& name) {
                return authority(get<account_authority_object, by_account>(name).posting);
            };

            try
            {
                trx.verify_authority(get_chain_id(), get_active, get_owner, get_posting, DEIP_MAX_SIG_CHECK_DEPTH);
            }
            catch (protocol::tx_missing_active_auth& e)
            {
                if (get_shared_db_merkle().find(head_block_num() + 1) == get_shared_db_merkle().end())
                    throw e;
            }
        }

        // Skip all manner of expiration and TaPoS checking if we're on block 1; It's impossible that the transaction is
        // expired, and TaPoS makes no sense as no blocks exist.
        if (BOOST_LIKELY(head_block_num() > 0))
        {
            if (!(skip & skip_tapos_check))
            {
                const auto& tapos_block_summary = get<block_summary_object>(trx.ref_block_num);
                // Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the
                // expiration
                DEIP_ASSERT(trx.ref_block_prefix == tapos_block_summary.block_id._hash[1],
                              transaction_tapos_exception, "",
                              ("trx.ref_block_prefix", trx.ref_block_prefix)("tapos_block_summary",
                                                                             tapos_block_summary.block_id._hash[1]));
            }

            fc::time_point_sec now = head_block_time();

            DEIP_ASSERT(
                trx.expiration <= now + fc::seconds(DEIP_MAX_TIME_UNTIL_EXPIRATION), transaction_expiration_exception,
                "", ("trx.expiration", trx.expiration)("now", now)("max_til_exp", DEIP_MAX_TIME_UNTIL_EXPIRATION));
            // Simple solution to pending trx bug when now == trx.expiration
            DEIP_ASSERT(now < trx.expiration, transaction_expiration_exception, "",
                          ("now", now)("trx.exp", trx.expiration));
        }

        // Insert transaction into unique transactions database.
        if (!(skip & skip_transaction_dupe_check))
        {
            create<transaction_object>([&](transaction_object& transaction) {
                transaction.trx_id = trx_id;
                transaction.expiration = trx.expiration;
                fc::raw::pack(transaction.packed_trx, trx);
            });
        }

        notify_on_pre_apply_transaction(trx);

        // Finally process the operations
        _current_op_in_trx = 0;
        for (const auto& op : trx.operations)
        {
            try
            {
                apply_operation(op);
                ++_current_op_in_trx;
            }
            FC_CAPTURE_AND_RETHROW((op));
        }
        _current_trx_id = transaction_id_type();
    }
    FC_CAPTURE_AND_RETHROW((trx))
}

void database::apply_operation(const operation& op)
{
    operation_notification note(op);
    notify_pre_apply_operation(note);
    _my->_evaluator_registry.get_evaluator(op).apply(op);
    notify_post_apply_operation(note);
}

const witness_object& database::validate_block_header(uint32_t skip, const signed_block& next_block) const
{
    try
    {
        FC_ASSERT(head_block_id() == next_block.previous, "",
                  ("head_block_id", head_block_id())("next.prev", next_block.previous));
        FC_ASSERT(
            head_block_time() < next_block.timestamp, "",
            ("head_block_time", head_block_time())("next", next_block.timestamp)("blocknum", next_block.block_num()));
        const witness_object& witness = get_witness(next_block.witness);

        if (!(skip & skip_witness_signature))
            FC_ASSERT(next_block.validate_signee(witness.signing_key));

        if (!(skip & skip_witness_schedule_check))
        {
            uint32_t slot_num = get_slot_at_time(next_block.timestamp);
            FC_ASSERT(slot_num > 0);

            string scheduled_witness = get_scheduled_witness(slot_num);

            FC_ASSERT(witness.owner == scheduled_witness, "Witness produced block at wrong time",
                      ("block witness", next_block.witness)("scheduled", scheduled_witness)("slot_num", slot_num));
        }

        return witness;
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::create_block_summary(const signed_block& next_block)
{
    try
    {
        block_summary_id_type sid(next_block.block_num() & 0xffff);
        modify(get<block_summary_object>(sid), [&](block_summary_object& p) { p.block_id = next_block.id(); });
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::update_global_dynamic_data(const signed_block& b)
{
    try
    {
        const dynamic_global_property_object& _dgp = get_dynamic_global_properties();

        uint32_t missed_blocks = 0;
        if (head_block_time() != fc::time_point_sec())
        {
            missed_blocks = get_slot_at_time(b.timestamp);
            assert(missed_blocks != 0);
            missed_blocks--;
            for (uint32_t i = 0; i < missed_blocks; ++i)
            {
                const auto& witness_missed = get_witness(get_scheduled_witness(i + 1));
                if (witness_missed.owner != b.witness)
                {
                    modify(witness_missed, [&](witness_object& w) {
                        w.total_missed++;

                        if (head_block_num() - w.last_confirmed_block_num > DEIP_WITNESS_MISSED_BLOCKS_THRESHOLD)
                        {
                            w.signing_key = public_key_type();
                            push_virtual_operation(shutdown_witness_operation(w.owner));
                        }
                    });
                }
            }
        }

        // dynamic global properties updating
        modify(_dgp, [&](dynamic_global_property_object& dgp) {
            // This is constant time assuming 100% participation. It is O(B) otherwise (B = Num blocks between update)
            for (uint32_t i = 0; i < missed_blocks + 1; i++)
            {
                dgp.participation_count -= dgp.recent_slots_filled.hi & 0x8000000000000000ULL ? 1 : 0;
                dgp.recent_slots_filled = (dgp.recent_slots_filled << 1) + (i == 0 ? 1 : 0);
                dgp.participation_count += (i == 0 ? 1 : 0);
            }

            dgp.head_block_number = b.block_num();
            dgp.head_block_id = b.id();
            dgp.time = b.timestamp;
            dgp.current_aslot += missed_blocks + 1;
        });

        if (!(get_node_properties().skip_flags & skip_undo_history_check))
        {
            DEIP_ASSERT(
                _dgp.head_block_number - _dgp.last_irreversible_block_num < DEIP_MAX_UNDO_HISTORY,
                undo_database_exception,
                "The database does not have enough undo history to support a blockchain with so many missed blocks. "
                "Please add a checkpoint if you would like to continue applying blocks beyond this point.",
                ("last_irreversible_block_num", _dgp.last_irreversible_block_num)("head", _dgp.head_block_number)(
                    "max_undo", DEIP_MAX_UNDO_HISTORY));
        }
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::update_signing_witness(const witness_object& signing_witness, const signed_block& new_block)
{
    try
    {
        const dynamic_global_property_object& dpo = get_dynamic_global_properties();
        uint64_t new_block_aslot = dpo.current_aslot + get_slot_at_time(new_block.timestamp);

        modify(signing_witness, [&](witness_object& _wit) {
            _wit.last_aslot = new_block_aslot;
            _wit.last_confirmed_block_num = new_block.block_num();
        });
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::update_last_irreversible_block()
{
    try
    {
        const dynamic_global_property_object& dpo = get_dynamic_global_properties();

        /**
         * Prior to voting taking over, we must be more conservative...
         *
         */
        if (head_block_num() < DEIP_START_MINER_VOTING_BLOCK)
        {
            modify(dpo, [&](dynamic_global_property_object& _dpo) {
                if (head_block_num() > DEIP_MAX_WITNESSES)
                    _dpo.last_irreversible_block_num = head_block_num() - DEIP_MAX_WITNESSES;
            });
        }
        else
        {
            const witness_schedule_object& wso = get_witness_schedule_object();

            vector<const witness_object*> wit_objs;
            wit_objs.reserve(wso.num_scheduled_witnesses);
            for (int i = 0; i < wso.num_scheduled_witnesses; i++)
                wit_objs.push_back(&get_witness(wso.current_shuffled_witnesses[i]));

            static_assert(DEIP_IRREVERSIBLE_THRESHOLD > 0, "irreversible threshold must be nonzero");

            // 1 1 1 2 2 2 2 2 2 2 -> 2     .7*10 = 7
            // 1 1 1 1 1 1 1 2 2 2 -> 1
            // 3 3 3 3 3 3 3 3 3 3 -> 3

            size_t offset
                = ((DEIP_100_PERCENT - DEIP_IRREVERSIBLE_THRESHOLD) * wit_objs.size() / DEIP_100_PERCENT);

            std::nth_element(wit_objs.begin(), wit_objs.begin() + offset, wit_objs.end(),
                             [](const witness_object* a, const witness_object* b) {
                                 return a->last_confirmed_block_num < b->last_confirmed_block_num;
                             });

            uint32_t new_last_irreversible_block_num = wit_objs[offset]->last_confirmed_block_num;

            if (new_last_irreversible_block_num > dpo.last_irreversible_block_num)
            {
                modify(dpo, [&](dynamic_global_property_object& _dpo) {
                    _dpo.last_irreversible_block_num = new_last_irreversible_block_num;
                });
            }
        }

        commit(dpo.last_irreversible_block_num);

        if (!(get_node_properties().skip_flags & skip_block_log))
        {
            // output to block log based on new last irreverisible block num
            const auto& tmp_head = _block_log.head();
            uint64_t log_head_num = 0;

            if (tmp_head)
                log_head_num = tmp_head->block_num();

            if (log_head_num < dpo.last_irreversible_block_num)
            {
                while (log_head_num < dpo.last_irreversible_block_num)
                {
                    shared_ptr<fork_item> block = _fork_db.fetch_block_on_main_branch_by_number(log_head_num + 1);
                    FC_ASSERT(block, "Current fork in the fork database does not contain the last_irreversible_block");
                    _block_log.append(block->data);
                    log_head_num++;
                }

                _block_log.flush();
            }
        }

        _fork_db.set_max_size(dpo.head_block_number - dpo.last_irreversible_block_num + 1);
    }
    FC_CAPTURE_AND_RETHROW()
}

void database::clear_expired_transactions()
{
    // Look for expired transactions in the deduplication list, and remove them.
    // Transactions must have expired by at least two forking windows in order to be removed.
    auto& transaction_idx = get_index<transaction_index>();
    const auto& dedupe_index = transaction_idx.indices().get<by_expiration>();
    while ((!dedupe_index.empty()) && (head_block_time() > dedupe_index.begin()->expiration))
        remove(*dedupe_index.begin());
}

void database::clear_expired_proposals()
{
    auto& proposal_service = obtain_service<dbs_proposal>();
    proposal_service.clear_expired_proposals();
}

void database::clear_expired_invites()
{
    auto& research_group_invite_service = obtain_service<dbs_research_group_invite>();
    research_group_invite_service.clear_expired_invites();
}

void database::clear_expired_grants()
{
    dbs_grant& grant_service = obtain_service<dbs_grant>();
    grant_service.clear_expired_grants();
}

void database::adjust_balance(const account_object& a, const asset& delta)
{
    modify(a, [&](account_object& acnt) {
        switch (delta.symbol)
        {
        case DEIP_SYMBOL:
            acnt.balance += delta;
            break;
        default:
            FC_ASSERT(false, "invalid symbol");
        }
    });
}

void database::adjust_supply(const asset& delta, bool adjust_common_token)
{
    const auto& props = get_dynamic_global_properties();
    if (props.head_block_number < DEIP_BLOCKS_PER_DAY * 7)
        adjust_common_token = false;

    modify(props, [&](dynamic_global_property_object& props) {
        switch (delta.symbol)
        {
        case DEIP_SYMBOL:
        {
            // TODO: remove unusable value
            asset new_common_token((adjust_common_token && delta.amount > 0) ? delta.amount * 9 : 0, DEIP_SYMBOL);
            props.current_supply += delta + new_common_token;
            props.common_tokens_fund += new_common_token;
            assert(props.current_supply.amount.value >= 0);
            break;
        }
        default:
            FC_ASSERT(false, "invalid symbol");
        }
    });
}

asset database::get_balance(const account_object& a, asset_symbol_type symbol) const
{
    switch (symbol)
    {
    case DEIP_SYMBOL:
        return a.balance;
    default:
        FC_ASSERT(false, "invalid symbol");
    }
}

void database::init_hardforks(time_point_sec genesis_time)
{
    _hardfork_times[0] = genesis_time;
    _hardfork_versions[0] = hardfork_version(0, 0);

    // DEIP: structure to initialize hardofrks

    // FC_ASSERT( DEIP_HARDFORK_0_1 == 1, "Invalid hardfork configuration" );
    //_hardfork_times[ DEIP_HARDFORK_0_1 ] = fc::time_point_sec( DEIP_HARDFORK_0_1_TIME );
    //_hardfork_versions[ DEIP_HARDFORK_0_1 ] = DEIP_HARDFORK_0_1_VERSION;

    const auto& hardforks = get_hardfork_property_object();
    FC_ASSERT(hardforks.last_hardfork <= DEIP_NUM_HARDFORKS, "Chain knows of more hardforks than configuration",
              ("hardforks.last_hardfork", hardforks.last_hardfork)("DEIP_NUM_HARDFORKS", DEIP_NUM_HARDFORKS));
    FC_ASSERT(_hardfork_versions[hardforks.last_hardfork] <= DEIP_BLOCKCHAIN_VERSION,
              "Blockchain version is older than last applied hardfork");
    FC_ASSERT(DEIP_BLOCKCHAIN_HARDFORK_VERSION == _hardfork_versions[DEIP_NUM_HARDFORKS]);
}

void database::process_hardforks()
{
    try
    {
        // If there are upcoming hardforks and the next one is later, do nothing
        const auto& hardforks = get_hardfork_property_object();

        while (_hardfork_versions[hardforks.last_hardfork] < hardforks.next_hardfork
               && hardforks.next_hardfork_time <= head_block_time())
        {
            if (hardforks.last_hardfork < DEIP_NUM_HARDFORKS)
            {
                apply_hardfork(hardforks.last_hardfork + 1);
            }
            else
                throw unknown_hardfork_exception();
        }
    }
    FC_CAPTURE_AND_RETHROW()
}

bool database::has_hardfork(uint32_t hardfork) const
{
    return get_hardfork_property_object().processed_hardforks.size() > hardfork;
}

void database::set_hardfork(uint32_t hardfork, bool apply_now)
{
    auto const& hardforks = get_hardfork_property_object();

    for (uint32_t i = hardforks.last_hardfork + 1; i <= hardfork && i <= DEIP_NUM_HARDFORKS; i++)
    {
        modify(hardforks, [&](hardfork_property_object& hpo) {
            hpo.next_hardfork = _hardfork_versions[i];
            hpo.next_hardfork_time = head_block_time();
        });

        if (apply_now)
            apply_hardfork(i);
    }
}

void database::apply_hardfork(uint32_t hardfork)
{
    if (_log_hardforks)
        elog("HARDFORK ${hf} at block ${b}", ("hf", hardfork)("b", head_block_num()));

    switch (hardfork)
    {
    case DEIP_HARDFORK_0_1:
        break;
    default:
        break;
    }

    modify(get_hardfork_property_object(), [&](hardfork_property_object& hfp) {
        FC_ASSERT(hardfork == hfp.last_hardfork + 1, "Hardfork being applied out of order",
                  ("hardfork", hardfork)("hfp.last_hardfork", hfp.last_hardfork));
        FC_ASSERT(hfp.processed_hardforks.size() == hardfork, "Hardfork being applied out of order");
        hfp.processed_hardforks.push_back(_hardfork_times[hardfork]);
        hfp.last_hardfork = hardfork;
        hfp.current_hardfork_version = _hardfork_versions[hardfork];
        FC_ASSERT(hfp.processed_hardforks[hfp.last_hardfork] == _hardfork_times[hfp.last_hardfork],
                  "Hardfork processing failed sanity check...");
    });

    push_hf_operation(hardfork_operation(hardfork));
}

/**
 * Verifies all supply invariantes check out
 */
void database::validate_invariants() const
{
    try
    {
        const auto& account_idx = get_index<account_index>().indices().get<by_name>();
        asset total_supply = asset(0, DEIP_SYMBOL);
        share_type total_common_tokens_amount = share_type(0);
        share_type total_expert_tokens_amount = share_type(0);

        share_type total_vsf_votes = share_type(0);

        auto gpo = get_dynamic_global_properties();

        /// verify no witness has too many votes
        const auto& witness_idx = get_index<witness_index>().indices();
        for (auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr)
            FC_ASSERT(itr->votes <= gpo.total_expert_tokens_amount, "", ("itr", *itr));

        for (auto itr = account_idx.begin(); itr != account_idx.end(); ++itr)
        {
            total_supply += itr->balance;
            total_common_tokens_amount += itr->common_tokens_balance;
            total_expert_tokens_amount += itr->expertise_tokens_balance;
            
            total_vsf_votes += (itr->proxy == DEIP_PROXY_TO_SELF_ACCOUNT
                                    ? itr->witness_vote_weight()
                                    : (DEIP_MAX_PROXY_RECURSION_DEPTH > 0
                                           ? itr->proxied_vsf_votes[DEIP_MAX_PROXY_RECURSION_DEPTH - 1]
                                           : itr->expertise_tokens_balance));
        }

        const auto& reward_idx = get_index<reward_fund_index, by_id>();

        for (auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr)
        {
            total_supply += itr->reward_balance;
        }

        const auto& research_group_idx = get_index<research_group_index, by_id>();

        for (auto itr = research_group_idx.begin(); itr != research_group_idx.end(); ++itr)
        {
            total_supply += itr->balance;
        }

        const auto& research_content_reward_pool_idx = get_index<research_content_reward_pool_index, by_id>();

        for (auto itr = research_content_reward_pool_idx.begin(); itr != research_content_reward_pool_idx.end(); ++itr)
        {
            total_supply += itr->reward_share;
        }

        total_supply +=  gpo.common_tokens_fund;

        FC_ASSERT(gpo.current_supply == total_supply, "",
                  ("gpo.current_supply", gpo.current_supply)("total_supply", total_supply));
        FC_ASSERT(gpo.total_common_tokens_amount == total_common_tokens_amount, "",
                  ("gpo.common_tokens_balance", gpo.total_common_tokens_amount)("total_common_tokens", total_common_tokens_amount));
        FC_ASSERT(gpo.total_expert_tokens_amount == total_expert_tokens_amount, "",
                  ("gpo.total_expert_tokens", gpo.total_expert_tokens_amount)("total_expert_tokens", total_expert_tokens_amount));

//        FC_ASSERT(gpo.total_expert_tokens_amount == total_vsf_votes, "",
//                  ("total_expert_tokens_amount", gpo.total_expert_tokens_amount)("total_vsf_votes", total_vsf_votes));
    }
    FC_CAPTURE_LOG_AND_RETHROW((head_block_num()));
}

void database::retally_witness_votes()
{
    dbs_witness& witness_service = obtain_service<dbs_witness>();

    const auto& witness_idx = get_index<witness_index>().indices();

    // Clear all witness votes
    for (auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr)
    {
        modify(*itr, [&](witness_object& w) {
            w.votes = 0;
            w.virtual_position = 0;
        });
    }

    const auto& account_idx = get_index<account_index>().indices();

    // Apply all existing votes by account
    for (auto itr = account_idx.begin(); itr != account_idx.end(); ++itr)
    {
        if (itr->proxy != DEIP_PROXY_TO_SELF_ACCOUNT)
            continue;

        const auto& a = *itr;

        const auto& vidx = get_index<witness_vote_index>().indices().get<by_account_witness>();
        auto wit_itr = vidx.lower_bound(boost::make_tuple(a.id, witness_id_type()));
        while (wit_itr != vidx.end() && wit_itr->account == a.id)
        {
            witness_service.adjust_witness_vote(get(wit_itr->witness), a.witness_vote_weight());
            ++wit_itr;
        }
    }
}

void database::process_content_activity_windows()
{
    auto now = head_block_time();
    dbs_research_content& research_content_service = obtain_service<dbs_research_content>();
    dbs_discipline& discipline_service = obtain_service<dbs_discipline>();
    dbs_vote& votes_service = obtain_service<dbs_vote>();
    dbs_research_content_reward_pool& research_content_reward_pool_service = obtain_service<dbs_research_content_reward_pool>();

    const auto& research_content_by_activity_end = get_index<research_content_index, by_activity_window_end>();
    auto itr_by_end = research_content_by_activity_end.begin();

    std::map<discipline_id_type, share_type> expired_active_weight;

    // close activity windows for content with expired end point
    while (itr_by_end != research_content_by_activity_end.end() && itr_by_end->activity_window_end < now)
    {
        modify(research_content_service.get(itr_by_end->id), [&](research_content_object& rc) {

            if (rc.type == research_content_type::announcement ||
                rc.type == research_content_type::milestone) {

                switch (rc.activity_round) {
                    case 1: {
                        // the 2nd activity period for intermediate result
                        // starts in 2 weeks after the 1st one has ended and continues for 1 week
                        rc.activity_round = 2;
                        rc.activity_state = research_content_activity_state::pending;
                        rc.activity_window_start = now + DAYS_TO_SECONDS(14);
                        rc.activity_window_end = now + DAYS_TO_SECONDS(14 + 7);
                        break;
                    }
                    default: {
                        // mark intermediate result activity period as expired
                        // and set the bounds to max value to exclude content from future iterations
                        rc.activity_round = 0;
                        rc.activity_state = research_content_activity_state::closed;
                        rc.activity_window_start = fc::time_point_sec::maximum();
                        rc.activity_window_end = fc::time_point_sec::maximum();
                        break;
                    }
                }

            } else if (rc.type == research_content_type::final_result) {

                switch (rc.activity_round) {
                    case 1: {
                        // the 2nd activity period for final result
                        // starts in 2 months after the 1st one has ended and continues for 1 months
                        rc.activity_round = 2;
                        rc.activity_state = research_content_activity_state::pending;
                        rc.activity_window_start = now + DAYS_TO_SECONDS(60);
                        rc.activity_window_end = now + DAYS_TO_SECONDS(60 + 30);
                        break;
                    }
                    case 2: {
                        // the 3rd activity period for final result
                        // starts in one half of a year after the 2nd one has ended and continues for 2 weeks
                        rc.activity_round = 3;
                        rc.activity_state = research_content_activity_state::pending;
                        rc.activity_window_start = now + DAYS_TO_SECONDS(182);
                        rc.activity_window_end = now + DAYS_TO_SECONDS(182 + 14);
                        break;
                    }
                    default: {
                        // mark final result activity period as expired
                        // and set the bounds to max value to exclude content from future iterations
                        rc.activity_round = 0;
                        rc.activity_state = research_content_activity_state::closed;
                        rc.activity_window_start = fc::time_point_sec::maximum();
                        rc.activity_window_end = fc::time_point_sec::maximum();
                        break;
                    }
                }
            }
        });

        // get all total votes for current content and accumulate it by discipline and type
        auto rc_total_votes_refs = votes_service.get_total_votes_by_content(itr_by_end->id);
        for (auto wrapper : rc_total_votes_refs) {
            const total_votes_object& rc_total_votes = wrapper.get();
            if (expired_active_weight.find(rc_total_votes.discipline_id) != expired_active_weight.end()) {
                expired_active_weight[rc_total_votes.discipline_id] += rc_total_votes.total_weight;
            } else {
                expired_active_weight[rc_total_votes.discipline_id] = rc_total_votes.total_weight;
            }
        }

        //distribute content reward

        auto research_content_reward_pools = research_content_reward_pool_service.get_research_content_reward_pools_by_content_id(itr_by_end->id);

        for (const research_content_reward_pool_object& research_content_reward_pool : research_content_reward_pools)
        {
            auto used_reward = reward_research_content(itr_by_end->id,
                                                       research_content_reward_pool.discipline_id,
                                                       research_content_reward_pool.reward_share,
                                                       research_content_reward_pool.expertise_share);

            modify(research_content_reward_pool, [&](research_content_reward_pool_object& rcrp) {
                rcrp.reward_share -= used_reward;
                rcrp.expertise_share = 0;
            });

        }

        ++itr_by_end;
    }


    // decrease total active votes in discipline object
    for(auto it = expired_active_weight.begin(); it != expired_active_weight.end(); it++)
    {
        auto& discipline = discipline_service.get_discipline(it->first);
        modify(discipline, [&](discipline_object& d) {
            d.total_active_weight -= it->second;
        });
    }



    const auto& research_content_by_activity_start = get_index<research_content_index, by_activity_window_start>();
    auto itr_by_start = research_content_by_activity_start.begin();
    std::map<discipline_id_type, share_type> resumed_active_weight;

    // reopen activity windows for content with actual start point
    while (itr_by_start != research_content_by_activity_start.end() && itr_by_start->activity_window_start < now)
    {
        if(itr_by_start->activity_state == research_content_activity_state::pending)
        {
            auto& content = research_content_service.get(itr_by_start->id);
            modify(content, [&](research_content_object& rc) {
                    rc.activity_state = research_content_activity_state::active;
            });

            // get all total votes for current content and accumulate it by discipline and type
            auto rc_total_votes_refs = votes_service.get_total_votes_by_content(itr_by_start->id);
            for (auto wrapper : rc_total_votes_refs) {
                const total_votes_object& rc_total_votes = wrapper.get();
                if (resumed_active_weight.find(rc_total_votes.discipline_id) != resumed_active_weight.end()) {
                    resumed_active_weight[rc_total_votes.discipline_id] += rc_total_votes.total_weight;
                } else {
                    resumed_active_weight[rc_total_votes.discipline_id] = rc_total_votes.total_weight;
                }
            }
        }
        ++itr_by_start;
    }

    // increase total active votes in discipline object
    for(auto it = resumed_active_weight.begin(); it != resumed_active_weight.end(); it++)
    {
        auto& discipline = discipline_service.get_discipline(it->first);
        modify(discipline, [&](discipline_object& d) {
            d.total_active_weight += it->second;
        });
    }
}

share_type database::calculate_review_weight_modifier(const review_id_type& review_id, const discipline_id_type& discipline_id)
{
    dbs_review& review_service = obtain_service<dbs_review>();
    dbs_vote& vote_service = obtain_service<dbs_vote>();

    auto& review = review_service.get(review_id);
    auto content_reviews = review_service.get_research_content_reviews(review.research_content_id);

    share_type total_expertise = 0;
    share_type total_weight = 0;
    std::map<review_id_type, share_type> weights_per_review;
    for (auto review_wrapper : content_reviews) {
        auto& content_review = review_wrapper.get();
        total_expertise += content_review.expertise_amounts_used.at(discipline_id);

        auto votes = vote_service.get_review_votes_by_review_and_discipline(content_review.id, discipline_id);
        share_type votes_weight = share_type(0);

        for (uint32_t i = 0; i < votes.size(); i++) {
            auto& vote = votes.at(i).get();
            votes_weight += vote.weight;
        }

        weights_per_review[content_review.id] = votes_weight;
        total_weight += votes_weight;
    }

    if (content_reviews.size() == 0 || total_weight == 0) return 0;

    share_type avg_expertise = total_expertise / content_reviews.size();
    auto review_weight = weights_per_review[review.id];
    auto review_used_expertise = review.expertise_amounts_used.at(discipline_id);
    return 1 * (review_used_expertise / avg_expertise) + 10 * (1 - 1 / content_reviews.size()) * (review_weight / total_weight);
}


} // namespace chain
} // namespace deip
