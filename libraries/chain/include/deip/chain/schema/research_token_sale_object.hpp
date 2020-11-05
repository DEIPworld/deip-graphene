#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include "deip_object_types.hpp"

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <numeric>

#include <vector>

namespace deip {
namespace chain {

using fc::time_point;
using protocol::external_id_type;
using protocol::asset_symbol_type;

enum class research_token_sale_status : uint16_t
{
    active = 1,
    finished = 2,
    expired = 3,
    inactive = 4
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
    external_id_type external_id;

    research_id_type research_id;
    external_id_type research_external_id;
    flat_set<asset> security_tokens_on_sale;
    time_point_sec start_time;
    time_point_sec end_time;
    protocol::asset total_amount;
    protocol::asset soft_cap;
    protocol::asset hard_cap;
    uint16_t status;
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
    external_id_type research_token_sale;
    research_token_sale_id_type research_token_sale_id;
    account_name_type owner;
    protocol::asset amount;
    fc::time_point_sec contribution_time;
};

struct by_external_id;
struct by_research_id;
struct by_research;
struct by_end_time;
struct by_research_id_and_status;

typedef multi_index_container<research_token_sale_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<research_token_sale_object,
                        research_token_sale_id_type,
                        &research_token_sale_object::id>>,
                ordered_unique<tag<by_external_id>,
                        member<research_token_sale_object,
                                external_id_type,
                                &research_token_sale_object::external_id>>,
                ordered_non_unique<tag<by_research_id>,
                        member<research_token_sale_object,
                                research_id_type,
                                &research_token_sale_object::research_id>>,
                ordered_non_unique<tag<by_research>,
                        member<research_token_sale_object,
                                external_id_type,
                                &research_token_sale_object::research_external_id>>,
                ordered_non_unique<tag<by_end_time>,
                        member<research_token_sale_object,
                                fc::time_point_sec,
                                &research_token_sale_object::end_time>>,
                ordered_non_unique<tag<by_research_id_and_status>,
                composite_key<research_token_sale_object,
                        member<research_token_sale_object,
                               research_id_type,
                               &research_token_sale_object::research_id>,
                        member<research_token_sale_object,
                               uint16_t,
                               &research_token_sale_object::status>>>>,
        allocator<research_token_sale_object>>
        research_token_sale_index;

struct by_research_token_sale;
struct by_research_token_sale_id;
struct by_owner_and_research_token_sale_id;
struct by_owner;

typedef multi_index_container<research_token_sale_contribution_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<research_token_sale_contribution_object,
                        research_token_sale_contribution_id_type,
                        &research_token_sale_contribution_object::id>>,
                ordered_non_unique<tag<by_research_token_sale>,
                        member<research_token_sale_contribution_object,
                                external_id_type,
                                &research_token_sale_contribution_object::research_token_sale>>,
                ordered_non_unique<tag<by_research_token_sale_id>,
                        member<research_token_sale_contribution_object,
                                research_token_sale_id_type,
                                &research_token_sale_contribution_object::research_token_sale_id>>,
                ordered_non_unique<tag<by_owner>,
                        member<research_token_sale_contribution_object,
                               account_name_type,
                               &research_token_sale_contribution_object::owner>>,
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

FC_REFLECT_ENUM(deip::chain::research_token_sale_status, 
  (active)
  (finished)
  (expired)
  (inactive)
)

FC_REFLECT(deip::chain::research_token_sale_object, (id)(external_id)(research_id)(research_external_id)(security_tokens_on_sale)(start_time)(end_time)(total_amount)(soft_cap)(hard_cap)(status))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_token_sale_object, deip::chain::research_token_sale_index)

FC_REFLECT(deip::chain::research_token_sale_contribution_object, (id)(research_token_sale)(research_token_sale_id)(owner)(amount)(contribution_time))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_token_sale_contribution_object, deip::chain::research_token_sale_contribution_index)