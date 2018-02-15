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

enum vote_target_type : uint8_t
{
    research_vote = 1,
    content_vote = 2,
    review_vote = 3
};

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
    vote_target_type vote_type;
    int64_t vote_for_id;
    share_type weight;
    time_point_sec voting_time;
};

struct by_discipline_id;
struct by_type_and_target_id;
struct by_voter;


typedef multi_index_container<vote_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<vote_object,
                        vote_id_type,
                        &vote_object::id>>,
                ordered_non_unique<tag<by_discipline_id>,
                        member<vote_object,
                              discipline_id_type,
                              &vote_object::discipline_id>>,
                ordered_non_unique<tag<by_type_and_target_id>,
                        composite_key<vote_object,
                                member<vote_object,
                                      vote_target_type,
                                      &vote_object::vote_type>,
                                member<vote_object,
                                      int64_t,
                                      &vote_object::vote_for_id>>>,
                ordered_non_unique<tag<by_voter>,
                        member<vote_object,
                                account_name_type,
                                &vote_object::voter>>>,
        allocator<vote_object>>
        vote_index;
}
}

FC_REFLECT( deip::chain::vote_object,
            (id)(discipline_id)(voter)(vote_type)(vote_for_id)(weight)(voting_time)
)

FC_REFLECT_ENUM(deip::chain::vote_target_type , (research_vote)(content_vote)(review_vote))

CHAINBASE_SET_INDEX_TYPE( deip::chain::vote_object, deip::chain::vote_index )

