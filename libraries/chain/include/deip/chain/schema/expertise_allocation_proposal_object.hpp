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

using deip::protocol::percent_type;

class expertise_allocation_proposal_object : public object<expertise_allocation_proposal_object_type, expertise_allocation_proposal_object>
{
    expertise_allocation_proposal_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    expertise_allocation_proposal_object(Constructor&& c, allocator<Allocator> a) : description(a)
    {
        c(*this);
    }

    expertise_allocation_proposal_id_type id;
    account_name_type claimer;
    discipline_id_type discipline_id;

    int64_t total_voted_expertise = 0;

    percent_type quorum;

    time_point_sec creation_time;
    time_point_sec expiration_time;

    fc::shared_string description;
};

struct by_claimer;
struct by_expiration_time;
struct by_claimer_and_discipline;
struct by_discipline_id;

typedef multi_index_container<expertise_allocation_proposal_object,
            indexed_by<ordered_unique<tag<by_id>,
                    member<expertise_allocation_proposal_object,
                            expertise_allocation_proposal_id_type,
                           &expertise_allocation_proposal_object::id>>,
            ordered_non_unique<tag<by_claimer>,
                    member<expertise_allocation_proposal_object,
                           account_name_type,
                           &expertise_allocation_proposal_object::claimer>>,
            ordered_non_unique<tag<by_expiration_time>,
                    member<expertise_allocation_proposal_object,
                           time_point_sec,
                           &expertise_allocation_proposal_object::expiration_time>>,
            ordered_unique<tag<by_claimer_and_discipline>,
                    composite_key<expertise_allocation_proposal_object,                                   
                            member<expertise_allocation_proposal_object,
                                   account_name_type,
                                   &expertise_allocation_proposal_object::claimer>,
                            member<expertise_allocation_proposal_object,
                                   discipline_id_type,
                                   &expertise_allocation_proposal_object::discipline_id>>>,
            ordered_non_unique<tag<by_discipline_id>,
                    member<expertise_allocation_proposal_object,
                           discipline_id_type,
                           &expertise_allocation_proposal_object::discipline_id>>>,
        allocator<expertise_allocation_proposal_object>>
        expertise_allocation_proposal_index;
    }
}

FC_REFLECT( deip::chain::expertise_allocation_proposal_object,
            (id)(claimer)(discipline_id)(total_voted_expertise)
                    (quorum)(creation_time)(expiration_time)(description)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::expertise_allocation_proposal_object, deip::chain::expertise_allocation_proposal_index )