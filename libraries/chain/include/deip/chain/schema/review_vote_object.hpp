#pragma once

#include <fc/fixed_string.hpp>

#include "../../../../../protocol/include/deip/protocol/authority.hpp"
#include "../../../../../protocol/include/deip/protocol/deip_operations.hpp"

#include "deip_object_types.hpp"
#include "shared_authority.hpp"

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {

using deip::protocol::asset;


class review_vote_object : public object<review_vote_object_type, review_vote_object>
{
    review_vote_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    review_vote_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    review_vote_id_type id;
    discipline_id_type discipline_id;
    account_name_type voter;
    review_id_type review_id;
    int64_t weight;
    time_point_sec voting_time;
};

struct by_discipline_id;
struct by_review_id;
struct by_voter;
struct by_review_and_discipline;
struct by_voter_discipline_and_review;

typedef multi_index_container<review_vote_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<review_vote_object,
                        review_vote_id_type,
                        &review_vote_object::id>>,
                ordered_non_unique<tag<by_discipline_id>,
                        member<review_vote_object,
                                discipline_id_type,
                                &review_vote_object::discipline_id>>,
                ordered_non_unique<tag<by_review_id>,
                        member<review_vote_object,
                                review_id_type,
                                &review_vote_object::review_id>>,
                ordered_non_unique<tag<by_review_and_discipline>,
                        composite_key<review_vote_object,
                                member<review_vote_object,
                                        review_id_type,
                                        &review_vote_object::review_id>,
                                member<review_vote_object,
                                        discipline_id_type,
                                        &review_vote_object::discipline_id>>>,
                ordered_unique<tag<by_voter_discipline_and_review>,
                        composite_key<review_vote_object,
                                member<review_vote_object,
                                        account_name_type,
                                        &review_vote_object::voter>,
                                member<review_vote_object,
                                        discipline_id_type,
                                        &review_vote_object::discipline_id>,
                                member<review_vote_object,
                                        review_id_type,
                                        &review_vote_object::review_id>>>,
                ordered_non_unique<tag<by_voter>,
                        member<review_vote_object,
                                account_name_type,
                                &review_vote_object::voter>>>,
        allocator<review_vote_object>>
        review_vote_index;
}
}

FC_REFLECT( deip::chain::review_vote_object,
            (id)(discipline_id)(voter)(review_id)(weight)(voting_time)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::review_vote_object, deip::chain::review_vote_index )

