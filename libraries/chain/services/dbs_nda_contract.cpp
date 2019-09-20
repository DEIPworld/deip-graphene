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

const nda_contract_object& dbs_nda_contract::create(const account_name_type& contract_creator,
                                                    const account_name_type& party_a,
                                                    const research_group_id_type& party_a_research_group_id,
                                                    const account_name_type& party_b,
                                                    const research_group_id_type& party_b_research_group_id,
                                                    const fc::string& title,
                                                    const fc::string& contract_hash,
                                                    const fc::time_point_sec& created_at,
                                                    const fc::time_point_sec& start_date,
                                                    const fc::time_point_sec& end_date)
{
    const auto& contract = db_impl().create<nda_contract_object>([&](nda_contract_object& c_o) {
        c_o.contract_creator = contract_creator;
        c_o.party_a = party_a;
        c_o.party_a_research_group_id = party_a_research_group_id;
        fc::from_string(c_o.party_a_signature, "");
        c_o.party_b = party_b;
        c_o.party_b_research_group_id = party_b_research_group_id;
        fc::from_string(c_o.party_b_signature, "");
        fc::from_string(c_o.title, title);        
        fc::from_string(c_o.contract_hash, contract_hash);        
        c_o.created_at = created_at;
        c_o.start_date = start_date;
        c_o.end_date = end_date;
        c_o.status = nda_contract_status::nda_contract_pending;
    });

    return contract;
}

const nda_contract_object& dbs_nda_contract::get(const nda_contract_id_type& id) const
{
    try {
        return db_impl().get<nda_contract_object>(id);
    }
    FC_CAPTURE_AND_RETHROW((id))
}

void dbs_nda_contract::check_contract_existence(const nda_contract_id_type& id) const
{
    const auto& contract = db_impl().find<nda_contract_object, by_id>(id);
    FC_ASSERT(contract != nullptr, "Contract with id \"${1}\" must exist.", ("1", id));
}

dbs_nda_contract::contracts_refs_type dbs_nda_contract::get_by_creator(const account_name_type& party_a)
{
    contracts_refs_type ret;

    auto it_pair = db_impl().get_index<nda_contract_index>().indicies().get<by_party_a>().equal_range(party_a);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_nda_contract::contracts_refs_type dbs_nda_contract::get_by_signee(const account_name_type& party_b)
{
    contracts_refs_type ret;

    auto it_pair = db_impl().get_index<nda_contract_index>().indicies().get<by_party_b>().equal_range(party_b);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_nda_contract::contracts_refs_type dbs_nda_contract::get_by_hash(const fc::string& hash)
{
    contracts_refs_type ret;

    auto it_pair = db_impl().get_index<nda_contract_index>().indicies().get<by_contract_hash>().equal_range(hash, fc::strcmp_less());
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_nda_contract::contracts_refs_type dbs_nda_contract::get_by_creator_research_group(const research_group_id_type& research_group_id)
{
    contracts_refs_type ret;

    auto it_pair = db_impl().get_index<nda_contract_index>().indicies().get<by_creator_research_group>().equal_range(research_group_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_nda_contract::contracts_refs_type dbs_nda_contract::get_by_signee_research_group(const research_group_id_type& research_group_id)
{
    contracts_refs_type ret;

    auto it_pair = db_impl().get_index<nda_contract_index>().indicies().get<by_signee_research_group>().equal_range(
        research_group_id);
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_nda_contract::contracts_refs_type dbs_nda_contract::get_by_creator_research_group_and_contract_hash(const research_group_id_type& research_group_id, const fc::string& hash)
{
    contracts_refs_type ret;

    auto it_pair = db_impl()
                       .get_index<nda_contract_index>()
                       .indicies()
                       .get<by_creator_research_group_and_contract_hash>()
                       .equal_range(boost::make_tuple(research_group_id, hash));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_nda_contract::contracts_refs_type dbs_nda_contract::get_by_signee_research_group_and_contract_hash(const research_group_id_type& research_group_id, const fc::string& hash)
{
    contracts_refs_type ret;

    auto it_pair = db_impl()
                       .get_index<nda_contract_index>()
                       .indicies()
                       .get<by_signee_research_group_and_contract_hash>()
                       .equal_range(boost::make_tuple(research_group_id, hash));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_nda_contract::contracts_refs_type dbs_nda_contract::get_by_creator_research_group_and_signee_research_group(const research_group_id_type& party_a_research_group_id, const research_group_id_type& party_b_research_group_id)
{
    contracts_refs_type ret;

    auto it_pair = db_impl()
                       .get_index<nda_contract_index>()
                       .indicies()
                       .get<by_creator_research_group_and_signee_research_group>()
                       .equal_range(boost::make_tuple(party_a_research_group_id, party_b_research_group_id));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

dbs_nda_contract::contracts_refs_type
dbs_nda_contract::get_by_creator_research_group_and_signee_research_group_and_contract_hash(
    const research_group_id_type& party_a_research_group_id,
    const research_group_id_type& party_b_research_group_id,
    const fc::string& hash)
{
    contracts_refs_type ret;

    auto it_pair = db_impl()
                       .get_index<nda_contract_index>()
                       .indicies()
                       .get<by_creator_research_group_and_signee_research_group_and_contract_hash>()
                       .equal_range(boost::make_tuple(party_a_research_group_id, party_b_research_group_id, hash));
    auto it = it_pair.first;
    const auto it_end = it_pair.second;
    while (it != it_end)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

const nda_contract_object& dbs_nda_contract::sign(const nda_contract_object& contract, const account_name_type& contract_signer, const fc::string& sig)
{
    db_impl().modify(contract, [&](nda_contract_object& c_o) {
        const bool is_party_a_sig = c_o.party_a == contract_signer;
        std::string stringified_signature = is_party_a_sig
          ? fc::to_string(c_o.party_a_signature)
          : fc::to_string(c_o.party_b_signature);

        if (stringified_signature.empty())
        {
            fc::from_string(is_party_a_sig ? c_o.party_a_signature : c_o.party_b_signature, sig);
        }
        else
        {
            std::set<std::string> signature_set;
            boost::split(signature_set, stringified_signature, boost::is_any_of(","));
            signature_set.insert(sig);
            std::string updated_stringified_signature = boost::algorithm::join(signature_set, ",");
            fc::from_string(is_party_a_sig ? c_o.party_a_signature : c_o.party_b_signature,
                            updated_stringified_signature);
        }
    });
    return db_impl().get<nda_contract_object>(contract.id);
}

void dbs_nda_contract::set_new_contract_status(const nda_contract_object& contract, const nda_contract_status& status)
{
    db_impl().modify(contract, [&](nda_contract_object& c_o) { c_o.status = status; });
}

} //namespace chain
} //namespace deip