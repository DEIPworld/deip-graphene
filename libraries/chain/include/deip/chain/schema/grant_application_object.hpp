#pragma once

#include "deip_object_types.hpp"

namespace deip {
namespace chain {

enum grant_application_status : uint16_t
{
    application_pending = 1,
    application_approved = 2,
    application_rejected = 3
};

class grant_application_object : public object<grant_application_object_type, grant_application_object>
{
    grant_application_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    grant_application_object(Constructor&& c, allocator<Allocator> a) : application_hash(a)
    {
        c(*this);
    }

    grant_application_id_type id;
    grant_id_type grant_id;
    research_id_type research_id;
    fc::shared_string application_hash;

    account_name_type creator;

    fc::time_point_sec created_at;

    grant_application_status status;
};

struct by_grant_id;
struct by_status;
struct by_research_id;

typedef multi_index_container<grant_application_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<grant_application_object,
                                                                grant_application_id_type,
                                                               &grant_application_object::id>>,
                                         ordered_non_unique<tag<by_grant_id>,
                                                        member<grant_application_object,
                                                                grant_id_type,
                                                               &grant_application_object::grant_id>>,
                                         ordered_non_unique<tag<by_status>,
                                                        member<grant_application_object,
                                                                grant_application_status,
                                                               &grant_application_object::status>>,
                                         ordered_non_unique<tag<by_research_id>,
                                                        member<grant_application_object,
                                                                research_id_type,
                                                               &grant_application_object::research_id>>>,
                              allocator<grant_application_object>>
    grant_application_index;

}
}

FC_REFLECT_ENUM(deip::chain::grant_application_status, (application_pending)(application_approved)(application_rejected))

FC_REFLECT( deip::chain::grant_application_object,
             (id)(grant_id)(research_id)(application_hash)(creator)(created_at)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::grant_application_object, deip::chain::grant_application_index )

