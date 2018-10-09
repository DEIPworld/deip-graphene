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

class expertise_allocation_proposal_vote_object : public object<expertise_allocation_proposal_vote_object_type, expertise_allocation_proposal_vote_object>
{
    expertise_allocation_proposal_vote_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    expertise_allocation_proposal_vote_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    expertise_allocation_proposal_vote_id_type id;
    expertise_allocation_proposal_id_type expertise_allocation_proposal_id;
    discipline_id_type discipline_id;

    account_name_type voter;
    share_type weight;

    time_point_sec voting_time;
};

struct by_expertise_allocation_proposal_id;
struct by_voter_and_expertise_allocation_proposal_id;
struct by_voter_and_discipline_id;
struct by_voter;


typedef multi_index_container<expertise_allocation_proposal_vote_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<expertise_allocation_proposal_vote_object,
                        expertise_allocation_proposal_vote_id_type,
                        &expertise_allocation_proposal_vote_object::id>>,
                ordered_non_unique<tag<by_expertise_allocation_proposal_id>,
                        member<expertise_allocation_proposal_vote_object,
                               expertise_allocation_proposal_id_type,
                              &expertise_allocation_proposal_vote_object::expertise_allocation_proposal_id>>,
                ordered_unique<tag<by_voter_and_expertise_allocation_proposal_id>,
                composite_key<expertise_allocation_proposal_vote_object,
                        member<expertise_allocation_proposal_vote_object,
                               account_name_type,
                               &expertise_allocation_proposal_vote_object::voter>,
                        member<expertise_allocation_proposal_vote_object,
                               expertise_allocation_proposal_id_type,
                               &expertise_allocation_proposal_vote_object::expertise_allocation_proposal_id>>>,
                ordered_unique<tag<by_voter_and_discipline_id>,
                composite_key<expertise_allocation_proposal_vote_object,
                        member<expertise_allocation_proposal_vote_object,
                               account_name_type,
                               &expertise_allocation_proposal_vote_object::voter>,
                        member<expertise_allocation_proposal_vote_object,
                                discipline_id_type,
                               &expertise_allocation_proposal_vote_object::discipline_id>>>,
                ordered_non_unique<tag<by_voter>,
                        member<expertise_allocation_proposal_vote_object,
                                account_name_type,
                                &expertise_allocation_proposal_vote_object::voter>>>,
        allocator<expertise_allocation_proposal_vote_object>>
        expertise_allocation_proposal_vote_index;
}
}

FC_REFLECT( deip::chain::expertise_allocation_proposal_vote_object,
            (id)(expertise_allocation_proposal_id)(discipline_id)(voter)(weight)(voting_time)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::expertise_allocation_proposal_vote_object, deip::chain::expertise_allocation_proposal_vote_index )

