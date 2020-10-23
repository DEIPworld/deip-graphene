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

    account_name_type owner;
    external_id_type security_token_external_id;
    external_id_type research_external_id;
    uint32_t amount;
};

struct by_owner;
struct by_research;
struct by_security_token;
struct by_owner_and_research;
struct by_owner_and_security_token;

typedef multi_index_container<security_token_object,
                indexed_by<ordered_unique<tag<by_id>,
                            member<security_token_object,
                                   security_token_id_type,
                                   &security_token_object::id>>,
                    ordered_non_unique<tag<by_owner>,
                            member<security_token_object,
                                   account_name_type,
                                   &security_token_object::owner>>,
                    ordered_non_unique<tag<by_research>,
                            member<security_token_object,
                                   external_id_type,
                                   &security_token_object::research_external_id>>,
                    ordered_non_unique<tag<by_security_token>,
                            member<security_token_object,
                                   external_id_type,
                                   &security_token_object::security_token_external_id>>,
                    ordered_non_unique<tag<by_owner_and_research>,
                            composite_key<security_token_object,
                            member<security_token_object,
                                   account_name_type,
                                   &security_token_object::owner>,
                            member<security_token_object,
                                   external_id_type,
                                   &security_token_object::research_external_id>>>,
                    ordered_unique<tag<by_owner_and_security_token>,
                            composite_key<security_token_object,
                            member<security_token_object,
                                   account_name_type,
                                   &security_token_object::owner>,
                            member<security_token_object,
                                   external_id_type,
                                   &security_token_object::security_token_external_id>>>>,
                allocator<security_token_object>>
                security_token_index;
    }
}


FC_REFLECT(deip::chain::security_token_object, (id)(owner)(security_token_external_id)(research_external_id)(amount))

CHAINBASE_SET_INDEX_TYPE(deip::chain::security_token_object, deip::chain::security_token_index)