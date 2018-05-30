#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/shared_authority.hpp>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

#include <vector>

namespace deip {
namespace chain {

using fc::time_point;

enum research_token_sale_status : uint16_t
{
    token_sale_active = 1,
    token_sale_finished = 2,
    token_sale_expired = 3
};

class research_token_sale_object : public object<research_token_sale_object_type, research_token_sale_object>
{

    research_token_sale_object() = delete;

public:
    template <typename Constructor, typename Allocator> research_token_sale_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    research_token_sale_id_type id;

    research_id_type research_id;
    time_point_sec start_time;
    time_point_sec end_time;
    share_type total_amount;
    share_type balance_tokens;
    share_type soft_cap;
    share_type hard_cap;
    research_token_sale_status status;
};

class research_token_sale_contribution_object : public object<research_token_sale_contribution_object_type, research_token_sale_contribution_object>
{
    research_token_sale_contribution_object() = delete;
public:
    template <typename Constructor, typename Allocator> research_token_sale_contribution_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    research_token_sale_contribution_id_type id;
    research_token_sale_id_type research_token_sale_id;
    account_name_type owner;
    share_type amount;
    fc::time_point_sec contribution_time;
};

struct by_research_id;
struct by_end_time;

typedef multi_index_container<research_token_sale_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<research_token_sale_object,
                        research_token_sale_id_type,
                        &research_token_sale_object::id>>,
                ordered_unique<tag<by_research_id>,
                        member<research_token_sale_object,
                                research_id_type,
                                &research_token_sale_object::research_id>>,
                ordered_non_unique<tag<by_end_time>,
                        member<research_token_sale_object,
                                fc::time_point_sec,
                                &research_token_sale_object::end_time>>>,
        allocator<research_token_sale_object>>
        research_token_sale_index;

struct by_research_token_sale_id;
struct by_owner_and_research_token_sale_id;

typedef multi_index_container<research_token_sale_contribution_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<research_token_sale_contribution_object,
                        research_token_sale_contribution_id_type,
                        &research_token_sale_contribution_object::id>>,
                ordered_non_unique<tag<by_research_token_sale_id>,
                        member<research_token_sale_contribution_object,
                                research_token_sale_id_type,
                                &research_token_sale_contribution_object::research_token_sale_id>>,
                ordered_unique<tag<by_owner_and_research_token_sale_id>,
                composite_key<research_token_sale_contribution_object,
                        member<research_token_sale_contribution_object,
                               account_name_type,
                               &research_token_sale_contribution_object::owner>,
                        member<research_token_sale_contribution_object,
                               research_token_sale_id_type,
                               &research_token_sale_contribution_object::research_token_sale_id>>>>,
        allocator<research_token_sale_contribution_object>>
        research_token_sale_contribution_index;

} // namespace chain
} // namespace deip

FC_REFLECT_ENUM(deip::chain::research_token_sale_status, (token_sale_active)(token_sale_finished)(token_sale_expired))

FC_REFLECT(deip::chain::research_token_sale_object, (id)(research_id)(start_time)(end_time)(total_amount)(balance_tokens)(soft_cap)(hard_cap))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_token_sale_object, deip::chain::research_token_sale_index)

FC_REFLECT(deip::chain::research_token_sale_contribution_object, (id)(research_token_sale_id)(owner)(amount)(contribution_time))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_token_sale_contribution_object, deip::chain::research_token_sale_contribution_index)