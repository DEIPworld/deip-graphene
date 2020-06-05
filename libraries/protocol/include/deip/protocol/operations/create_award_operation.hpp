#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/asset.hpp>
#include <deip/protocol/percent.hpp>

namespace deip {
namespace protocol {

using deip::protocol::asset;

struct subawardee_type
{
    subawardee_type()
    {
    }

    subawardee_type(const external_id_type& subaward_number,
                    const asset& subaward,
                    const account_name_type& subawardee,
                    const account_name_type& source,
                    const external_id_type& research_id)

        : subaward_number(subaward_number)
        , subaward(subaward)
        , subawardee(subawardee)
        , source(source)
        , research_external_id(research_id)
    {
    }

    external_id_type subaward_number;
    asset subaward;
    account_name_type subawardee;
    account_name_type source;
    external_id_type research_external_id;

    bool operator<(const subawardee_type& other) const
    {
        return (this->subawardee < other.subawardee);
    }
};

struct create_award_operation : public base_operation
{
    external_id_type award_number;
    external_id_type funding_opportunity_number;
    asset award;
    account_name_type awardee;
    external_id_type research_external_id;
    external_id_type university_external_id;
    percent university_overhead;
    vector<subawardee_type> subawardees;
    account_name_type creator;
    
    extensions_type extensions;

    void validate() const;

    void get_required_active_authorities(flat_set<account_name_type>& a) const
    {
        a.insert(creator);
    }
};


}
}

FC_REFLECT(deip::protocol::create_award_operation, 
  (award_number)
  (funding_opportunity_number)
  (award)
  (awardee)
  (research_external_id)
  (university_external_id)
  (university_overhead)
  (subawardees)
  (creator)
  (extensions)
)

FC_REFLECT(deip::protocol::subawardee_type, 
  (subaward_number)
  (subaward)
  (subawardee)
  (source)
  (research_external_id)
)