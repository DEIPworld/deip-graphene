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
    optional<research_id_type> research_id;
    optional<research_content_id_type> content_id;
    optional<id_type> review_id;
    share_type weight;
    time_point_sec voting_time;
};

struct by_discipline_id;
struct by_research_id;
struct by_content_id;
struct by_review_id;
struct by_voter;

struct optional_content_id_extractor
{
    typedef optional<research_content_id_type> result_type;

    result_type operator()(const vote_object& v) const
    {
        return v.content_id;
    }
};

struct optional_research_id_extractor
{
    typedef optional<research_id_type> result_type;

    result_type operator()(const vote_object& v) const
    {
        return v.research_id;
    }
};

typedef multi_index_container<vote_object,
        indexed_by<ordered_unique<tag<by_id>,
                member<vote_object,
                        vote_id_type,
                        &vote_object::id>>,
                ordered_non_unique<tag<by_discipline_id>,
                        member<vote_object,
                              discipline_id_type,
                              &vote_object::discipline_id>>,
                ordered_non_unique<tag<by_research_id>,
                        optional_research_id_extractor>,
                ordered_non_unique<tag<by_content_id>,
                        optional_content_id_extractor>,
                ordered_non_unique<tag<by_voter>,
                        member<vote_object,
                                account_name_type,
                                &vote_object::voter>>>,
        allocator<vote_object>>
        vote_index;
}
}

FC_REFLECT( deip::chain::vote_object,
            (id)(discipline_id)(voter)(research_id)(content_id)(review_id)(weight)(voting_time)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::vote_object, deip::chain::vote_index )

