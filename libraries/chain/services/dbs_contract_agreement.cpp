#include <deip/chain/database/database.hpp>
#include <deip/chain/services/dbs_contract_agreement.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace deip {
namespace chain {

dbs_contract_agreement::dbs_contract_agreement(database& db)
    : _base_type(db)
{
}

const contract_agreement_object& dbs_contract_agreement::create(
        const protocol::external_id_type& external_id,
        const protocol::account_name_type& creator,
        const flat_set<protocol::account_name_type>& parties,
        const std::string& hash,
        const fc::time_point_sec& start_time,
        const fc::optional<fc::time_point_sec>& end_time)
{
    const auto& block_time = db_impl().head_block_time();

    const auto& contract = db_impl().create<contract_agreement_object>([&](contract_agreement_object& c_o) {
        c_o.external_id = external_id;
        c_o.creator = creator;

        for (const auto& party: parties)
        {
            c_o.parties[party] = static_cast<uint8_t>(acceptance_status::NotAccepted);
        }

        fc::from_string(c_o.hash, hash);
        c_o.start_time = start_time;
        c_o.end_time = end_time;
        c_o.created_at = block_time;
    });

    return contract;
}

const bool dbs_contract_agreement::exists(const external_id_type &external_id) const
{
    const auto& idx = db_impl()
      .get_index<contract_agreement_index>()
      .indices()
      .get<by_external_id>();

    auto itr = idx.find(external_id);
    return itr != idx.end();
}

const dbs_contract_agreement::optional_ref_type dbs_contract_agreement::get_if_exists(const external_id_type& id) const
{
    optional_ref_type result;

    const auto& idx = db_impl()
      .get_index<contract_agreement_index>()
      .indicies()
      .get<by_external_id>();

    auto itr = idx.find(id);
    if (itr != idx.end())
    {
        result = *itr;
    }

    return result;
}

const dbs_contract_agreement::refs_type dbs_contract_agreement::get_by_creator(
        const account_name_type& creator) const
{
    refs_type ret;

    auto it_pair = db_impl()
      .get_index<contract_agreement_index>()
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

const contract_agreement_object& dbs_contract_agreement::accept_by(
        const contract_agreement_object& contract,
        const account_name_type& party)
{
    db_impl().modify(contract, [&](contract_agreement_object& o) {
        o.parties[party] = static_cast<uint8_t>(acceptance_status::Accepted);
    });

    return contract;
}

const contract_agreement_object& dbs_contract_agreement::reject_by(
        const contract_agreement_object& contract,
        const account_name_type& party)
{
    db_impl().modify(contract, [&](contract_agreement_object& o) {
        o.parties[party] = static_cast<uint8_t>(acceptance_status::Rejected);
    });

    return contract;
}

} //namespace chain
} //namespace deip
