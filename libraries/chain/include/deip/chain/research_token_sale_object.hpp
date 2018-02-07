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
    fc::time_point start_time;
    fc::time_point end_time;
    share_type total_amount;
    share_type balance_tokens;
    share_type soft_cap;
    share_type hard_cap;
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
                                fc::time_point,
                                &research_token_sale_object::end_time>>>,
        allocator<research_token_sale_object>>
        research_token_sale_index;

} // namespace chain
} // namespace deip

FC_REFLECT(deip::chain::research_token_sale_object, (id)(research_id)(start_time)(end_time)(total_amount)(balance_tokens)(soft_cap)(hard_cap))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_token_sale_object, deip::chain::research_token_sale_index)