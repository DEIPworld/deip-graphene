//
// Created by dzeranov on 7.1.18.
//
#pragma once

#include <deip/chain/deip_object_types.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace deip{
namespace chain{

    class proposal_vote_object : public object<proposal_vote_object_type, proposal_vote_object>
    {
        typedef deip::protocol::account_name_type account_t;

    public:
        template <typename Constructor, typename Allocator> proposal_vote_object(Constructor&& c, allocator<Allocator> a)
        {
            c(*this);
        }
    public:
        proposal_vote_id_type id;
        proposal_id_type proposal_id;
        research_group_id_type research_group_id;
        fc::time_point_sec voting_time;
        account_t voter;
        share_type weight;
    };

    struct by_voter;
    struct by_proposal_id;

    typedef multi_index_container<proposal_vote_object,
                                            indexed_by<
                                                ordered_unique<tag<by_id>,
                                                            member<
                                                                proposal_vote_object,
                                                                proposal_vote_id_type,
                                                                &proposal_vote_object::id>>,
                                                ordered_non_unique<tag<by_voter>,
                                                                composite_key<proposal_vote_object,
                                                                      member<proposal_vote_object,
                                                                             account_name_type,
                                                                             &proposal_vote_object::voter>,
                                                                      member<proposal_vote_object,
                                                                             research_group_id_type,
                                                                             &proposal_vote_object::research_group_id>>>,
                                                ordered_non_unique<tag<by_proposal_id>,
                                                    member<proposal_vote_object,
                                                           proposal_id_type,
                                                           &proposal_vote_object::proposal_id>>
                                            >,
                                            allocator<proposal_vote_object>>
    proposal_vote_index;

} // namespace chain
} // namespace deip

FC_REFLECT(deip::chain::proposal_vote_object, (id)(proposal_id)(research_group_id)(voting_time)(voter)(weight))

CHAINBASE_SET_INDEX_TYPE(deip::chain::proposal_vote_object, deip::chain::proposal_vote_index)