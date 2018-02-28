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
    research_id_type research_id;
    research_content_id_type research_content_id;
    share_type tokens_amount;
    int64_t weight;
    uint16_t voting_power;
    time_point_sec voting_time;
};

struct by_discipline_id;
struct by_research_id;
struct by_research_content_id;
struct by_voter;
struct by_research_and_discipline;
struct by_content_and_discipline;
struct by_voter_discipline_and_research;
struct by_voter_discipline_and_content;

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
                        member<vote_object,
                                research_id_type,
                                &vote_object::research_id>>,
                ordered_non_unique<tag<by_research_and_discipline>,
                        composite_key<vote_object,
                                member<vote_object,
                                       research_id_type,
                                       &vote_object::research_id>,
                                member<vote_object,
                                       discipline_id_type,
                                       &vote_object::discipline_id>>>,
                ordered_non_unique<tag<by_content_and_discipline>,
                        composite_key<vote_object,
                                member<vote_object,
                                       research_content_id_type,
                                       &vote_object::research_content_id>,
                                member<vote_object,
                                       discipline_id_type,
                                       &vote_object::discipline_id>>>,
                ordered_non_unique<tag<by_voter_discipline_and_research>,
                        composite_key<vote_object,
                                member<vote_object,
                                        account_name_type,
                                        &vote_object::voter>,
                                member<vote_object,
                                        discipline_id_type,
                                        &vote_object::discipline_id>,
                                member<vote_object,
                                        research_id_type,
                                        &vote_object::research_id>>>,
                ordered_non_unique<tag<by_voter_discipline_and_content>,
                        composite_key<vote_object,
                                member<vote_object,
                                        account_name_type,
                                        &vote_object::voter>,
                                member<vote_object,
                                        discipline_id_type,
                                        &vote_object::discipline_id>,
                                member<vote_object,
                                        research_content_id_type,
                                        &vote_object::research_content_id>>>,
                ordered_non_unique<tag<by_research_content_id>,
                        member<vote_object,
                                research_content_id_type,
                                &vote_object::research_content_id>>,
                ordered_non_unique<tag<by_voter>,
                        member<vote_object,
                                account_name_type,
                                &vote_object::voter>>>,
        allocator<vote_object>>
        vote_index;
}
}

FC_REFLECT( deip::chain::vote_object,
            (id)(discipline_id)(voter)(research_id)(research_content_id)(tokens_amount)(weight)(voting_power)(voting_time)
)

CHAINBASE_SET_INDEX_TYPE( deip::chain::vote_object, deip::chain::vote_index )

