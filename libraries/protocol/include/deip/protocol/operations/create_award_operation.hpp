#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/block_header.hpp>
#include <deip/protocol/version.hpp>
#include <deip/protocol/asset.hpp>
#include <fc/optional.hpp>

namespace deip {
namespace protocol {

using deip::protocol::asset;

struct awardee_type
{
    awardee_type()
    {
    }

    awardee_type(const account_name_type& awardee,
                 const int64_t& research_id,
                 const int64_t& university_id,
                 const share_type& university_overhead,
                 const asset& award,
                 const fc::optional<account_name_type>& source)
            : awardee(awardee)
            , research_id(research_id)
            , university_id(university_id)
            , university_overhead(university_overhead)
            , award(award)
            , source(source)
    {
    }

    account_name_type awardee;
    int64_t research_id;
    int64_t university_id;
    share_type university_overhead;
    asset award;

    fc::optional<account_name_type> source;

    bool operator<(const awardee_type& other) const
    {
        return (this->awardee < other.awardee);
    }
};

struct create_award_operation : public base_operation
{
    int64_t funding_opportunity_id;
    account_name_type creator;

    flat_set<awardee_type> awardees;
    asset award;

    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(creator);
    }

};


}
}

FC_REFLECT( deip::protocol::create_award_operation, (funding_opportunity_id)(creator)(awardees)(award)(extensions) )
FC_REFLECT( deip::protocol::awardee_type, (awardee)(research_id)(university_id)(university_overhead)(award)(source) )