#pragma once
#include <deip/protocol/base.hpp>
#include <deip/protocol/asset.hpp>

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
                    const int64_t& research_id)

        : subaward_number(subaward_number)
        , subaward(subaward)
        , subawardee(subawardee)
        , source(source)
        , research_id(research_id)
    {
    }

    external_id_type subaward_number;
    asset subaward;
    account_name_type subawardee;
    account_name_type source;
    int64_t research_id;

    bool operator<(const subawardee_type& other) const
    {
        return (this->subawardee < other.subawardee);
    }
};

struct create_award_operation : public base_operation
{
    external_id_type funding_opportunity_number;
    external_id_type award_number;

    asset award;
    account_name_type awardee;
    int64_t research_id;
    int64_t university_id;
    uint32_t university_overhead;
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
  (funding_opportunity_number)
  (award_number)
  (award)
  (awardee)
  (research_id)
  (university_id)
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
  (research_id)
)