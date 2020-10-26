#pragma once

#include "deip_object_types.hpp"

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {

using protocol::external_id_type;

class security_token_object : public object<security_token_object_type, security_token_object>
{
    security_token_object() = delete;

public:
    template <typename Constructor, typename Allocator>
    security_token_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    security_token_id_type id;
    external_id_type external_id;
    external_id_type research_external_id;
    uint32_t total_amount;
};

struct by_research;
struct by_external_id;

typedef multi_index_container<security_token_object,
                indexed_by<ordered_unique<tag<by_id>,
                            member<security_token_object,
                                   security_token_id_type,
                                   &security_token_object::id>>,
                    ordered_non_unique<tag<by_research>,
                            member<security_token_object,
                                   external_id_type,
                                   &security_token_object::research_external_id>>,
                    ordered_non_unique<tag<by_external_id>,
                            member<security_token_object,
                                   external_id_type,
                                   &security_token_object::external_id>>>,
                allocator<security_token_object>>
                security_token_index;
    }
}


FC_REFLECT(deip::chain::security_token_object, (id)(external_id)(research_external_id)(total_amount))

CHAINBASE_SET_INDEX_TYPE(deip::chain::security_token_object, deip::chain::security_token_index)