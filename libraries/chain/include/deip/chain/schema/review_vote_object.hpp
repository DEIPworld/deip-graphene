#pragma once

#include <fc/fixed_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include "deip_object_types.hpp"

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {

using deip::protocol::asset;
using deip::protocol::external_id_type;

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
    external_id_type external_id;
    external_id_type discipline_external_id;
    discipline_id_type discipline_id;
    account_name_type voter;
    external_id_type review_external_id;
    review_id_type review_id;
    int64_t weight;
    time_point_sec voting_time;
    research_content_id_type research_content_id;
    research_id_type research_id;
};

struct by_discipline_id;
struct by_external_id;
struct by_review_id;
struct by_voter;
struct by_review_and_discipline;
struct by_voter_discipline_and_review;
struct by_research_content;
struct by_research;
struct by_review_external_id;

typedef multi_index_container<review_vote_object,
        indexed_by<

                ordered_unique<tag<by_id>,
                        member<review_vote_object,
                              review_vote_id_type,
                              &review_vote_object::id>>,

                ordered_unique<tag<by_external_id>,
                        member<review_vote_object,
                              external_id_type,
                              &review_vote_object::external_id>>,

                ordered_non_unique<tag<by_discipline_id>,
                        member<review_vote_object,
                                discipline_id_type,
                                &review_vote_object::discipline_id>>,

                ordered_non_unique<tag<by_review_id>,
                        member<review_vote_object,
                                review_id_type,
                                &review_vote_object::review_id>>,

                ordered_non_unique<tag<by_review_external_id>,
                        member<review_vote_object,
                                external_id_type,
                                &review_vote_object::review_external_id>>,

                ordered_non_unique<tag<by_research_content>,
                        member<review_vote_object,
                                research_content_id_type,
                                &review_vote_object::research_content_id>>,
                ordered_non_unique<tag<by_research>,
                        member<review_vote_object,
                                research_id_type,
                                &review_vote_object::research_id>>,
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
  (id)
  (external_id)
  (discipline_external_id)
  (discipline_id)
  (voter)
  (review_external_id)
  (review_id)
  (weight)
  (voting_time)
  (research_content_id)
  (research_id)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::review_vote_object, deip::chain::review_vote_index )

