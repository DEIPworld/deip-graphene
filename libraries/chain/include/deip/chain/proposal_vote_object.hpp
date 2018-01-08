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
        proposal_vote_id_type vote_id;
        proposal_id_type proposal_id;

        fc::time_point_sec voting_time;
        account_t voter;
        share_type weight;
    };

    typedef multi_index_container<proposal_vote_object,
            indexed_by<ordered_unique<tag<by_id>,
                    member<proposal_vote_object,
                            proposal_vote_id_type,
                            &proposal_vote_object::vote_id>>>,
            allocator<proposal_vote_object>>
            proposal_vote_index;

} // namespace chain
} // namespace deip

FC_REFLECT(deip::chain::proposal_vote_object, (vote_id)(proposal_id)(voting_time)(voter)(vote_percent))

CHAINBASE_SET_INDEX_TYPE(deip::chain::proposal_vote_object, deip::chain::proposal_vote_index)