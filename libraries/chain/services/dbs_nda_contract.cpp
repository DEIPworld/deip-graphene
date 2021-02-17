#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_nda_contract.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace deip {
namespace chain {

dbs_nda_contract::dbs_nda_contract(database& db)
    : _base_type(db)
{
}

const nda_contract_object& dbs_nda_contract::create_research_nda(const external_id_type& external_id,
                                                                 const account_name_type& creator,
                                                                 const std::set<account_name_type>& parties,
                                                                 const fc::string& description,
                                                                 const external_id_type& research_external_id,
                                                                 const fc::time_point_sec& start_time,
                                                                 const fc::time_point_sec& end_time)
{
    const auto& block_time = db_impl().head_block_time();

    const auto& contract = db_impl().create<nda_contract_object>([&](nda_contract_object& c_o) {
        c_o.external_id = external_id;
        c_o.creator = creator;
        c_o.parties.insert(parties.begin(), parties.end());
        fc::from_string(c_o.description, description);
        c_o.research_external_id = research_external_id;
        c_o.start_time = start_time;
        c_o.end_time = end_time;
        c_o.created_at = block_time;
    });

    return contract;
}

const bool dbs_nda_contract::research_nda_exists(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<nda_contract_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    return itr != idx.end();
}

const dbs_nda_contract::nda_contract_optional_ref_type dbs_nda_contract::get_research_nda_if_exists(const external_id_type& external_id) const
{
    nda_contract_optional_ref_type result;

    const auto& idx = db_impl()
      .get_index<nda_contract_index>()
      .indicies()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}


const nda_contract_object& dbs_nda_contract::get_research_nda(const external_id_type& external_id) const
{
    const auto& idx = db_impl()
      .get_index<nda_contract_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    FC_ASSERT(itr != idx.end(), "NDA ${1} does not exist", ("1", external_id));
    return *itr;
}


const dbs_nda_contract::nda_contract_refs_type dbs_nda_contract::get_research_nda_by_creator(const account_name_type& creator) const
{
    nda_contract_refs_type ret;

    auto it_pair = db_impl()
      .get_index<nda_contract_index>()
      .indicies()
      .get<by_creator>()
      .equal_range(creator);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


const dbs_nda_contract::nda_contract_refs_type dbs_nda_contract::get_research_nda_by_hash(const fc::string& hash) const
{
    nda_contract_refs_type ret;

    auto it_pair = db_impl()
      .get_index<nda_contract_index>()
      .indicies()
      .get<by_nda_hash>()
      .equal_range(hash, fc::strcmp_less());

    auto it = it_pair.first;
    const auto it_end = it_pair.second;

    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


const dbs_nda_contract::nda_contract_refs_type dbs_nda_contract::get_research_nda_by_research(const external_id_type& research_external_id) const
{
    nda_contract_refs_type ret;

    auto it_pair = db_impl()
      .get_index<nda_contract_index>()
      .indicies()
      .get<by_research>()
      .equal_range(research_external_id);

    auto it = it_pair.first;
    const auto it_end = it_pair.second;

    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}


void dbs_nda_contract::process_nda_contracts()
{
    const auto& idx = db_impl()
      .get_index<nda_contract_index>()
      .indices()
      .get<by_end_date>();
    
    const auto& head_block = db_impl().head_block_time();
    auto itr = idx.begin();
    const auto itr_end = idx.end();

    while (itr != itr_end)
    {
        auto entity = itr++;
        if (entity->end_time <= head_block)
        {
            db_impl().remove(*entity);
        }
    }
}


} //namespace chain
} //namespace deip