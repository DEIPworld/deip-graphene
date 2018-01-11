#pragma once

#include <fc/fixed_string.hpp>

#include <deip/protocol/authority.hpp>
#include <deip/protocol/deip_operations.hpp>

#include <deip/chain/deip_object_types.hpp>
#include <deip/chain/shared_authority.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace deip {
namespace chain {

using deip::protocol::asset;

class vote_object : public object<vote_object_type, vote_object>
{
    vote_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    vote_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    vote_id_type id;
    discipline_id_type discipline_id;
    account_name_type voter;
    optional<id_type> research_id;
    optional<id_type> material_id;
    optional<id_type> review_id;
    share_type weight;
    time_point_sec voting_time;
};

struct by_discipline_id;
struct by_discpline_id_voting_time;
struct by_discpline_id_voting_time_voter;
struct by_research_id;
struct by_research_id_voting_time;
struct by_research_id_voting_time_voter;
struct by_material_id;
struct by_material_id_voting_time;
struct by_material_id_voting_time_voter;
struct by_review_id;
struct by_review_id_voting_time;
struct by_review_id_voting_time_voter;

typedef multi_index_container<vote_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<vote_object,
                        vote_id_type,
                        &vote_object::id>>,
                ordered_non_unique<tag<by_discipline_id>,
                        member<vote_object,
                                discipline_id_type,
                                &vote_object::discipline_id>>,
                ordered_non_unique<tag<by_discpline_id_voting_time>,
                        composite_key<vote_object,
                                member<vote_object,
                                        discipline_id_type,
                                        &vote_object::discipline_id>,
                                member<vote_object,
                                        time_point_sec,
                                        &vote_object::voting_time>,
                                member<vote_object,
                                        vote_id_type,
                                        &vote_object::id>>,
                        composite_key_compare<std::less<discipline_id_type>,
                                std::greater<time_point_sec>,
                                std::less<vote_id_type>>>>,
        allocator<vote_object>>
        vote_index;
}
}

FC_REFLECT( deip::chain::vote_object,
            (id)(discipline_id)(voter)(research_id)(material_id)(review_id)(weight)(voting_time)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::vote_object, deip::chain::vote_index )

