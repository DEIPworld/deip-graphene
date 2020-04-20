#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/database/database.hpp>
#include <deip/chain/database/database_exceptions.hpp>
#include <deip/chain/database/db_with.hpp>

#include <deip/chain/deip_evaluator.hpp>
#include <deip/chain/evaluator_registry.hpp>
#include <deip/chain/genesis_state.hpp>
#include <deip/chain/operation_notification.hpp>
#include <deip/chain/shared_db_merkle.hpp>

#include <deip/chain/schema/award_object.hpp>
#include <deip/chain/schema/award_recipient_object.hpp>
#include <deip/chain/schema/award_withdrawal_request_object.hpp>
#include <deip/chain/schema/block_summary_object.hpp>
#include <deip/chain/schema/chain_property_object.hpp>
#include <deip/chain/schema/deip_objects.hpp>
#include <deip/chain/schema/global_property_object.hpp>
#include <deip/chain/schema/nda_contract_file_access_object.hpp>
#include <deip/chain/schema/offer_research_tokens_object.hpp>
#include <deip/chain/schema/operation_object.hpp>
#include <deip/chain/schema/research_discipline_relation_object.hpp>
#include <deip/chain/schema/research_object.hpp>
#include <deip/chain/schema/research_token_object.hpp>
#include <deip/chain/schema/expertise_contribution_object.hpp>
#include <deip/chain/schema/transaction_object.hpp>

#include <deip/chain/services/dbs_account.hpp>
#include <deip/chain/services/dbs_account_balance.hpp>
#include <deip/chain/services/dbs_asset.hpp>
#include <deip/chain/services/dbs_discipline.hpp>
#include <deip/chain/services/dbs_discipline_supply.hpp>
#include <deip/chain/services/dbs_dynamic_global_properties.hpp>
#include <deip/chain/services/dbs_expert_token.hpp>
#include <deip/chain/services/dbs_expertise_allocation_proposal.hpp>
#include <deip/chain/services/dbs_nda_contract.hpp>
#include <deip/chain/services/dbs_proposal.hpp>
#include <deip/chain/services/dbs_research_content.hpp>
#include <deip/chain/services/dbs_research_group.hpp>
#include <deip/chain/services/dbs_research_group_invite.hpp>
#include <deip/chain/services/dbs_research_token_sale.hpp>
#include <deip/chain/services/dbs_review.hpp>
#include <deip/chain/services/dbs_reward_pool.hpp>
#include <deip/chain/services/dbs_vesting_balance.hpp>
#include <deip/chain/services/dbs_review_vote.hpp>
#include <deip/chain/services/dbs_expertise_contribution.hpp>
#include <deip/chain/services/dbs_witness.hpp>
#include <deip/chain/services/dbs_grant.hpp>
#include <deip/chain/services/dbs_grant_application.hpp>
#include <deip/chain/services/dbs_grant_application_review.hpp>
#include <deip/chain/services/dbs_funding_opportunity.hpp>

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

#include <boost/range/adaptor/transformed.hpp>

#ifdef DEFAULT_LOGGER
#undef DEFAULT_LOGGER
#endif
#define DEFAULT_LOGGER "db_all"

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
    FC_ASSERT(fee.symbol == DEIP_SYMBOL, "Invalid asset type.");
    FC_ASSERT(fee.amount >= 0); /// NOTE if this fails then validate() on some operation is probably wrong
    if (fee.amount == 0)
        return;

    auto& account_balance_service = obtain_service<dbs_account_balance>();

    account_balance_service.adjust_balance(account.name, -fee);
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

void database::push_virtual_operation(const operation& op)
{
#if defined(IS_LOW_MEM) && !defined(IS_TEST_NET)
    return;
#endif

    FC_ASSERT(is_virtual_operation(op));
    operation_notification note(op);
    notify_pre_apply_operation(note);
    notify_post_apply_operation(note);
}

void database::push_hf_operation(const operation& op)
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
    dbs_account_balance& account_balance_service = obtain_service<dbs_account_balance>();

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
                    auto& account_balance = account_balance_service.get_by_owner_and_asset(to_account.name, DEIP_SYMBOL);
                    modify(account_balance, [&](account_balance_object& ab_o) { ab_o.amount += converted_deip; });

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

        auto& from_account_balance = account_balance_service.get_by_owner_and_asset(from_account.name, DEIP_SYMBOL);
        modify(from_account_balance, [&](account_balance_object& ab_o) { ab_o.amount += converted_deip; });

        modify(from_account, [&](account_object& a) {
            a.common_tokens_balance -= to_withdraw;
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

    share_type to_emit = (props.current_supply.amount * current_inflation_rate) / (int64_t(DEIP_100_PERCENT) * int64_t(DEIP_BLOCKS_PER_YEAR));

    asset new_deip = asset(to_emit, DEIP_SYMBOL);
    // TODO: Expertise adaptive emission model
    share_type new_expertise = to_emit;

    asset contribution_reward = util::calculate_share(new_deip, DEIP_CONTRIBUTION_REWARD_PERCENT); /// 97% to contribution rewards
    share_type witness_reward = new_deip.amount - contribution_reward.amount; /// Remaining 3% to witness pay

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

    new_deip = asset(contribution_reward.amount + witness_reward, DEIP_SYMBOL);

    modify(props, [&](dynamic_global_property_object& p) {
        p.current_supply += new_deip;
    });

    account_service.increase_common_tokens(get_account(cwit.owner), witness_reward);

    // witness_reward = producer_reward because 1 DEIP = 1 Common Token. Add producer_reward as separate value if 1 DEIP != 1 Common Token
    push_virtual_operation(producer_reward_operation(cwit.owner, witness_reward));
}

asset database::distribute_reward(const asset& reward, const share_type& expertise)
{
    dbs_expertise_contribution& expertise_contributions_service = obtain_service<dbs_expertise_contribution>();
    dbs_research_content& research_content_service = obtain_service<dbs_research_content>();
    dbs_review& reviews_service = obtain_service<dbs_review>();
    dbs_expert_token& expert_tokens_service = obtain_service<dbs_expert_token>();
    const fc::time_point_sec now = head_block_time();

    const auto& altered_contributions = expertise_contributions_service.get_altered_expertise_contributions_in_block();

    for (auto& wrap : altered_contributions)
    {
        const expertise_contribution_object& expertise_contribution = wrap.get();
        const research_content_object& research_content = research_content_service.get_research_content(expertise_contribution.research_content_id);
        const auto& research_content_reviews = reviews_service.get_reviews_by_research_content(expertise_contribution.research_content_id);

        std::multimap<share_type, account_name_type, std::greater<share_type>> upvoters;
        share_type upvoters_total_expertise = 0;
        std::multimap<share_type, account_name_type, std::greater<share_type>> downvoters;
        share_type downvoters_total_expertise = 0;

        for (auto& review_wrap : research_content_reviews)
        {
            const review_object& review = review_wrap.get();
            if (review.expertise_tokens_amount_by_discipline.count(expertise_contribution.discipline_id) != 0)
            {
                const share_type review_expertise = review.expertise_tokens_amount_by_discipline.at(expertise_contribution.discipline_id);
                if (review.is_positive)
                {
                    upvoters.insert(std::make_pair(review_expertise, review.author));
                    upvoters_total_expertise += review_expertise;
                }
                else
                {
                    downvoters.insert(std::make_pair(review_expertise, review.author));
                    downvoters_total_expertise += review_expertise;
                }
            }
        }

        for (auto& diff : expertise_contribution.eci_current_block_diffs)
        {
            const share_type delta = diff.diff();

            if (delta > 0) // reward for upvoters, penalty for downvoters
            {
                const share_type reviewers_expertise_reward = util::calculate_share(delta.value, DEIP_1_PERCENT * 30);
                const share_type researchers_expertise_reward = delta.value - reviewers_expertise_reward;

                for (auto& upvoter : upvoters)
                {
                    const share_type reviewer_expertise_reward = util::calculate_share(
                      reviewers_expertise_reward, 
                      upvoter.first, 
                      upvoters_total_expertise);

                    const auto& exp_token_diff = expert_tokens_service.adjust_expert_token(
                      upvoter.second,
                      expertise_contribution.discipline_id, 
                      reviewer_expertise_reward);

                    const eci_diff upvoter_eci_diff = eci_diff(
                      std::get<0>(exp_token_diff),
                      std::get<1>(exp_token_diff),
                      now,
                      diff.alteration_source_type,
                      diff.alteration_source_id
                    );

                    push_virtual_operation(account_eci_history_operation(
                        upvoter.second, 
                        expertise_contribution.discipline_id._id,
                        upvoter_eci_diff)
                    );
                }

                for (auto& downvoter : downvoters)
                {
                    const share_type reviewer_expertise_penalty = util::calculate_share(
                      reviewers_expertise_reward, 
                      downvoter.first, 
                      downvoters_total_expertise);

                    const auto& exp_token_diff = expert_tokens_service.adjust_expert_token(
                      downvoter.second,
                      expertise_contribution.discipline_id, 
                      -reviewer_expertise_penalty);

                    const eci_diff downvoter_eci_diff = eci_diff(
                      std::get<0>(exp_token_diff),
                      std::get<1>(exp_token_diff),
                      now,
                      diff.alteration_source_type,
                      diff.alteration_source_id
                    );

                    push_virtual_operation(account_eci_history_operation(
                        downvoter.second, 
                        expertise_contribution.discipline_id._id,
                        downvoter_eci_diff)
                    );
                }

                for (auto& author : research_content.authors)
                {
                    const share_type author_expertise_reward = util::calculate_share(
                      researchers_expertise_reward,
                      DEIP_100_PERCENT / research_content.authors.size(), 
                      DEIP_100_PERCENT);

                    const auto& exp_token_diff = expert_tokens_service.adjust_expert_token(
                        author, 
                        expertise_contribution.discipline_id, 
                        author_expertise_reward
                    );

                    const eci_diff author_eci_diff = eci_diff(
                      std::get<0>(exp_token_diff),
                      std::get<1>(exp_token_diff),
                      now,
                      diff.alteration_source_type,
                      diff.alteration_source_id
                    );

                    push_virtual_operation(account_eci_history_operation(
                        author, 
                        expertise_contribution.discipline_id._id,
                        author_eci_diff)
                    );
                }
            }

            else if (delta < 0) // reward for downvoters, penalty for upvoters
            {
                const share_type reviewers_expertise_reward = util::calculate_share(abs(delta.value), DEIP_1_PERCENT * 30);
                const share_type researchers_expertise_reward = abs(delta.value) - reviewers_expertise_reward;

                for (auto& upvoter : upvoters)
                {
                    const share_type reviewer_expertise_penalty = util::calculate_share(
                      reviewers_expertise_reward, 
                      upvoter.first, 
                      upvoters_total_expertise);

                    const auto& exp_token_diff = expert_tokens_service.adjust_expert_token(
                      upvoter.second, 
                      expertise_contribution.discipline_id,
                      -reviewer_expertise_penalty);

                    const eci_diff upvoter_eci_diff = eci_diff(
                      std::get<0>(exp_token_diff),
                      std::get<1>(exp_token_diff),
                      now,
                      diff.alteration_source_type,
                      diff.alteration_source_id
                    );

                    push_virtual_operation(account_eci_history_operation(
                        upvoter.second, 
                        expertise_contribution.discipline_id._id,
                        upvoter_eci_diff)
                    );
                }

                for (auto& downvoter : downvoters)
                {
                    const share_type reviewer_expertise_reward = util::calculate_share(
                      reviewers_expertise_reward, 
                      downvoter.first,
                      downvoters_total_expertise);

                    const auto& exp_token_diff = expert_tokens_service.adjust_expert_token(
                      downvoter.second, 
                      expertise_contribution.discipline_id,
                      reviewer_expertise_reward);

                    const eci_diff downvoter_eci_diff = eci_diff(
                      std::get<0>(exp_token_diff),
                      std::get<1>(exp_token_diff),
                      now,
                      diff.alteration_source_type,
                      diff.alteration_source_id
                    );

                    push_virtual_operation(account_eci_history_operation(
                        downvoter.second, 
                        expertise_contribution.discipline_id._id,
                        downvoter_eci_diff)
                    );
                }

                for (auto& author : research_content.authors)
                {
                    const share_type author_expertise_penalty = util::calculate_share(
                      researchers_expertise_reward, 
                      DEIP_100_PERCENT / research_content.authors.size(), 
                      DEIP_100_PERCENT);

                    const auto& exp_token_diff = expert_tokens_service.adjust_expert_token(
                      author, 
                      expertise_contribution.discipline_id,
                      -author_expertise_penalty);

                    const eci_diff author_eci_diff = eci_diff(
                      std::get<0>(exp_token_diff),
                      std::get<1>(exp_token_diff),
                      now,
                      diff.alteration_source_type,
                      diff.alteration_source_id
                    );

                    push_virtual_operation(account_eci_history_operation(
                        author, 
                        expertise_contribution.discipline_id._id,
                        author_eci_diff)
                    );
                }
            }
        }

        modify(expertise_contribution, [&](expertise_contribution_object& ec_o) { 
          ec_o.eci_current_block_delta = 0;
          ec_o.eci_current_block_diffs.clear();
          ec_o.has_eci_current_block_diffs = false;
        });
    }

    return reward;
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
    _my->_evaluator_registry.register_evaluator<create_account_evaluator>();
    _my->_evaluator_registry.register_evaluator<account_update_evaluator>();
    _my->_evaluator_registry.register_evaluator<witness_update_evaluator>();
    _my->_evaluator_registry.register_evaluator<account_witness_vote_evaluator>();
    _my->_evaluator_registry.register_evaluator<account_witness_proxy_evaluator>();
    _my->_evaluator_registry.register_evaluator<request_account_recovery_evaluator>();
    _my->_evaluator_registry.register_evaluator<recover_account_evaluator>();
    _my->_evaluator_registry.register_evaluator<change_recovery_account_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_proposal_evaluator>();
    _my->_evaluator_registry.register_evaluator<make_review_evaluator>();
    _my->_evaluator_registry.register_evaluator<contribute_to_token_sale_evaluator>();
    _my->_evaluator_registry.register_evaluator<approve_research_group_invite_evaluator>();
    _my->_evaluator_registry.register_evaluator<reject_research_group_invite_evaluator>();
    _my->_evaluator_registry.register_evaluator<vote_for_review_evaluator>();
    _my->_evaluator_registry.register_evaluator<transfer_research_tokens_to_research_group_evaluator>();
    _my->_evaluator_registry.register_evaluator<research_update_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_vesting_balance_evaluator>();
    _my->_evaluator_registry.register_evaluator<withdraw_vesting_balance_evaluator>();
    _my->_evaluator_registry.register_evaluator<vote_proposal_evaluator>();
    _my->_evaluator_registry.register_evaluator<transfer_research_tokens_evaluator>();
    _my->_evaluator_registry.register_evaluator<delegate_expertise_evaluator>();
    _my->_evaluator_registry.register_evaluator<revoke_expertise_delegation_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_expertise_allocation_proposal_evaluator>();
    _my->_evaluator_registry.register_evaluator<vote_for_expertise_allocation_proposal_evaluator>();
    _my->_evaluator_registry.register_evaluator<accept_research_token_offer_evaluator>();
    _my->_evaluator_registry.register_evaluator<reject_research_token_offer_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_grant_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_grant_application_evaluator>();
    _my->_evaluator_registry.register_evaluator<make_review_for_application_evaluator>();
    _my->_evaluator_registry.register_evaluator<approve_grant_application_evaluator>();
    _my->_evaluator_registry.register_evaluator<reject_grant_application_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_asset_evaluator>();
    _my->_evaluator_registry.register_evaluator<issue_asset_evaluator>();
    _my->_evaluator_registry.register_evaluator<reserve_asset_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_award_evaluator>();
    _my->_evaluator_registry.register_evaluator<approve_award_evaluator>();
    _my->_evaluator_registry.register_evaluator<reject_award_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_award_withdrawal_request_evaluator>();
    _my->_evaluator_registry.register_evaluator<certify_award_withdrawal_request_evaluator>();
    _my->_evaluator_registry.register_evaluator<approve_award_withdrawal_request_evaluator>();
    _my->_evaluator_registry.register_evaluator<reject_award_withdrawal_request_evaluator>();
    _my->_evaluator_registry.register_evaluator<pay_award_withdrawal_request_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_nda_contract_evaluator>();
    _my->_evaluator_registry.register_evaluator<sign_nda_contract_evaluator>();
    _my->_evaluator_registry.register_evaluator<decline_nda_contract_evaluator>();
    _my->_evaluator_registry.register_evaluator<close_nda_contract_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_request_by_nda_contract_evaluator>();
    _my->_evaluator_registry.register_evaluator<fulfill_request_by_nda_contract_evaluator>();
    _my->_evaluator_registry.register_evaluator<invite_member_evaluator>();
    _my->_evaluator_registry.register_evaluator<exclude_member_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_research_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_research_content_evaluator>();
    _my->_evaluator_registry.register_evaluator<create_research_token_sale_evaluator>();
    _my->_evaluator_registry.register_evaluator<update_research_group_metadata_evaluator>();
    _my->_evaluator_registry.register_evaluator<update_research_metadata_evaluator>();
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
    add_index<hardfork_property_index>();
    add_index<withdraw_common_tokens_route_index>();
    add_index<owner_authority_history_index>();
    add_index<account_recovery_request_index>();
    add_index<change_recovery_account_request_index>();
    add_index<discipline_supply_index>();
    add_index<proposal_index>();
    add_index<proposal_vote_index>();
    add_index<research_group_index>();
    add_index<research_group_token_index>();
    add_index<research_group_organization_contract_index>();
    add_index<discipline_index>();
    add_index<research_discipline_relation_index>();
    add_index<research_index>();
    add_index<research_content_index>();
    add_index<expert_token_index>();
    add_index<research_token_index>();
    add_index<research_token_sale_index>();
    add_index<research_token_sale_contribution_index>();
    add_index<expertise_contribution_index>();
    add_index<research_group_invite_index>();
    add_index<review_index>();
    add_index<review_vote_index>();
    add_index<vesting_balance_index>();
    add_index<reward_pool_index>();
    add_index<expertise_allocation_proposal_index>();
    add_index<expertise_allocation_proposal_vote_index>();
    add_index<offer_research_tokens_index>();
    add_index<grant_index>();
    add_index<grant_application_index>();
    add_index<grant_application_review_index>();
    add_index<funding_opportunity_index>();
    add_index<account_balance_index>();
    add_index<asset_index>();
    add_index<award_index>();
    add_index<award_recipient_index>();
    add_index<award_withdrawal_request_index>();
    add_index<nda_contract_index>();
    add_index<nda_contract_file_access_index>();

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

        auto& account_service = obtain_service<dbs_account>();
        auto& discipline_supply_service = obtain_service<dbs_discipline_supply>();
        auto& expertise_allocation_proposal_service = obtain_service<dbs_expertise_allocation_proposal>();
        auto& proposal_service = obtain_service<dbs_proposal>();
        auto& research_group_invite_service = obtain_service<dbs_research_group_invite>();
        auto& research_token_sale_service = obtain_service<dbs_research_token_sale>();
        auto& nda_contract_service = obtain_service<dbs_nda_contract>();

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

        proposal_service.clear_expired_proposals();
        research_group_invite_service.clear_expired_invites();
        discipline_supply_service.clear_expired_discipline_supplies();

        // in dbs_database_witness_schedule.cpp
        update_witness_schedule();
        research_token_sale_service.process_research_token_sales();
        process_funds();
        process_common_tokens_withdrawals();
        account_service.process_account_recovery();
        // process_content_activity_windows();
        process_hardforks();
        discipline_supply_service.process_discipline_supplies();

        expertise_allocation_proposal_service.process_expertise_allocation_proposals();
        nda_contract_service.process_nda_contracts();

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
            idump( (next_block.witness)(signing_witness.running_version)(reported_version) );

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
            idump( (next_block.witness)(signing_witness.running_version)(hfv) );

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
        FC_ASSERT((skip & skip_transaction_dupe_check) || trx_idx.indices().get<by_trx_id>().find(trx_id) == trx_idx.indices().get<by_trx_id>().end(),
          "Duplicate transaction check failed", ("trx_ix", trx_id));

        if (!(skip & (skip_transaction_signatures | skip_authority_check)))
        {
            auto get_active
                = [&](const string& name) { return authority(get<account_authority_object, by_account>(name).active); };
            auto get_owner
                = [&](const string& name) { return authority(get<account_authority_object, by_account>(name).owner); };
            auto get_posting
                = [&](const string& name) { return authority(get<account_authority_object, by_account>(name).posting); };

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

asset database::get_balance(const account_object& a,const protocol::asset_symbol_type& symbol) const
{
    dbs_asset& asset_service = obtain_service<dbs_asset>();
    dbs_account_balance& account_balance_service = obtain_service<dbs_account_balance>();

    asset_service.check_existence(symbol);
    account_balance_service.check_existence_by_owner_and_asset(a.name, symbol);

    auto& balance = account_balance_service.get_by_owner_and_asset(a.name, symbol);

    return asset(balance.amount, balance.symbol);
}

void database::init_hardforks(time_point_sec genesis_time)
{
    _hardfork_times[0] = genesis_time;
    _hardfork_versions[0] = hardfork_version(0, 0);

    // DEIP: structure to initialize hardofrks

    FC_ASSERT( DEIP_HARDFORK_0_1 == 1, "Invalid hardfork configuration" );
    _hardfork_times[ DEIP_HARDFORK_0_1 ] = fc::time_point_sec( DEIP_HARDFORK_0_1_TIME );
    _hardfork_versions[ DEIP_HARDFORK_0_1 ] = DEIP_HARDFORK_0_1_VERSION;

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
        case DEIP_HARDFORK_0_1: {

            #ifdef IS_TEST_NET
                const auto& w_idx = get_index<witness_index>().indices().get<by_name>();
                auto w_itr = w_idx.begin();
                while (w_itr != w_idx.end())
                {
                    if (w_itr->owner != DEIP_HARDFORK_0_1_ACTIVE_WITNESS)
                    {
                        remove(*(w_itr++));
                    }
                    else
                    {
                        ++w_itr;
                    }
                }

                dbs_account& account_service = obtain_service<dbs_account>();
                const auto& a_idx = get_index<account_index>().indices().get<by_name>();
                for (auto a_itr = a_idx.begin(); a_itr != a_idx.end(); a_itr++) 
                {
                    const account_object& a = *a_itr;
                    account_service.clear_witness_votes(a);
                }

                const witness_schedule_object& wso = get_witness_schedule_object();
                modify(wso, [&](witness_schedule_object& _wso) {
                    _wso.current_shuffled_witnesses[0] = DEIP_HARDFORK_0_1_ACTIVE_WITNESS;
                    _wso.num_scheduled_witnesses = 1;
                    _wso.witness_pay_normalization_factor = _wso.top20_weight * 1 + _wso.timeshare_weight * 0;
                    _wso.next_shuffle_block_num = head_block_num() + _wso.num_scheduled_witnesses;
                });

                _reset_virtual_schedule_time();
                _update_median_witness_props();

                const witness_object& wit = get_witness(DEIP_HARDFORK_0_1_ACTIVE_WITNESS);
                modify(wit, [&](witness_object& wobj) {
                    wobj.votes = share_type(0);
                });
            #endif

            break;
        }
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
        auto& account_balance_service = obtain_service<dbs_account_balance>();
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
            auto& balance = account_balance_service.get_by_owner_and_asset(itr->name, DEIP_SYMBOL);
            total_supply += asset(balance.amount, balance.symbol);
            total_common_tokens_amount += itr->common_tokens_balance;
            total_expert_tokens_amount += itr->expertise_tokens_balance;
            
            total_vsf_votes += (itr->proxy == DEIP_PROXY_TO_SELF_ACCOUNT
                                    ? itr->witness_vote_weight()
                                    : (DEIP_MAX_PROXY_RECURSION_DEPTH > 0
                                           ? itr->proxied_vsf_votes[DEIP_MAX_PROXY_RECURSION_DEPTH - 1]
                                           : itr->expertise_tokens_balance));
        }

        const auto& research_group_idx = get_index<research_group_index, by_id>();
        for (auto itr = research_group_idx.begin(); itr != research_group_idx.end(); ++itr)
        {
            total_supply += itr->balance;
        }

        const auto& reward_pool_idx = get_index<reward_pool_index, by_id>();
        for (auto itr = reward_pool_idx.begin(); itr != reward_pool_idx.end(); ++itr)
        {
            total_supply += itr->balance;
        }

        const auto& vesting_balance_idx = get_index<vesting_balance_index, by_id>();
        for (auto itr = vesting_balance_idx.begin(); itr != vesting_balance_idx.end(); ++itr)
        {
            total_supply += itr->balance;
        }

        const auto& research_token_sale_contribution_idx = get_index<research_token_sale_contribution_index, by_id>();
        for (auto itr = research_token_sale_contribution_idx.begin(); itr != research_token_sale_contribution_idx.end(); ++itr)
        {
            total_supply += itr->amount;
        }

        const auto& discipline_supply_idx = get_index<discipline_supply_index, by_id>();
        for (auto itr = discipline_supply_idx.begin(); itr != discipline_supply_idx.end(); ++itr)
        {
            total_supply += itr->balance;
        }

        const auto& grant_idx = get_index<grant_index, by_id>();
        for (auto itr = grant_idx.begin(); itr != grant_idx.end(); ++itr)
        {
            total_supply += itr->amount;
        }

        total_supply += gpo.common_tokens_fund;

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

} // namespace chain
} // namespace deip
