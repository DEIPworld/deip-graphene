#pragma once

#include <fc/fixed_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include "deip_object_types.hpp"
#include "shared_authority.hpp"

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {

using deip::protocol::asset;

class offer_research_tokens_object : public object<offer_research_tokens_object_type, offer_research_tokens_object>
{
    offer_research_tokens_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    offer_research_tokens_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    offer_research_tokens_id_type id;

    account_name_type sender;
    account_name_type receiver;

    research_id_type research_id;
    share_type amount = 0;
    asset price = asset(0, DEIP_SYMBOL);
};

struct by_receiver;
struct by_research_id;
struct by_receiver_and_research_id;

typedef multi_index_container<offer_research_tokens_object,
            indexed_by<ordered_unique<tag<by_id>,
                    member<offer_research_tokens_object,
                            offer_research_tokens_id_type,
                           &offer_research_tokens_object::id>>,
            ordered_non_unique<tag<by_receiver>,
                    member<offer_research_tokens_object,
                           account_name_type,
                           &offer_research_tokens_object::receiver>>,
            ordered_unique<tag<by_receiver_and_research_id>,
                    composite_key<offer_research_tokens_object,
                            member<offer_research_tokens_object,
                                   account_name_type,
                                   &offer_research_tokens_object::receiver>,
                            member<offer_research_tokens_object,
                                   research_id_type,
                                   &offer_research_tokens_object::research_id>>>,
            ordered_non_unique<tag<by_research_id>,
                    member<offer_research_tokens_object,
                           research_id_type,
                           &offer_research_tokens_object::research_id>>>,
        allocator<expert_token_object>>
        offer_research_tokens_index;
    }
}

FC_REFLECT( deip::chain::offer_research_tokens_object,
            (id)(sender)(receiver)(research_id)(amount)(price)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::offer_research_tokens_object, deip::chain::offer_research_tokens_index )