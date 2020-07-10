#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/asset.hpp>
#include <deip/protocol/eci_diff.hpp>
#include <fc/utf8.hpp>

namespace deip {
namespace protocol {

struct fill_common_tokens_withdraw_operation : public virtual_operation
{
    fill_common_tokens_withdraw_operation() {}
    fill_common_tokens_withdraw_operation(const string& f, const string& t, const share_type& w, const share_type& d, const bool tr)
        : from_account(f)
        , to_account(t)
        , withdrawn(w)
        , deposited(d)
        , transfer(tr)
    {
    }

    account_name_type from_account;
    account_name_type to_account;
    share_type withdrawn;
    share_type deposited;
    bool transfer;
};

struct shutdown_witness_operation : public virtual_operation
{
    shutdown_witness_operation() {}
    shutdown_witness_operation(const string& o)
        : owner(o)
    {
    }

    account_name_type owner;
};

struct hardfork_operation : public virtual_operation
{
    hardfork_operation() {}
    hardfork_operation(uint32_t hf_id)
        : hardfork_id(hf_id)
    {
    }

    uint32_t hardfork_id = 0;
};

struct producer_reward_operation : public virtual_operation
{
    producer_reward_operation() {}
    producer_reward_operation(const string& p, const share_type c)
        : producer(p)
        , common_tokens_amount(c)
    {
    }

    account_name_type producer;
    share_type common_tokens_amount;
};

struct token_sale_contribution_to_history_operation : public virtual_operation
{
    token_sale_contribution_to_history_operation() {}
    token_sale_contribution_to_history_operation(const int64_t& research_id, const int64_t& research_token_sale_id, const string& contributor, const asset& amount)
        : research_id(research_id)
        , research_token_sale_id(research_token_sale_id)
        , contributor(contributor)
        , amount(amount)
    {
    }

    int64_t research_id;
    int64_t research_token_sale_id;
    account_name_type contributor;
    asset amount;
};

struct research_content_reference_history_operation : public virtual_operation
{
    research_content_reference_history_operation() {}
    research_content_reference_history_operation(const int64_t& research_content_id, 
                                                 const int64_t& research_id,
                                                 const std::string& content, 
                                                 const int64_t& research_content_reference_id,
                                                 const int64_t& research_reference_id,
                                                 const std::string& content_reference)
        : research_content_id(research_content_id)
        , research_id(research_id)
        , content(content)
        , research_content_reference_id(research_content_reference_id)
        , research_reference_id(research_reference_id)
        , content_reference(content_reference)
    {
    }

    int64_t research_content_id;
    int64_t research_id;
    std::string content;
    int64_t research_content_reference_id;
    int64_t research_reference_id;
    std::string content_reference;
};

struct research_content_eci_history_operation : public virtual_operation
{
    research_content_eci_history_operation() {}
    research_content_eci_history_operation(const int64_t& research_content_id,
                                           const int64_t& discipline_id,
                                           const eci_diff& diff)
        : research_content_id(research_content_id)
        , discipline_id(discipline_id)
        , diff(diff)
    {
    }

    int64_t research_content_id;
    int64_t discipline_id;
    eci_diff diff;
};

struct research_eci_history_operation : public virtual_operation
{
    research_eci_history_operation() {}
    research_eci_history_operation(const int64_t& research_id,
                                   const int64_t& discipline_id,
                                   const eci_diff& diff)
        : research_id(research_id)
        , discipline_id(discipline_id)
        , diff(diff)
    {
    }

    int64_t research_id;
    int64_t discipline_id;
    eci_diff diff;
};

struct account_eci_history_operation : public virtual_operation
{
    account_eci_history_operation() {}
    account_eci_history_operation(const account_name_type& account,
                                  const int64_t& discipline_id,
                                  const uint16_t& recipient_type,
                                  const eci_diff& diff)
        : account(account)
        , discipline_id(discipline_id)
        , recipient_type(recipient_type)
        , diff(diff)
    {
    }

    account_name_type account;
    int64_t discipline_id;
    uint16_t recipient_type;
    eci_diff diff;
};

struct disciplines_eci_history_operation : public virtual_operation
{
    disciplines_eci_history_operation(){}
    disciplines_eci_history_operation(const flat_map<int64_t, vector<eci_diff>>& contributions_map, const fc::time_point_sec& timestamp)
        : timestamp(timestamp)
    {
        for (const auto& pair : contributions_map)
        {
            contributions.insert(pair);
        }
    }

    flat_map<int64_t, vector<eci_diff>> contributions;
    fc::time_point_sec timestamp;
};


}
} // deip::protocol

FC_REFLECT(deip::protocol::fill_common_tokens_withdraw_operation, (from_account)(to_account)(withdrawn)(deposited)(transfer))
FC_REFLECT(deip::protocol::shutdown_witness_operation, (owner))
FC_REFLECT(deip::protocol::hardfork_operation, (hardfork_id))
FC_REFLECT(deip::protocol::producer_reward_operation, (producer)(common_tokens_amount))
FC_REFLECT(deip::protocol::token_sale_contribution_to_history_operation, (research_id)(research_token_sale_id)(contributor)(amount))
FC_REFLECT(deip::protocol::research_content_reference_history_operation, (research_content_id)(research_id)(content)(research_content_reference_id)(research_reference_id)(content_reference))
FC_REFLECT(deip::protocol::research_content_eci_history_operation, (research_content_id)(discipline_id)(diff))
FC_REFLECT(deip::protocol::research_eci_history_operation, (research_id)(discipline_id)(diff))
FC_REFLECT(deip::protocol::account_eci_history_operation, (account)(discipline_id)(recipient_type)(diff))
FC_REFLECT(deip::protocol::disciplines_eci_history_operation, (contributions)(timestamp))