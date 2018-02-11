#pragma once
#include <deip/chain/deip_object_types.hpp>

namespace deip {
namespace chain {

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

struct by_research_token_sale_id;
struct by_owner;

typedef multi_index_container<research_token_sale_contribution_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<research_token_sale_contribution_object,
                        research_token_sale_contribution_id_type,
                        &research_token_sale_contribution_object::id>>,
                ordered_non_unique<tag<by_research_token_sale_id>,
                        member<research_token_sale_contribution_object,
                                research_token_sale_id_type,
                                &research_token_sale_contribution_object::research_token_sale_id>>,
                ordered_non_unique<tag<by_owner>,
                        member<research_token_sale_contribution_object,
                                account_name_type,
                                &research_token_sale_contribution_object::owner>>>,
        allocator<research_token_sale_contribution_object>>
        research_token_sale_contribution_index;

} // namespace chain
} // namespace deip

FC_REFLECT(deip::chain::research_token_sale_contribution_object, (id)(research_token_sale_id)(owner)(amount)(contribution_time))

CHAINBASE_SET_INDEX_TYPE(deip::chain::research_token_sale_contribution_object, deip::chain::research_token_sale_contribution_index)