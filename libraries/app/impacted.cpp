/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <deip/protocol/authority.hpp>
#include <deip/protocol/operations/create_grant_operation.hpp>

#include <deip/app/impacted.hpp>

#include <fc/utility.hpp>

namespace deip {
namespace app {

using namespace fc;
using namespace deip::protocol;

// TODO:  Review all of these, especially no-ops
struct get_impacted_account_visitor
{
    flat_set<account_name_type>& _impacted;
    get_impacted_account_visitor(flat_set<account_name_type>& impact)
        : _impacted(impact)
    {
    }
    typedef void result_type;

    template <typename T> void operator()(const T& op)
    {
        op.get_required_posting_authorities(_impacted);
        op.get_required_active_authorities(_impacted);
        op.get_required_owner_authorities(_impacted);
    }

    // ops
    void operator()(const create_account_operation& op)
    {
        _impacted.insert(op.new_account_name);
        _impacted.insert(op.creator);
    }

    void operator()(const update_account_operation& op)
    {
        _impacted.insert(op.account);
    }

    void operator()(const vote_for_review_operation& op)
    {
        _impacted.insert(op.voter);
    }

    void operator()(const transfer_operation& op)
    {
        _impacted.insert(op.from);
        _impacted.insert(op.to);
    }

    void operator()(const transfer_to_common_tokens_operation& op)
    {
        _impacted.insert(op.from);

        if (op.to != account_name_type() && op.to != op.from)
        {
            _impacted.insert(op.to);
        }
    }

    void operator()(const withdraw_common_tokens_operation& op)
    {
        _impacted.insert(op.account);
    }

    void operator()(const set_withdraw_common_tokens_route_operation& op)
    {
        _impacted.insert(op.from_account);
        _impacted.insert(op.to_account);
    }

    void operator()(const witness_update_operation& op)
    {
        _impacted.insert(op.owner);
    }

    void operator()(const account_witness_vote_operation& op)
    {
        _impacted.insert(op.account);
        _impacted.insert(op.witness);
    }

    void operator()(const account_witness_proxy_operation& op)
    {
        _impacted.insert(op.account);
        _impacted.insert(op.proxy);
    }

    void operator()(const request_account_recovery_operation& op)
    {
        _impacted.insert(op.account_to_recover);
        _impacted.insert(op.recovery_account);
    }

    void operator()(const recover_account_operation& op)
    {
        _impacted.insert(op.account_to_recover);
    }

    void operator()(const change_recovery_account_operation& op)
    {
        _impacted.insert(op.account_to_recover);
    }

    void operator()(const create_proposal_operation& op)
    {
        _impacted.insert(op.creator);
    }

    void operator()(const make_review_operation& op)
    {
        _impacted.insert(op.author);
    }

    void operator()(const contribute_to_token_sale_operation& op)
    {
        _impacted.insert(op.contributor);
    }

    void operator()(const transfer_research_tokens_to_research_group_operation& op)
    {
        _impacted.insert(op.owner);
    }

    void operator()(const create_vesting_balance_operation& op)
    {
        _impacted.insert(op.creator);
        _impacted.insert(op.owner);
    }

    void operator()(const withdraw_vesting_balance_operation& op)
    {
        _impacted.insert(op.owner);
    }

    void operator()(const transfer_research_tokens_operation& op)
    {
        _impacted.insert(op.sender);
        _impacted.insert(op.receiver);
    }
    
    void operator()(const delegate_expertise_operation& op)
    {
        _impacted.insert(op.sender);
        _impacted.insert(op.receiver);
    }

    void operator()(const revoke_expertise_delegation_operation& op)
    {
        _impacted.insert(op.sender);
    }
    
    void operator()(const create_expertise_allocation_proposal_operation& op)
    {
        _impacted.insert(op.claimer);
    }
    
    void operator()(const vote_for_expertise_allocation_proposal_operation& op)
    {
        _impacted.insert(op.voter);
    }
    
    void operator()(const create_grant_operation& op)
    {
        _impacted.insert(op.grantor);
    }

    void operator()(const create_grant_application_operation& op)
    {
        _impacted.insert(op.creator);
    }

    void operator()(const create_asset_operation& op)
    {
        _impacted.insert(op.issuer);
    }

    void operator()(const issue_asset_operation& op)
    {
        _impacted.insert(op.issuer);
    }
    
    // virtual operations

    void operator()(const fill_common_tokens_withdraw_operation& op)
    {
        _impacted.insert(op.from_account);
        _impacted.insert(op.to_account);
    }

    void operator()(const shutdown_witness_operation& op)
    {
        _impacted.insert(op.owner);
    }

    void operator()(const producer_reward_operation& op)
    {
        _impacted.insert(op.producer);
    }
};

void operation_get_impacted_accounts(const operation& op, flat_set<account_name_type>& result)
{
    get_impacted_account_visitor vtor = get_impacted_account_visitor(result);
    op.visit(vtor);
}

void transaction_get_impacted_accounts(const transaction& tx, flat_set<account_name_type>& result)
{
    for (const auto& op : tx.operations)
        operation_get_impacted_accounts(op, result);
}
}
}
