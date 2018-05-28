#pragma once

#include <deip/chain/deip_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {

class research_token_object : public object<research_token_object_type, research_token_object>
{
    research_token_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    research_token_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    research_token_id_type id;
    account_name_type account_name;
    research_id_type research_id;
    share_type amount = 0;

};

struct by_account_name;
struct by_research_id;
struct by_account_name_and_research_id;

typedef multi_index_container<research_token_object,
                indexed_by<ordered_unique<tag<by_id>,
                                   member<research_token_object,
                                   research_token_id_type,
                                   &research_token_object::id>>,
                    ordered_non_unique<tag<by_account_name>,
                            member<research_token_object,
                                   account_name_type,
                                   &research_token_object::account_name>>,
                    ordered_non_unique<tag<by_research_id>,
                            member<research_token_object,
                                   research_id_type,
                                   &research_token_object::research_id>>,
                    ordered_unique<tag<by_account_name_and_research_id>,
                            composite_key<research_token_object,
                            member<research_token_object,
                                   account_name_type,
                                   &research_token_object::account_name>,
                            member<research_token_object,
                                   research_id_type,
                                   &research_token_object::research_id>>>>,
                allocator<research_token_object>>
                research_token_index;
    }
}

FC_REFLECT( deip::chain::research_token_object, (id)(account_name)(research_id)(amount))

CHAINBASE_SET_INDEX_TYPE( deip::chain::research_token_object, deip::chain::research_token_index )