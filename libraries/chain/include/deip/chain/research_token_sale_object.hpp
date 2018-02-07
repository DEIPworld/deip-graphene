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

    research_token_id_type research_token_id;
    research_id_type research_id;
    fc::time_point start_time
    fc::time_point end_time;
    share_type total_amount;
    share_type balance_tokens;
    share_type soft_cap;
    share_type hard_cap;
};

} // namespace chain
} // namespace deip

FC_REFLECT(deip::chain::research_token_sale_object, (id)(research_token_id)(research_id)(start_time)(end_time)(total_amount)(balance_tokens)(soft_cap)(hard_cap))
